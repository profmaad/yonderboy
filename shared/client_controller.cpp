//      client_controller.cpp
//      
//      Copyright 2010 Prof. MAAD <prof.maad@lambda-bb.de>
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

# include "client_controller.h"

# include "ev_cpp.h"

# include "package.h"
# include "package_factories.h"
# include "server_connection.h"

ClientController::ClientController(int socket)
{
	setupServerConnection(socket);
}

ClientController::~ClientController()
{
	delete serverConnection;
	pthread_join(serverConnectionThread, NULL);
}

void ClientController::setupServerConnection(int serverSocket)
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
	g_io_channel_set_encoding(signalChannel, NULL, NULL);
	signalWatcher = g_io_add_watch(signalChannel, (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR), signalCallback, this);
}
void* ClientController::startServerConnection(void *args)
{
	if(!args) { return NULL; }
	
	ServerConnectionThreadInfo *infos = static_cast<ServerConnectionThreadInfo*>(args);

	*(infos->serverConnection) = new ServerConnection(infos->serverSocket, infos->signalSocket);

	ev::default_loop().loop();

	std::cerr<<"___CLIENTCONTROLLER__startServerConnection(): past loop()"<<std::endl;

	return NULL;
}
gboolean ClientController::signalCallback(GIOChannel *channel, GIOCondition condition, gpointer data)
{
	ClientController *me = static_cast<ClientController*>(data);

	if(condition == G_IO_HUP || condition == G_IO_ERR)
	{
		me->signalSocketClosed();
		return FALSE;
	}
	else if(condition == G_IO_IN)
	{
		gchar payload;
		gsize bytesRead = -1;
		GIOStatus result = G_IO_STATUS_ERROR;
		
		result = g_io_channel_read_chars(channel, &payload, sizeof(gchar), &bytesRead, NULL);
		
		if(result == G_IO_STATUS_NORMAL)
		{
			me->handlePackagesFromServerConnection();
		}
		else if(result == G_IO_STATUS_ERROR || bytesRead == 0)
		{
			me->signalSocketClosed();
			return FALSE;
		}
	}
	
	return TRUE;
}
void ClientController::handlePackagesFromServerConnection()
{
	Package *package = NULL;
	while((package = serverConnection->popPackage()) != NULL)
	{
		handlePackage(package);
	}
}
void ClientController::sendSignal()
{
	// note: it doesn't matter what we send here, the only information necessary is that something was send
	// 42 is just chosen as payload for "obvious" reasons ;-)
	int payload = 42;
	write(signalSocket, static_cast<void*>(&payload), sizeof(int));
}
void ClientController::sendPackageAndDelete(Package *thePackage)
{
	serverConnection->pushPackage(thePackage);
	sendSignal();
}
void ClientController::closeServerConnection()
{
	shutdown(signalSocket, SHUT_WR);
	close(signalSocket);
}

std::string ClientController::getNextPackageID()
{
	return serverConnection->getNextPackageID();
}
