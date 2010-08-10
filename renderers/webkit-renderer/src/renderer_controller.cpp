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

# include "renderer_controller.h"

# include "defaults.h"

# include <package.h>
# include <package_factories.h>

RendererController::RendererController(int socket) : AbstractHost(socket)
{
// initialize webkit backend (webview inside plug)
	backendWebView = webkit_web_view_new();
	backendPlug = gtk_plug_new(0);
	gtk_container_add(GTK_CONTAINER(backendPlug), backendWebView);

	// setup signals

	// send init package
	std::stringstream conversionStream;
	conversionStream<<gtk_plug_get_id(GTK_PLUG(backendPlug));
	
	Package* initPackage = constructPackage("connection-management", "id", getNextPackageID().c_str(), "command", "initialize", "client-name", PROJECT_NAME, "client-version", PROJECT_VERSION, "backend-name", BACKEND_NAME, "backend-version", BACKEND_VERSION, "display-information-type", DISPLAY_INFORMATION_TYPE, "display-information", conversionStream.str().c_str(), NULL);
	sendPackageAndDelete(initPackage);

	// react to signals from backend and commands from server
}

RendererController::~RendererController()
{
}

void RendererController::handlePackage(Package* thePackage)
{
}
void RendererController::socketClosed()
{
	ev::get_default_loop().unloop(ev::ALL);
}
