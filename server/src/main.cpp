//      main.cpp
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
# include <streambuf>
# include <fstream>

# include <cerrno>
# include <unistd.h>

# include "config.h"

# include "server_controller.h"
# include "configuration_manager.h"

ServerController *server = NULL;

void switchFileDescriptorToDevNull(int fileDescriptor, int mode)
{
	int newFileDescriptor = -1;
	
	newFileDescriptor = open("/dev/null", mode);
	if(newFileDescriptor >= 0)
	{
		close(fileDescriptor);
		dup2(newFileDescriptor, fileDescriptor);
		close(newFileDescriptor);
	}
}
void changeToWorkingDirectory(const char* workingDir)
{
	int result = -1;
	char errorBuffer[128] = { '\0' };
	
	result = chdir(workingDir);
	if(result < 0)
	{
		strerror_r(errno, errorBuffer, 128);
		LOG_FATAL("failed to chdir to working directory "<<workingDir<<", going down ("<<errorBuffer<<")")
		exit(EXIT_FAILURE);
	}
}
void daemonize()
{
	int result = -1;
	char errorBuffer[128] = { '\0' };
	pid_t daemonPid = -1;
	
	daemonPid = fork();
	if(daemonPid < 0) // error
	{
		strerror_r(errno, errorBuffer, 128);
		LOG_ERROR("failed to fork daemon process, continuing in foreground ("<<errorBuffer<<")")
		return;
	}
	else if(daemonPid == 0) // child
	{
		LOG_INFO("successfully forked into background")
	}
	else if(daemonPid > 0) // parent
	{
		LOG_INFO("forked daemon with pid "<<daemonPid)
		exit(EXIT_SUCCESS);
	}
	
	result = setsid();
	if(result < 0)
	{
		strerror_r(errno, errorBuffer, 128);
		LOG_FATAL("failed to acquire new process group, going down ("<<errorBuffer<<")")
		exit(EXIT_FAILURE);
	}
	
	switchFileDescriptorToDevNull(STDIN_FILENO, O_RDONLY);
	switchFileDescriptorToDevNull(STDOUT_FILENO, O_WRONLY);
	switchFileDescriptorToDevNull(STDERR_FILENO, O_WRONLY);
}

int main(int argc, char** argv)
{
	std::streambuf * const originalCLogStreambuf = std::clog.rdbuf();
	std::ofstream *logfileStream = NULL;
	
	ConfigurationManager *configurationManager = new ConfigurationManager("/tmp/cli-browser.conf"); /*HC*/
	
	if(configurationManager->retrieveAsBool("server", "daemonize", false))
	{
		daemonize();
	}
	if(configurationManager->isSet("server", "working-dir"))
	{
		changeToWorkingDirectory(configurationManager->retrieve("server", "working-dir", "~/.cli-browser/").c_str());
	}
	
	if(configurationManager->isSet("server", "logfile"))
	{
		const char *logfilePath = configurationManager->retrieve("server", "logfile", "server.log").c_str();
		logfileStream = new std::ofstream(logfilePath, std::ios_base::out | std::ios_base::app);
		if(logfileStream->fail())
		{
			LOG_ERROR("failed to open log file at "<<logfilePath<<", continue logging to STDOUT");
			
			delete logfileStream;
			logfileStream = NULL;
		}
		else
		{
			std::clog.rdbuf(logfileStream->rdbuf());
		}
	}
	delete configurationManager;
	
	server = new ServerController;

	server->start();

	ev::default_loop().loop();
	
	delete server;
	
	std::clog.rdbuf(originalCLogStreambuf);
	if(logfileStream)
	{
		logfileStream->close();
		delete logfileStream;
	}

	return 0;
}
