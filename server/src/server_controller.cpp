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
# include <iostream>

# include <csignal>
# include <unistd.h>
# include <cerrno>

# include "ev_cpp.h"

# include "macros.h"
# include "log.h"

# include "controller_listener.h"
# include "configuration_manager.h"
# include "hosts_manager.h"
# include "file_persistence_manager.h"
# include "persistent_storage.h"

# include "server_controller.h"

ServerController::ServerController(const char *configFile) : signalPipeWatcher(NULL), signalPipe(-1), controllerListener(NULL), configurationManager(NULL), state(ServerStateUninitialized)
{
	logLevel = DEFAULT_LOG_LEVEL;
	state = ServerStateInitializing;
	
	setupSignalWatching();
	
	
	configFilePath = std::string(configFile);
	configurationManager = new ConfigurationManager(configFilePath);
	
	controllerListener = new ControllerListener(configurationManager->retrieve("server", "controller-socket", "controller.sock"));
	
	logLevel = configurationManager->retrieveAsLogLevel("server", "loglevel", LogLevelWarning);
																			   
	state = ServerStateInitialized;
}
ServerController::~ServerController()
{
	state = ServerStateUninitialized;
	
	delete controllerListener;
	delete configurationManager;
	delete signalPipeWatcher;
}

void ServerController::signalPipeCallback(ev::io &watcher, int revents)
{
	int signal = -1;
	ssize_t bytesRead = -1;
	
	bytesRead = read(signalPipe, (void*)&signal, sizeof(int));
	if(bytesRead != sizeof(int))
	{
		signal = -1;
	}
	close(signalPipe);
	
	LOG_INFO("received signal "<<signal<<", going down")
	
	stop();

	ev::get_default_loop().unloop(ev::ALL);
}
void ServerController::setupSignalWatching()
{
	sigset_t blockSet;
	sigset_t watchSet;
	int pipeFDs[2] = { -1, -1 };
	char errorBuffer[128] = { '\0' };
	int result = -1;
	
	// block all signals - they are handled via sigwait in waitForSignals on signalThread
	sigfillset(&blockSet);
	result = pthread_sigmask(SIG_BLOCK, &blockSet, NULL);
	if(result < 0)
	{
		strerror_r(result, errorBuffer, 128);
		LOG_FATAL("failed to set signal mask, going down ("<<errorBuffer<<")");
		exit(EXIT_FAILURE);
	}
	
	// create a pipe for communication between this thread and the signalThread
	result = pipe(pipeFDs);
	if(result < 0)
	{
		strerror_r(result, errorBuffer, 128);
		LOG_FATAL("failed to create pipe, going down ("<<errorBuffer<<")");
		exit(EXIT_FAILURE);
	}
	signalPipe = pipeFDs[0];
	
	sigemptyset(&watchSet);
	sigaddset(&watchSet, SIGINT);
	sigaddset(&watchSet, SIGTERM);
	SignalThreadInfo *infos = new SignalThreadInfo;
		infos->pipeFD = pipeFDs[1];
		infos->signals = watchSet;
	
	signalPipeWatcher = new ev::io();
	signalPipeWatcher->set <ServerController, &ServerController::signalPipeCallback>(this);
	signalPipeWatcher->start(signalPipe, ev::READ);
	
	pthread_create(&signalThread, NULL, waitForSignals, static_cast<void*>(infos));
	pthread_detach(signalThread);
}
void* ServerController::waitForSignals(void* arg)
{
	if(!arg) { return NULL; }
	
	int result = -1;
	int signal = -1;
	char errorBuffer[128] = { '\0' };
	
	SignalThreadInfo *infos = static_cast<SignalThreadInfo*>(arg);
	
	result = sigwait(&infos->signals, &signal);
	if(result < 0)
	{
		strerror_r(result, errorBuffer, 128);
		LOG_FATAL("failed to wait for signals, going down ("<<errorBuffer<<")")
		
		exit(EXIT_FAILURE);
	}
	
	write(infos->pipeFD, static_cast<void*>(&signal), sizeof(int));
	close(infos->pipeFD);
	
	delete infos;
	
	return NULL;
}

void ServerController::start()
{
	state = ServerStateStarting;
	
	controllerListener->startListening(2); /*HC*/
	
	state = ServerStateRunning;
}
void ServerController::stop()
{
	state = ServerStateShuttingDown;
	
	controllerListener->closeSocket();

	HostsManager::deleteInstance();
	
	state = ServerStateInitialized;
}

bool ServerController::allowedToBlock()
{
	return (state == ServerStateInitializing || state == ServerStateShuttingDown || state == ServerStateStarting);
}
