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
# include <cstring>
# include <cstdlib>
# include <cerrno>

# include "ev_cpp.h"

# include "macros.h"
# include "log.h"

# include "controller_listener.h"
# include "viewer_listener.h"
# include "configuration_manager.h"
# include "display_manager.h"
# include "hosts_manager.h"
# include "job_manager.h"
# include "job.h"
# include "meta_decision_maker.h"
# include "file_persistence_manager.h"
# include "persistent_storage.h"
# include "package_router.h"

# include "server_controller.h"

ServerController::ServerController(std::string configFile) : signalPipeWatcher(NULL), signalPipe(-1), controllerListener(NULL), viewerListener(NULL), configurationManager(NULL), state(ServerStateUninitialized), displayManager(NULL), hostsManager(NULL), metaDecisionMaker(NULL), jobManager(NULL), packageRouter(NULL)
{
	state = ServerStateInitializing;
	
	setupSignalWatching();
		
	configFilePath = configFile;
	configurationManager = new ConfigurationManager(configFilePath);

	displayManager = new DisplayManager();
	hostsManager = new HostsManager();
	jobManager = new JobManager();
	packageRouter = new PackageRouter(configurationManager->retrieve("general", "net-spec", "net-spec.yml"));

	metaDecisionMaker = new MetaDecisionMaker();
	
	controllerListener = new ControllerListener(configurationManager->retrieve("general", "controller-socket", "controller.sock"));
	viewerListener = new ViewerListener(configurationManager->retrieve("general", "viewer-socket", "viewer.sock"));
	
	state = ServerStateInitialized;
}
ServerController::~ServerController()
{
	state = ServerStateUninitialized;

	delete hostsManager;
	delete controllerListener;
	delete viewerListener;
	delete displayManager;
	delete jobManager;
	delete packageRouter;
	delete metaDecisionMaker;
	delete configurationManager;
	delete signalPipeWatcher;
}

void ServerController::signalPipeCallback(ev::io &watcher, int revents)
{
	int signal = -1;
	ssize_t bytesRead = -1;	
	
	bytesRead = read(signalPipe, static_cast<void*>(&signal), sizeof(int));
	if(bytesRead != sizeof(int)) { signal = -1; }
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
void* ServerController::waitForSignals(void *arg)
{
	if(!arg) { return NULL; }
	
	int result = -1;
	int signal = -1;
	char errorBuffer[128] = { '\0' };
	
	SignalThreadInfo *infos = static_cast<SignalThreadInfo*>(arg);
	
	result = sigwait(&infos->signals, &signal);
	if(result != 0)
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
	viewerListener->startListening(2); /*HC*/
	
	state = ServerStateRunning;
}
void ServerController::stop()
{
	state = ServerStateShuttingDown;
	
	controllerListener->closeSocket();
	viewerListener->closeSocket();

	state = ServerStateInitialized;
}

void ServerController::doJob(Job *theJob)
{
	if(theJob->getValue("command") == "quit-server")
	{
		jobManagerInstance()->jobDone(theJob);
		stop();
		
		ev::get_default_loop().unloop(ev::ALL);
	}
}

bool ServerController::allowedToBlock()
{
	return (state == ServerStateInitializing || state == ServerStateShuttingDown || state == ServerStateStarting);
}
