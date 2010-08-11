//      server_connection.h
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

# ifndef SERVER_CONNECTION_H
# define SERVER_CONNECTION_H

# include <queue>

# include <pthread.h>

# include "ev_cpp.h"
# include "abstract_host.h"

class Package;

class ServerConnection : public AbstractHost
{
public:
	ServerConnection(int serverSocket, int signalSocket);
	~ServerConnection();
	
	Package* popPackage();
	void pushPackage(Package *thePackage);

protected:
	void handlePackage(Package *thePackage);
	void socketClosed();

private:
	void setupSignalSocket();
	void signalCallback(ev::io &watcher, int revents);
	void sendSignal();

	std::queue<Package*> *receivedPackages;
	std::queue<Package*> *toSendPackages;

	pthread_mutex_t receivedPackagesMutex;
	pthread_mutex_t toSendPackagesMutex;

	int serverSocket;
	int signalSocket;

	ev::io *signalWatcher;
};

# endif
