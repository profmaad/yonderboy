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
	// create basic GUI elements
	mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	tabBar = gtk_notebook_new();
	tabs = new std::vector<GtkWidget*>();
	
	// arrange GUI
	gtk_container_add(GTK_CONTAINER(mainWindow), tabBar);

	// show gui
	gtk_widget_show_all(mainWindow);

	// connect signals
	g_signal_connect_swapped(mainWindow, "destroy", G_CALLBACK(&ViewerController::gtkDestroyCallback), this);

	initID = getNextPackageID();
	Package* initPackage = constructPackage("connection-management", "id", initID.c_str(), "command", "initialize", "client-name", PROJECT_NAME, "client-version", PROJECT_VERSION, "can-display-stati", "",  NULL);
	sendPackageAndDelete(initPackage);
}
ViewerController::~ViewerController()
{
	
}

void ViewerController::handlePackage(Package *thePackage)
{
	std::cerr<<"___TABBEDGTKVIEWER::ViewerController___got Package with id "<<thePackage->getID()<<std::endl;

	std::string error;

	switch(thePackage->getType())
	{
	case Command:
		error = handleCommand(thePackage);
		sendPackageAndDelete(constructAcknowledgementPackage(thePackage, error));
		break;
	case Acknowledgement:
		if(thePackage->getValue("id") == initID && !initialised)
		{
			initialised = true;
			createNewTab();
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

	return error;
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

gint ViewerController::createNewTab()
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
		tabs->
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
	
	sendPackageAndDelete(constructPackage("connection-management", "command", "register-view", "id", getNextPackageID().c_str(), "display-information-type", DISPLAY_INFORMATION_TYPE, "display-information", displayInformationConversionStream.str().c_str(), "view-id", viewIDConversionStream.str().c_str(), "can-be-reassigned", "", NULL));

	gtk
	
	return result;
}
