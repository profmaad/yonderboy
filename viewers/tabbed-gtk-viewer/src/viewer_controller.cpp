//      viewer_controller.cpp
//      
//      Copyright 2009 Prof. MAAD <prof.maad@lambda-bb.de>
//      
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//      
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//      
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.

# include <string>
# include <vector>
# include <map>
# include <utility>
# include <sstream>
# include <stdexcept>
# include <iostream>

# include <cerrno>
# include <cstring>
# include <sys/types.h>
# include <sys/socket.h>
# include <unistd.h>

# include <gdk/gdkkeysyms.h>

# include "viewer_controller.h"

# include "defaults.h"
# include <ev_cpp.h>

# include <package.h>
# include <package_factories.h>

ViewerController::ViewerController(int socket) : ClientController(socket), initialised(false), nextViewID(0)
{
	// create state
	socketByID = new std::map<std::string, GtkSocket*>();
	viewByRenderer = new std::map<std::string, std::string>();

	// create basic GUI elements
	mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	mainVBox = gtk_vbox_new(FALSE, 0);
	tabBar = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(tabBar), TRUE);
	gtk_notebook_popup_enable(GTK_NOTEBOOK(tabBar));
	statusBar = gtk_statusbar_new();
	statusBarProgress = gtk_progress_bar_new();
	statusBarContextLocal = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "local");
	statusBarContextServer = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "server");
	statusBarContextTab = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "tab");

	globalHotkeys = gtk_accel_group_new();
	setupHotkeys();
	gtk_window_add_accel_group(GTK_WINDOW(mainWindow), globalHotkeys);
	
	// arrange GUI
	gtk_box_pack_end(GTK_BOX(statusBar), statusBarProgress, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mainVBox), tabBar, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(mainVBox), statusBar, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(mainWindow), mainVBox);

	// show gui
	gtk_widget_show_all(mainWindow);
	gtk_widget_hide(statusBarProgress);

	// connect signals
	g_signal_connect_swapped(mainWindow, "destroy", G_CALLBACK(&ViewerController::gtkDestroyCallback), this);
	g_signal_connect_swapped(tabBar, "switch-page", G_CALLBACK(&ViewerController::tabBarSwitchPageCallback), this);
	g_signal_connect_swapped(tabBar, "page-added", G_CALLBACK(&ViewerController::pageAddedCallback), this);
	g_signal_connect_swapped(tabBar, "page-removed", G_CALLBACK(&ViewerController::pageRemovedCallback), this);

	initID = getNextPackageID();
	Package* initPackage = constructPackage("connection-management", "id", initID.c_str(), "command", "initialize", "client-name", PROJECT_NAME, "client-version", PROJECT_VERSION, "can-display-stati", "",  NULL);
	sendPackageAndDelete(initPackage);

	gtk_statusbar_push(GTK_STATUSBAR(statusBar), statusBarContextLocal, "Welcome to tabbed-gtk-viewer on yonderboy 0.0");
}
ViewerController::~ViewerController()
{
	
}

void ViewerController::setupHotkeys()
{
	gtk_accel_group_connect(globalHotkeys, GDK_Right, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, g_cclosure_new_swap(GCallback(&ViewerController::nextTabCallback), this, NULL));
	gtk_accel_group_connect(globalHotkeys, GDK_Left, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, g_cclosure_new_swap(GCallback(&ViewerController::previousTabCallback), this, NULL));
	gtk_accel_group_connect(globalHotkeys, GDK_W, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, g_cclosure_new_swap(GCallback(&ViewerController::closeTabCallback), this, NULL));
	gtk_accel_group_connect(globalHotkeys, GDK_T, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, g_cclosure_new_swap(GCallback(&ViewerController::newTabCallback), this, NULL));
}

