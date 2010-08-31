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

# include "viewer_controller.h"

# include "defaults.h"
# include <ev_cpp.h>

# include <package.h>
# include <package_factories.h>

ViewerController::ViewerController(int socket) : ClientController(socket), initialised(false), nextViewID(0)
{
	// create state
	socketByID = new std::map<std::string, GtkSocket*>();

	// create basic GUI elements
	mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	mainVBox = gtk_vbox_new(FALSE, 0);
	tabBar = gtk_notebook_new();
	tabs = new std::vector<GtkWidget*>();
	statusBar = gtk_statusbar_new();
	statusBarContextLocal = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "local");
	statusBarContextServer = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "server");
	statusBarContextRendererLoad = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "renderer-load");
	statusBarContextRendererHover = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "renderer-hover");
	
	// arrange GUI
	gtk_box_pack_start(GTK_BOX(mainVBox), tabBar, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(mainVBox), statusBar, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(mainWindow), mainVBox);

	// show gui
	gtk_widget_show_all(mainWindow);

	// connect signals
	g_signal_connect_swapped(mainWindow, "destroy", G_CALLBACK(&ViewerController::gtkDestroyCallback), this);

	initID = getNextPackageID();
	Package* initPackage = constructPackage("connection-management", "id", initID.c_str(), "command", "initialize", "client-name", PROJECT_NAME, "client-version", PROJECT_VERSION, "can-display-stati", "",  NULL);
	sendPackageAndDelete(initPackage);

	gtk_statusbar_push(GTK_STATUSBAR(statusBar), statusBarContextLocal, "Welcome to tabbed-gtk-viewer on yonderboy 0.0");
}
ViewerController::~ViewerController()
{
	
}

void ViewerController::handlePackage(Package *thePackage)
{
	std::string error;

	switch(thePackage->getType())
	{
	case Command:
		error = handleCommand(thePackage);
		sendPackageAndDelete(constructAcknowledgementPackage(thePackage, error));
		break;
	case StatusChange:
		handleStatusChange(thePackage);
		break;
	case Acknowledgement:
		if(thePackage->getValue("id") == initID && !initialised)
		{
			initialised = true;
			createNewTab(true);
			createNewTab(true);
			createNewTab(true);
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
	std::string error = "unknown";

	if(command == "connect-to-renderer" && thePackage->hasValue("display-information") && thePackage->hasValue("view-id"))
	{
		std::cerr<<"connecting to renderer "<<thePackage->getValue("renderer-id")<<" with view "<<thePackage->getValue("view-id")<<": "<<thePackage->getValue("display-information")<<std::endl;

		GtkSocket *theSocket = retrieveSocket(thePackage->getValue("view-id"));
		if(!theSocket){ return std::string("invalid"); }
		
		gtk_socket_add_id(theSocket, 0); // remove plug
				  
		std::istringstream conversionStream(thePackage->getValue("display-information"));
		GdkNativeWindow plugID;
		conversionStream>>plugID;
		
		gtk_socket_add_id(theSocket, plugID);
	}
	else if(command == "disconnect-from-renderer" && thePackage->hasValue("view-id"))
	{
		GtkSocket *theSocket = retrieveSocket(thePackage->getValue("view-id"));
		if(!theSocket) { return std::string("invalid"); }

		gtk_socket_add_id(theSocket, 0);
	}

	return error;
}
void ViewerController::handleStatusChange(Package *thePackage)
{
	GtkStatusbar *bar = GTK_STATUSBAR(statusBar);
	std::string status = thePackage->getValue("status");

	if(status == "load-started")
	{
	}
	else if(status == "load-finished")
	{
	}
	else if(status == "load-failed")
	{
	}
	else if(status == "progress-changed")
	{
	}
	else if(status == "hovering-over-link")
	{
	}
	else if(status == "not-hovering-over-link")
	{
	}
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
	if(!gtkSocket) { return result; }
	
	tabs->push_back(gtkSocket);
	gtk_widget_show(gtkSocket);
	result = gtk_notebook_append_page(GTK_NOTEBOOK(tabBar), gtkSocket, NULL);
	if(result < 0)
	{
		tabs->erase(tabs->end()-1);
		g_object_unref(gtkSocket);
		return result;
	}
	gtk_notebook_set_current_page(GTK_NOTEBOOK(tabBar), -1);

	std::ostringstream displayInformationConversionStream;
	displayInformationConversionStream<<gtk_socket_get_id(GTK_SOCKET(gtkSocket));
	std::ostringstream viewIDConversionStream;
	viewIDConversionStream<<"view";
	viewIDConversionStream<<nextViewID;
	nextViewID++;
	
	if(createRenderer)
	{
		sendPackageAndDelete(constructPackage("connection-management", "command", "register-view", "id", getNextPackageID().c_str(), "display-information-type", DISPLAY_INFORMATION_TYPE, "display-information", displayInformationConversionStream.str().c_str(), "view-id", viewIDConversionStream.str().c_str(), "can-be-reassigned", "", "create-renderer", "", NULL));
	}
	else
	{
		sendPackageAndDelete(constructPackage("connection-management", "command", "register-view", "id", getNextPackageID().c_str(), "display-information-type", DISPLAY_INFORMATION_TYPE, "display-information", displayInformationConversionStream.str().c_str(), "view-id", viewIDConversionStream.str().c_str(), "can-be-reassigned", "", NULL));
	}

	socketByID->insert(std::make_pair(viewIDConversionStream.str(), GTK_SOCKET(gtkSocket)));

	gtk_notebook_set_tab_label(GTK_NOTEBOOK(tabBar), gtkSocket, gtk_label_new(viewIDConversionStream.str().c_str()));
	
	return result;
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
