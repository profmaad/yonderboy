//      client_controller.h
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

# ifndef CLIENT_CONTROLLER_H
# define CLIENT_CONTROLLER_H

# include <string>

# include <pthread.h>

class ServerConnection;
class Package;

class ClientController
{
public:
	ClientController(int socket);
	~ClientController();

protected:
	void closeServerConnection();
	void sendPackageAndDelete(Package *thePackage);	
	std::string getNextPackageID();

	virtual void handlePackage(Package *thePackage) = 0;
	virtual void signalSocketClosed() = 0;

private:
	void setupServerConnection(int serverSocket);
	static void* startServerConnection(void *args);
	void sendSignal();
	static gboolean signalCallback(GIOChannel *channel, GIOCondition condition, gpointer data);

	void handlePackagesFromServerConnection();

	ServerConnection *serverConnection;
	GIOChannel *signalChannel;
	guint signalWatcher;
	int signalSocket;
	pthread_t serverConnectionThread;

	struct ServerConnectionThreadInfo
	{
		int serverSocket;
		int signalSocket;
		ServerConnection **serverConnection;
	};
};

# endif
