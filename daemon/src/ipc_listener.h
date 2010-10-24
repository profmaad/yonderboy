//      ipc_listener.h
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

# ifndef IPC_LISTENER_H
# define IPC_LISTENER_H

# include "macros.h"
# include "ev_cpp.h"

# include <string>

class IPCListener
{
public:
	IPCListener(std::string socketPath);
	virtual ~IPCListener();

	void startListening(int backlog);
	void closeSocket();
	int acceptClient();

	ConnectionState getState() { return state; }

protected:
	virtual void callback(ev::io &watcher, int revents) = 0;

	std::string socketPath;
	int ipcSocket;
	ConnectionState state;
	ev::io *watcher;
};

# endif /*IPC_LISTENER_H*/