void ViewerController::handlePackage(Package *thePackage)
{
	std::string error;

	switch(thePackage->getType())
	{
	case Command:
		error = handleCommand(thePackage);
		std::cerr<<"handleCommand.error: "<<error<<std::endl;
		sendPackageAndDelete(constructAcknowledgementPackage(thePackage, error));
		break;
	case StatusChange:
		handleStatusChange(thePackage);
		break;
	case Acknowledgement:
		if(thePackage->getValue("id") == initID && !initialised)
		{
			initialised = true;
			createNewTab(true); //HC
		}
		if(thePackage->hasValue("error"))
		{
			std::cerr<<"got negative ack for "<<thePackage->getID()<<": "<<thePackage->getValue("error")<<std::endl;
		}
		break;
	case ConnectionManagement:
		break;
	}

	delete thePackage;
}
std::string ViewerController::handleCommand(Package *thePackage)
{
	std::string command = thePackage->getValue("command");
	std::string error;

	if(command == "connect-to-renderer" && thePackage->hasValue("display-information") && thePackage->hasValue("view-id") && thePackage->hasValue("renderer-id"))
	{
		std::cerr<<"connecting to renderer "<<thePackage->getValue("renderer-id")<<" with view "<<thePackage->getValue("view-id")<<": "<<thePackage->getValue("display-information")<<std::endl;

		GtkSocket *theSocket = retrieveSocket(thePackage->getValue("view-id"));
		if(!(theSocket && GTK_IS_SOCKET(theSocket))){ return std::string("invalid"); }
		
		gtk_socket_add_id(theSocket, 0); // remove plug
				  
		std::istringstream conversionStream(thePackage->getValue("display-information"));
		GdkNativeWindow plugID;
		conversionStream>>plugID;

		gtk_socket_add_id(theSocket, plugID);

		viewByRenderer->erase(thePackage->getValue("renderer-id"));
		viewByRenderer->insert(std::make_pair(thePackage->getValue("renderer-id"), thePackage->getValue("view-id")));
	}
	else if(command == "disconnect-from-renderer" && thePackage->hasValue("view-id") && thePackage->hasValue("renderer-id"))
	{
		std::cerr<<"disconnecting "<<thePackage->getValue("view-id")<<" from "<<thePackage->getValue("renderer-id")<<std::endl;

		GtkSocket *theSocket = retrieveSocket(thePackage->getValue("view-id"));
		if(!theSocket) { return std::string("invalid"); }

		gtk_socket_add_id(theSocket, 0);

		viewByRenderer->erase(thePackage->getValue("renderer-id"));
	}
	else
	{
		error = "unknown";
	}

	return error;
}
void ViewerController::handleStatusChange(Package *thePackage)
{
	if(!(thePackage->getValue("source-type") == "renderer" && thePackage->hasValue("source-id"))) { return; }

	GtkStatusbar *bar = GTK_STATUSBAR(statusBar);
	std::string status = thePackage->getValue("status");
	std::string sourceID = thePackage->getValue("source-id");
	GtkWidget *tab = NULL;
	std::map<std::string, std::string>::const_iterator idIter = viewByRenderer->find(thePackage->getValue("source-id"));
	if(idIter != viewByRenderer->end())
	{
		std::map<std::string, GtkSocket*>::const_iterator socketIter = socketByID->find(idIter->second);
		if(socketIter != socketByID->end())
		{
			tab = (GtkWidget*)socketIter->second;
		}
		else { return; }
	}
	else { return; }       

	if(status == "load-started")
	{
		setStatusOnTab(tab, std::string("started loading ")+thePackage->getValue("uri"));
	}
	else if(status == "load-finished")
	{
		setStatusOnTab(tab, std::string("done loading ")+thePackage->getValue("uri"));
	}
	else if(status == "load-failed")
	{
		setStatusOnTab(tab, std::string("loading failed"));
	}
	else if(status == "progress-changed")
	{
		gdouble progressDouble = 0.0;
		std::istringstream conversionStream(thePackage->getValue("progress"));
		conversionStream>>progressDouble;

		setProgressOnTab(tab, progressDouble);
	}
	else if(status == "hovering-over-link")
	{
		setStatusOnTab(tab, thePackage->getValue("uri"));
	}
	else if(status == "not-hovering-over-link")		
	{
		setStatusOnTab(tab, "");
	}

	if(tab == gtk_notebook_get_nth_page(GTK_NOTEBOOK(tabBar), gtk_notebook_get_current_page(GTK_NOTEBOOK(tabBar))))	{ updateStatusBar(gtk_notebook_get_current_page(GTK_NOTEBOOK(tabBar))); }
}

