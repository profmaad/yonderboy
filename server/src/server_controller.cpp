//      server_controller.cpp
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

# include "ev_cpp.h"

# include "controller_listener.h"
# include "hosts_manager.h"

# include "server_controller.h"

ServerController::ServerController() : sigintWatcher(NULL), controllerListener(NULL)
{
	sigintWatcher = new ev::sig();
	sigintWatcher->set(SIGINT);
	sigintWatcher->set <ServerController, &ServerController::sigintCallback>(this);

	controllerListener = new ControllerListener("/tmp/cli-browser.ctl"); /*HC*/
}
ServerController::~ServerController()
{
	delete controllerListener;
	delete sigintWatcher;
}

void ServerController::sigintCallback(ev::sig &watcher, int revents)
{
	std::cerr<<"received SIGINT, going down"<<std::endl;
	
	stop();

	ev::get_default_loop().unloop(ev::ALL);
}

void ServerController::start()
{
	controllerListener->startListening(2); /*HC*/

	sigintWatcher->start();
}
void ServerController::stop()
{
	controllerListener->closeSocket();

	sigintWatcher->stop();

	HostsManager::deleteInstance();
}
