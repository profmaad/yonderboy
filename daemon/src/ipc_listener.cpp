//      ipc_listener.cpp
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

# include <iostream>
# include <string>
# include <stdexcept>

# include <sys/types.h>
# include <sys/socket.h>
# include <sys/un.h>
# include <cerrno>
# include <cstring>
# include <unistd.h>
# include <fcntl.h>

# include "ev_cpp.h"

# include "ipc_listener.h"

IPCListener::IPCListener(std::string path) : ipcSocket(-1), state(Uninitialized), watcher(NULL), socketPath(path)
{
	//sanity checks
	if(path.size() > 107) { throw std::length_error("Socket paths can be at most 107 characters long"); }

	int result = -1;
	char errorBuffer[128] = {'\0'};
	sockaddr_un listenerAddress;
	memset(&listenerAddress,0,sizeof(sockaddr_un));

	ipcSocket = socket(AF_UNIX, SOCK_STREAM, 0);
	if(ipcSocket < 0)
	{
		strerror_r(errno,errorBuffer,128);
		throw std::runtime_error("Failed to create IPC listener socket: "+std::string(errorBuffer));
	}
	// get current flags
	int fileFlags = fcntl(ipcSocket, F_GETFL, 0);
	if(fileFlags == -1)
	{
		close(ipcSocket);
		strerror_r(errno,errorBuffer,128);
		throw std::runtime_error("Failed to get/set socket flags: "+std::string(errorBuffer));
	}
	
	// if the socket is blocking, set it to nonblocking
	if(!(fileFlags & O_NONBLOCK))
	{
		fileFlags |= O_NONBLOCK;
		
		result = fcntl(ipcSocket, F_SETFL, fileFlags);
		if(result == -1)
		{
			close(ipcSocket);
			strerror_r(errno,errorBuffer,128);
			throw std::runtime_error("Failed to get/set socket flags: "+std::string(errorBuffer));
		}
	}

	listenerAddress.sun_family = AF_UNIX;
	strncpy(listenerAddress.sun_path, path.c_str(), 107);
	listenerAddress.sun_path[107] = '\0'; //yeah, we checked this already - but lets just be very sure

	unlink(listenerAddress.sun_path);
	result = bind(ipcSocket, (sockaddr*)&listenerAddress, sizeof(sockaddr_un));
	if(result < 0)
	{
		strerror_r(errno,errorBuffer,128);
		throw std::runtime_error("Failed to bind IPC listener socket to path "+path+": "+errorBuffer);
	}

	watcher = new ev::io();
	watcher->set<IPCListener,&IPCListener::callback>(this);

	state = Bound;
}
IPCListener::~IPCListener()
{
	closeSocket();
	delete watcher;
}

void IPCListener::startListening(int backlog)
{
	//sanity checks
	if(backlog < 0) { throw std::domain_error("Backlog has to be positive"); }

	int result = -1;
	char errorBuffer[128] = {'\0'};

	result = listen(ipcSocket,backlog);
	if(result < 0)
	{
		strerror_r(errno,errorBuffer,128);
		throw std::runtime_error("Failed to listen on IPC socket: "+std::string(errorBuffer));
	}

	watcher->start(ipcSocket,ev::READ);

	state = Listening;
}
void IPCListener::closeSocket()
{
	if(ev::get_default_loop().depth() > 0) { watcher->stop(); }
	close(ipcSocket);
	unlink(socketPath.c_str());
	state = Disconnected;
}
int IPCListener::acceptClient()
{
	int result = -1;
	int clientSocket = -1;
	int fileFlags = 0;
	char errorBuffer[128] = {'\0'};

	clientSocket = accept(ipcSocket, NULL, NULL);
	if(clientSocket < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK))
	{
		strerror_r(errno,errorBuffer,128);
		throw std::runtime_error("Failed to accept a client connection: "+std::string(errorBuffer));
	}
	
	// get current flags
	fileFlags = fcntl(clientSocket, F_GETFL, 0);
	if(fileFlags == -1)
	{
		close(clientSocket);
		strerror_r(errno,errorBuffer,128);
		throw std::runtime_error("Failed to get/set socket flags: "+std::string(errorBuffer));
	}

	// if the socket is blocking, set it to nonblocking
	if(!(fileFlags & O_NONBLOCK))
	{
		fileFlags |= O_NONBLOCK;

		result = fcntl(clientSocket, F_SETFL, fileFlags);
		if(result == -1)
		{
			close(clientSocket);
			strerror_r(errno,errorBuffer,128);
			throw std::runtime_error("Failed to get/set socket flags: "+std::string(errorBuffer));
		}
	}

	return clientSocket;
}