void ViewerController::signalSocketClosed()
{
	std::cerr<<"___TABBEDGTKVIEWER___going down in signalSocketClosed"<<std::endl;

	closeServerConnection();

	gtk_main_quit();
}
void ViewerController::gtkDestroyCallback(GtkObject *object)
{
	std::cerr<<"___TABBEDGTKVIEWER___received gtk destroy signal, going down"<<std::endl;

	closeServerConnection();

	gtk_main_quit();
}

gint ViewerController::createNewTab(bool createRenderer)
{
	gint result = -1;
	if(!initialised) { return result; }

	GtkWidget *gtkSocket = gtk_socket_new();
	if(!(gtkSocket && GTK_IS_SOCKET(gtkSocket))) { return result; }

	g_signal_connect_swapped(gtkSocket, "plug-added", G_CALLBACK(&ViewerController::plugAddedCallback), this); 
	g_signal_connect_swapped(gtkSocket, "plug-removed", G_CALLBACK(&ViewerController::plugRemovedCallback), this);

	result = gtk_notebook_append_page(GTK_NOTEBOOK(tabBar), gtkSocket, NULL);
	if(result < 0)
	{
		std::cerr<<"failed to append page"<<std::endl;
		return result;
	}
	gtk_widget_show(gtkSocket);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(tabBar), -1);

	std::ostringstream displayInformationConversionStream;
	displayInformationConversionStream<<gtk_socket_get_id(GTK_SOCKET(gtkSocket));
	std::ostringstream viewIDConversionStream;
	viewIDConversionStream<<"view";
	viewIDConversionStream<<nextViewID;
	nextViewID++;

	socketByID->insert(std::make_pair(viewIDConversionStream.str(), GTK_SOCKET(gtkSocket)));
	g_object_ref(G_OBJECT(gtkSocket));

	gtk_notebook_set_tab_label(GTK_NOTEBOOK(tabBar), gtkSocket, gtk_label_new(viewIDConversionStream.str().c_str()));

	// set status data on new tab
	g_object_set_data_full(G_OBJECT(gtkSocket), "view-id", g_strdup(viewIDConversionStream.str().c_str()), (GDestroyNotify)g_free);	
	
	if(createRenderer)
	{
		sendPackageAndDelete(constructPackage("connection-management", "command", "register-view", "id", getNextPackageID().c_str(), "display-information-type", DISPLAY_INFORMATION_TYPE, "display-information", displayInformationConversionStream.str().c_str(), "view-id", viewIDConversionStream.str().c_str(), "can-be-reassigned", "", "create-renderer", "", NULL));
	}
	else
	{
		sendPackageAndDelete(constructPackage("connection-management", "command", "register-view", "id", getNextPackageID().c_str(), "display-information-type", DISPLAY_INFORMATION_TYPE, "display-information", displayInformationConversionStream.str().c_str(), "view-id", viewIDConversionStream.str().c_str(), "can-be-reassigned", "", NULL));
	}

	std::cerr<<"created new tab "<<result<<std::endl;

	return result;
}
void ViewerController::closeTab(guint pageNum)
{

}
GtkSocket* ViewerController::retrieveSocket(std::string viewID)
{
	std::map<std::string, GtkSocket*>::const_iterator iter = socketByID->find(viewID);
	if(iter != socketByID->end())
	{
		return iter->second;
	}

	return NULL;
}
gdouble ViewerController::getProgressFromTab(GtkWidget *tab)
{
	gdouble *progress = (gdouble*)g_object_get_data(G_OBJECT(tab), "load-progress");
	if(progress) { return *progress; }
	else { return 0; }
}
void ViewerController::setProgressOnTab(GtkWidget *tab, gdouble progress)
{
	gdouble *progressPointer = (gdouble*)g_malloc0(sizeof(progress));
	*progressPointer = progress;
	g_object_set_data_full(G_OBJECT(tab), "load-progress", (gpointer)progressPointer, (GDestroyNotify)g_free);
}
const char* ViewerController::getStatusFromTab(GtkWidget *tab)
{
	return (const char*)g_object_get_data(G_OBJECT(tab), "status-message");
}
void ViewerController::setStatusOnTab(GtkWidget *tab, std::string status)
{
	g_object_set_data_full(G_OBJECT(tab), "status-message", g_strdup(status.c_str()), (GDestroyNotify)g_free);
}

