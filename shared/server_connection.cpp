//      server_connection.cpp
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

# include <queue>
# include <iostream>

# include <pthread.h>
# include <unistd.h>

# include "ev_cpp.h"
# include "package.h"

# include "server_connection.h"

ServerConnection::ServerConnection(int serverSocket, int signalSocket) : AbstractHost(serverSocket), serverSocket(-1), signalSocket(-1)
{
	this->serverSocket = serverSocket;
	this->signalSocket = signalSocket;

	receivedPackages = new std::queue<Package*>();
	toSendPackages = new std::queue<Package*>();
	
	receivedPackagesMutex = PTHREAD_MUTEX_INITIALIZER;
	toSendPackagesMutex = PTHREAD_MUTEX_INITIALIZER;

	setupSignalSocket();
}
ServerConnection::~ServerConnection()
{
	pthread_mutex_destroy(&receivedPackagesMutex);
	pthread_mutex_destroy(&toSendPackagesMutex);

	if(ev::get_default_loop().depth() > 0)
	{
		signalWatcher->stop();
		delete signalWatcher;
	}
}
void ServerConnection::setupSignalSocket()
{
	signalWatcher = new ev::io();
	signalWatcher->set<ServerConnection, &ServerConnection::signalCallback>(this);
	signalWatcher->start(signalSocket, ev::READ);
}

Package* ServerConnection::popPackage()
{
	Package *returnValue = NULL;

	pthread_mutex_lock(&receivedPackagesMutex);
	
	if(!receivedPackages->empty())
	{
		returnValue = receivedPackages->front();
		receivedPackages->pop();
	}

	pthread_mutex_unlock(&receivedPackagesMutex);

	return returnValue;
}
void ServerConnection::pushPackage(Package *thePackage)
{
	if(!thePackage) { return; }

	pthread_mutex_lock(&toSendPackagesMutex);

	toSendPackages->push(thePackage);

	pthread_mutex_unlock(&toSendPackagesMutex);
}

void ServerConnection::handlePackage(Package *thePackage)
{
	pthread_mutex_lock(&receivedPackagesMutex);

	receivedPackages->push(thePackage);

	pthread_mutex_unlock(&receivedPackagesMutex);

	sendSignal();
}
void ServerConnection::socketClosed()
{
	char errorBuffer[128] = { '\0' };
	int result = -1;

	result = shutdown(signalSocket, SHUT_WR);
	if(result < 0)
	{
		strerror_r(result, errorBuffer, 128);
		std::cerr<<"failed to close signal socket on ServerConnection side: "<<errorBuffer<<std::endl;
	}
}

void ServerConnection::signalCallback(ev::io &watcher, int revents)
{
	int payload = -1;
	ssize_t bytesRead = -1;

	bytesRead = read(signalSocket, static_cast<void*>(&payload), sizeof(int));

	if(bytesRead == 0)
	{
		if(state == Connected) { shutdownHost(); }
		close(signalSocket);
	}

	pthread_mutex_lock(&toSendPackagesMutex);

	while(!toSendPackages->empty())
	{
		sendPackageAndDelete(toSendPackages->front());
		toSendPackages->pop();
	}

	pthread_mutex_unlock(&toSendPackagesMutex);
}
void ServerConnection::sendSignal()
{
	// note: it doesn't matter what we send here, the only information necessary is that something was send
	// 23 is just chosen as payload for "obvious" reasons ;-)
	int payload = 23;
	write(signalSocket, static_cast<void*>(&payload), sizeof(int));
}
