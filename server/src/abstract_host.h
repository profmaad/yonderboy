//      abstract_host.h
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

# ifndef ABSTRACT_HOST_H
# define ABSTRACT_HOST_H

# include <map>
# include <string>

# include "macros.h"
# include "ev_cpp.h"

class Package;

class AbstractHost
{
public:
	AbstractHost(int hostSocket);
	virtual ~AbstractHost();

	void disconnect();
	void shutdownHost();
	ConnectionState getState() { return state; }

	void sendPackage(Package *thePackage);

protected:
	virtual void handlePackage(Package *thePackage) = 0;

	ConnectionState state;

private:
	void closeSocket(); // only safe to call when we either already did a proper shutdown (thats what disconnect() is for) or we are just responding to the other side shuting down
	void shutdownSocket(); // shuts down the write half of the socket to signal the other party that we are going down

	void readCallback(ev::io &watcher, int revents);
	void parseReceivedData();
	void processPackage();

	void writeCallback(ev::io &watcher, int revents);

	int hostSocket;

	char receiveBuffer[4096];
	std::string parseBuffer;
	bool receivedNewline;

	std::string sendBuffer;

	ev::io *readWatcher;
	ev::io *writeWatcher;
};

#endif /*ABSTRACT_HOST_H*/ 
