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

# include <utility>
# include <string>
# include <vector>

/*TEMP*/
# include <iostream>

# include "ev_cpp.h"

# include "macros.h"
# include "log.h"

# include "controller_listener.h"
# include "configuration_manager.h"
# include "hosts_manager.h"
# include "file_persistence_manager.h"
# include "persistent_storage.h"

# include "server_controller.h"

ServerController::ServerController() : sigintWatcher(NULL), controllerListener(NULL), state(ServerStateUninitialized)
{
	logLevel = DEFAULT_LOG_LEVEL;
	state = ServerStateInitializing;
	
	sigintWatcher = new ev::sig();
	sigintWatcher->set(SIGINT);
	sigintWatcher->set <ServerController, &ServerController::sigintCallback>(this);
	
	configurationManager = new ConfigurationManager("/tmp/cli-browser.conf"); /*HC*/
	controllerListener = new ControllerListener("/tmp/cli-browser.ctl"); /*HC*/
	
	logLevel = configurationManager->retrieveAsLogLevel("server", "loglevel", LogLevelWarning);
	
	std::clog<<"loglevel: "<<logLevel<<std::endl;
																			   
	state = ServerStateInitialized;
}
ServerController::~ServerController()
{
	state = ServerStateUninitialized;
	
	delete controllerListener;
	delete configurationManager;
	delete sigintWatcher;
}

void ServerController::sigintCallback(ev::sig &watcher, int revents)
{
	LOG_INFO("received SIGINT, going down")
	
	stop();

	ev::get_default_loop().unloop(ev::ALL);
}

void ServerController::start()
{
	state = ServerStateStarting;
	
	controllerListener->startListening(2); /*HC*/

	sigintWatcher->start();
	
	state = ServerStateRunning;
}
void ServerController::stop()
{
	state = ServerStateShuttingDown;
	
	controllerListener->closeSocket();

	sigintWatcher->stop();

	HostsManager::deleteInstance();
	
	state = ServerStateInitialized;
}

bool ServerController::allowedToBlock()
{
	return (state == ServerStateInitializing || state == ServerStateShuttingDown || state == ServerStateStarting);
}
