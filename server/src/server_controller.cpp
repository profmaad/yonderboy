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

# include "ev_cpp.h"

# include "macros.h"
# include "log.h"

# include "controller_listener.h"
# include "hosts_manager.h"
# include "file_persistence_manager.h"
# include "persistent_storage.h"

# include "server_controller.h"

ServerController::ServerController() : sigintWatcher(NULL), controllerListener(NULL), state(ServerStateUninitialized)
{
	logLevel = LogLevelDebug;
	state = ServerStateInitializing;
	
	sigintWatcher = new ev::sig();
	sigintWatcher->set(SIGINT);
	sigintWatcher->set <ServerController, &ServerController::sigintCallback>(this);

	controllerListener = new ControllerListener("/tmp/cli-browser.ctl"); /*HC*/
																			   
	state = ServerStateInitialized;
}
ServerController::~ServerController()
{
	state = ServerStateUninitialized;
	
	delete controllerListener;
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
	
	/*TEMP BEGIN*/
	FilePersistenceManager *filePersistenceManager = new FilePersistenceManager("/tmp/file-storage");
	PersistentListStorage *listStorage = filePersistenceManager->retrieveListStorage("testgroup","test");
	std::cout<<"record 1: "<<listStorage->retrieve(1, NULL)<<std::endl;
	filePersistenceManager->releaseStorage("testgroup", "test");
	
	PersistentKeyValueStorage *kvStorage = filePersistenceManager->retrieveKeyValueStorage("group2", "kvtest");
	kvStorage->store(std::make_pair("key", "value"));
	kvStorage->store(std::make_pair("i and i", "rastafarian navy"));
	filePersistenceManager->releaseStorage("group2", "kvtest");
	/*TEMP END*/
	
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
