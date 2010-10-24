//      controller_listener.cpp
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

# include <cerrno>
# include <cstring>
# include <unistd.h>

# include "ev_cpp.h"
# include "log.h"
# include "controller_host.h"
# include "hosts_manager.h"

# include "controller_listener.h"

ControllerListener::ControllerListener(std::string socketPath) : IPCListener(socketPath)
{
}
ControllerListener::~ControllerListener()
{
}

void ControllerListener::callback(ev::io &watcher, int revents)
{
	int clientSocket = -1;
	ControllerHost *host = NULL;

	try
	{
		clientSocket = acceptClient();
		host = new ControllerHost(clientSocket);

		server->hostsManagerInstance()->registerHost(host);
	}
	catch(std::runtime_error e)
	{
		delete host;
		LOG_ERROR("failed to accept controller connection: "<<e.what())
	}
}
