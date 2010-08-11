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
# include "ev_cpp.h"

# include <package.h>
# include <package_factories.h>
# include <server_connection.h>

RendererController::RendererController(int socket)
{
// initialize webkit backend (webview inside plug)
	backendWebView = webkit_web_view_new();
	backendPlug = gtk_plug_new(0);
	gtk_container_add(GTK_CONTAINER(backendPlug), backendWebView);

	// setup signals

	// setup server connection
	setupServerConnection(socket);

	// send init package
	std::stringstream conversionStream;
	conversionStream<<gtk_plug_get_id(GTK_PLUG(backendPlug));
	
	Package* initPackage = constructPackage("connection-management", "id", serverConnection->getNextPackageID().c_str(), "command", "initialize", "client-name", PROJECT_NAME, "client-version", PROJECT_VERSION, "backend-name", BACKEND_NAME, "backend-version", BACKEND_VERSION, "display-information-type", DISPLAY_INFORMATION_TYPE, "display-information", conversionStream.str().c_str(), NULL);
	sendPackageAndDelete(initPackage);

	// react to signals from backend and commands from server
}

RendererController::~RendererController()
{
	pthread_join(serverConnectionThread, NULL);
	delete serverConnection;
}

void RendererController::setupServerConnection(int serverSocket)
{
	int sockets[2] = { -1, -1 };
	int result = -1;
	char errorBuffer[128] = { '\0' };

	result = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	if(result < 0)
	{
		strerror_r(errno, errorBuffer, 128);
		throw std::runtime_error("socket pair creation for server connection failed: "+std::string(errorBuffer));
	}

	ServerConnectionThreadInfo *infos = new ServerConnectionThreadInfo;
	infos->serverSocket = serverSocket;
	infos->signalSocket = sockets[1];
	infos->serverConnection = &(this->serverConnection);

	pthread_create(&serverConnectionThread, NULL, startServerConnection, static_cast<void*>(infos));
	
	this->signalSocket = sockets[0];
	signalChannel = g_io_channel_unix_new(this->signalSocket);
	signalWatcher = g_io_add_watch(signalChannel, (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR), signalCallback, this);
}
void* RendererController::startServerConnection(void *args)
{
	if(!args) { return NULL; }
	
	ServerConnectionThreadInfo *infos = static_cast<ServerConnectionThreadInfo*>(args);

	*(infos->serverConnection) = new ServerConnection(infos->serverSocket, infos->signalSocket);

	ev::default_loop().loop();
}
gboolean RendererController::signalCallback(GIOChannel *channel, GIOCondition condition, gpointer data)
{
	RendererController *me = static_cast<RendererController*>(data);

	if(condition == G_IO_HUP || condition == G_IO_ERR)
	{
		me->signalSocketClosed();
	}
	else if(condition == G_IO_IN)
	{
		gchar *readString;
		gsize readStringLength;

		GIOStatus result = g_io_channel_read_to_end(channel, &readString, &readStringLength, NULL);
		if(result == G_IO_STATUS_NORMAL)
		{
			me->handlePackagesFromServerConnection();
		}
		else if(result == G_IO_STATUS_ERROR)
		{		       
			me->signalSocketClosed();
		}
	}
}
void RendererController::handlePackagesFromServerConnection()
{
	Package *package = NULL;
	while((package = serverConnection->popPackage()) != NULL)
	{
		handlePackage(package);
	}
}
void RendererController::sendSignal()
{
	// note: it doesn't matter what we send here, the only information necessary is that something was send
	// 42 is just chosen as payload for "obvious" reasons ;-)
	int payload = 42;
	write(signalSocket, static_cast<void*>(&payload), sizeof(int));
}
void RendererController::sendPackageAndDelete(Package *thePackage)
{
	serverConnection->pushPackage(thePackage);
	sendSignal();
}

void RendererController::handlePackage(Package *thePackage)
{
}
void RendererController::signalSocketClosed()
{
	gtk_main_quit();
}
