//      server_controller.h
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

# ifndef SERVER_CONTROLLER_H
# define SERVER_CONTROLLER_H

# include <csignal>

# include <pthread.h>

# include "ev_cpp.h"

# include "macros.h"

class ControllerListener;
class ViewerListener;
class ConfigurationManager;

class ServerController
{
public:
	ServerController(const char *configFile);  // static initialization phase
	~ServerController();

	void start(); // runtime phase - once this is called, we are not supposed to quit until explicitely told so
	void stop(); // and this is what tells us to: stop running + static shutdown
	
	// queries
	bool allowedToBlock();
	ServerState getState() { return state; };
	LogLevel getLogLevel() { return logLevel; };

private:
	void signalPipeCallback(ev::io &watcher, int revents);
	static void* waitForSignals(void* arg);
	void setupSignalWatching();

	ControllerListener *controllerListener;
	ViewerListener *viewerListener;
	
	ConfigurationManager *configurationManager;
	std::string configFilePath;

	struct SignalThreadInfo
	{
		int pipeFD;
		sigset_t signals;
	};
	
	pthread_t signalThread;
	ev::io *signalPipeWatcher;
	int signalPipe;
	
	ServerState state;
	LogLevel logLevel;
};

# endif /*SERVER_CONTROLLER_H*/
