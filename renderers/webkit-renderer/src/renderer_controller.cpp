//      renderer_controller.cpp
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

# include "renderer_controller.h"

# include "defaults.h"
# include <ev_cpp.h>

# include <package.h>
# include <package_factories.h>

RendererController::RendererController(int socket) : ClientController(socket), backendWebView(NULL), backendPlug(NULL)
{
// initialize webkit backend (webview inside plug)
	backendWebView = webkit_web_view_new();
	backendScrolledWindow = gtk_scrolled_window_new(NULL,NULL);
	backendPlug = gtk_plug_new(0);
	gtk_container_add(GTK_CONTAINER(backendScrolledWindow), backendWebView);
	gtk_container_add(GTK_CONTAINER(backendPlug), backendScrolledWindow);

	// setup signals
	g_signal_connect_swapped(backendPlug, "embedded", G_CALLBACK(&RendererController::plugEmbeddedCallback),this);

	// show everything
	gtk_widget_show_all(backendPlug);

	// send init package
	std::stringstream conversionStream;
	conversionStream<<gtk_plug_get_id(GTK_PLUG(backendPlug));
	
	Package* initPackage = constructPackage("connection-management", "id", getNextPackageID().c_str(), "command", "initialize", "client-name", PROJECT_NAME, "client-version", PROJECT_VERSION, "backend-name", BACKEND_NAME, "backend-version", BACKEND_VERSION, "display-information-type", DISPLAY_INFORMATION_TYPE, "display-information", conversionStream.str().c_str(), NULL);
	sendPackageAndDelete(initPackage);
}
RendererController::~RendererController()
{
	
}

void RendererController::handlePackage(Package *thePackage)
{
	std::cerr<<"___WEBKITRENDERER::RendererController___got Package with id "<<thePackage->getID()<<std::endl;

	std::string error;

	switch(thePackage->getType())
	{
	case Command:
		error = handleCommand(thePackage);
		sendPackageAndDelete(constructAcknowledgementPackage(thePackage, error));
		break;
	case Response:
		break;
	case ConnectionManagement:
		break;
	}
}
std::string RendererController::handleCommand(Package *thePackage)
{
	std::string command = thePackage->getValue("command");
	WebKitWebView *view = WEBKIT_WEB_VIEW(backendWebView);

	std::string error;

	if(command == "open-uri" && thePackage->hasValue("uri"))
	{
		webkit_web_view_load_uri(view, thePackage->getValue("uri").c_str());
	}
	else if(command == "reload-page")
	{
		if(thePackage->isSet("bypass-cache"))
		{
			webkit_web_view_reload_bypass_cache(view);
		}
		else
		{
			webkit_web_view_reload(view);
		}
	}
	else
	{
		error = "invalid";
	}

	return error;
}

void RendererController::signalSocketClosed()
{
	std::cerr<<"___WEBKITRENDERER___going down in signalSocketClosed"<<std::endl;

	closeServerConnection();

	gtk_main_quit();
}

void RendererController::plugEmbeddedCallback(GtkPlug *plug)
{
	std::cerr<<"___WEBKITRENDERER___plug got embedded into socket"<<std::endl;
}