void ViewerController::updateStatusBar(guint currentPage)
{
	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(tabBar), currentPage);
	gdouble progress = getProgressFromTab(tab);
	const char* status = getStatusFromTab(tab);

	gtk_statusbar_pop(GTK_STATUSBAR(statusBar), statusBarContextTab);

	if(progress > 0 && progress < 1)
	{
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(statusBarProgress), progress);
		gtk_widget_show(statusBarProgress);
	}
	else
	{
		gtk_widget_hide(statusBarProgress);
	}

	if(status)
	{
		gtk_statusbar_push(GTK_STATUSBAR(statusBar), statusBarContextTab, status);
	}
}

void ViewerController::tabBarSwitchPageCallback(GtkNotebookPage *page, guint pageNum, GtkNotebook *notebook)
{
	updateStatusBar(pageNum);
}
void ViewerController::nextTabCallback()
{
	if(gtk_notebook_get_current_page(GTK_NOTEBOOK(tabBar)) == gtk_notebook_get_n_pages(GTK_NOTEBOOK(tabBar))-1)
	{
		gtk_notebook_set_current_page(GTK_NOTEBOOK(tabBar), 0);
	}
	else
	{
		gtk_notebook_next_page(GTK_NOTEBOOK(tabBar));
	}
}
void ViewerController::previousTabCallback()
{
	if(gtk_notebook_get_current_page(GTK_NOTEBOOK(tabBar)) == 0)
	{
		gtk_notebook_set_current_page(GTK_NOTEBOOK(tabBar), gtk_notebook_get_n_pages(GTK_NOTEBOOK(tabBar))-1);
	}
	else
	{
		gtk_notebook_prev_page(GTK_NOTEBOOK(tabBar));
	}
}
void ViewerController::closeTabCallback()
{
	closeTab(gtk_notebook_get_current_page(GTK_NOTEBOOK(tabBar)));
}
void ViewerController::newTabCallback()
{
	int result = createNewTab(true); //HC
}

void ViewerController::pageAddedCallback(GtkWidget *child, guint pageNum, GtkNotebook *notebook)
{
	std::cerr<<"page added: "<<child<<" @ "<<pageNum<<std::endl;
}
void ViewerController::pageRemovedCallback(GtkWidget *child, guint pageNum, GtkNotebook *notebook)
{
	std::cerr<<"page removed: "<<child<<" @ "<<pageNum<<std::endl;
}
void ViewerController::plugAddedCallback(GtkSocket *socket)
{
	std::cerr<<"plug added to "<<socket<<": "<<gtk_socket_get_plug_window(GTK_SOCKET(socket))<<" ("<<g_type_name(G_OBJECT_TYPE(gtk_socket_get_plug_window(GTK_SOCKET(socket))))<<")"<<std::endl;
}
gboolean ViewerController::plugRemovedCallback(GtkSocket *socket)
{
	std::cerr<<"plug removed: "<<socket<<"|"<<gtk_socket_get_plug_window(GTK_SOCKET(socket))<<std::endl;

	return true;
}
