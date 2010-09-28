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
# include <iomanip>
# include <streambuf>
# include <fstream>
# include <stdexcept>

# include <cerrno>
# include <unistd.h>
# include <cstdlib>
# include <fcntl.h>
# include <cstring>
# include <getopt.h>

# include "config.h"
# include "defaults.h"

# include "server_controller.h"
# include "configuration_manager.h"
# include "configuration_finder.h"
# include "macros.h"

ServerController *server = NULL;
LogLevel logLevel = DEFAULT_LOG_LEVEL;

void switchFileDescriptorToDevNull(int fileDescriptor, int mode);
void changeToWorkingDirectory(const char* workingDir);
void printHelpMessage(const char *executable);
void printVersion(bool onOneLine = false);

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
		LOG_FATAL("failed to chdir to working directory "<<workingDir<<", going down ("<<errorBuffer<<")");
		exit(EXIT_FAILURE);
	}
}

void printHelpMessage(const char *executable)
{
	printVersion(true);
	std::cout<<std::endl;
	
	std::cout<<"usage: "<<executable<<" [-h/--help] [-v/--version] [-c/--config <file>]"<<std::endl;
	
	std::cout<<"options:"<<std::endl;
	std::cout<<" -h/--help\t\tshow this help and exit"<<std::endl;
	std::cout<<" -v/--version\t\tshow version and exit"<<std::endl;
	std::cout<<" -c/--config <file>\tconfig file to use"<<std::endl;
}
void printVersion(bool onOneLine)
{
	if(onOneLine)
	{
		std::cout<<PROJECT_NAME<<"-"<<PROJECT_VERSION<<" ("<<__DATE__<<" "<<__TIME__<<") - "<<PROJECT_BRIEF_DESCRIPTION<<std::endl;
	}
	else
	{
		std::cout<<PROJECT_NAME<<"-"<<PROJECT_VERSION<<" - "<<PROJECT_BRIEF_DESCRIPTION;
		std::cout<<std::endl;
		std::cout<<"Build Date: "<<__DATE__<<" "<<__TIME__<<std::endl;
	}
}

pid_t daemonize()
{
	int result = -1;
	char errorBuffer[128] = { '\0' };
	pid_t daemonPid = -1;
	
	daemonPid = fork();
	if(daemonPid < 0) // error
	{
		strerror_r(errno, errorBuffer, 128);
		LOG_ERROR("failed to fork daemon process, continuing in foreground ("<<errorBuffer<<")")
		return daemonPid;
	}
	else if(daemonPid == 0) // child
	{
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
	
	return getpid();
}

int main(int argc, char** argv)
{
	// extract command line options
	std::string configFilePath;
	int option = -1;
	struct option longOptions[] = {
		{"config", required_argument, NULL, 'c'},
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'v'},
		{NULL, 0, NULL, 0}
	};
	
	opterr = 0;
	while( (option = getopt_long(argc, argv, ":c:hv", longOptions, NULL)) != -1 )
	{
		switch(option)
		{
			case '?':
				LOG_WARNING("unknown option '"<<optopt<<"' encountered, ignoring")
				break;
			case ':':
				LOG_FATAL("missing argument for option '"<<optopt<<"'")
				printHelpMessage(argv[0]);
				exit(EXIT_FAILURE);
				break;
			case 'c':
				if(optarg) { configFilePath = std::string(optarg); }
				break;
			case 'v':
				printVersion();
				exit(EXIT_SUCCESS);
				break;
			case 'h':
			default:
				printHelpMessage(argv[0]);
				exit(EXIT_SUCCESS);
		}
	}
	if(configFilePath.empty())
	{
		try
		{
			configFilePath = findConfigurationFile();
		}
		catch(std::runtime_error &e)
		{
			std::cerr<<"Error occured: "<<e.what()<<std::endl;
			exit(EXIT_FAILURE);
		}
	}
	
	std::streambuf * const originalCLogStreambuf = std::clog.rdbuf();
	std::ofstream *logfileStream = NULL;
	pid_t daemonPid = -1;
	
	// values extracted from configuration
	bool forkDaemon = false;
	const char *workingDir = NULL;
	const char *logfilePath = NULL;
	
	ConfigurationManager *configurationManager = new ConfigurationManager(configFilePath);
		forkDaemon = configurationManager->retrieveAsBool("server", "daemonize", false);
		if(configurationManager->isSet("general", "working-dir")) { workingDir = configurationManager->retrieveAsPath("general", "working-dir", "~/.config/yonderboy/").c_str(); } //HC
		if(configurationManager->isSet("server", "logfile")) { logfilePath = configurationManager->retrieve("server", "logfile", "server.log").c_str(); }
		if(configurationManager->isSet("server", "loglevel")) { logLevel = configurationManager->retrieveAsLogLevel("server", "loglevel", DEFAULT_LOG_LEVEL); }
	delete configurationManager;
	
	if(forkDaemon)
	{
		daemonPid = daemonize();
	}
	if(workingDir)
	{
		changeToWorkingDirectory(workingDir);
	}
	
	if(logfilePath)
	{
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
	
	LOG_INFO("successfully forked into background with pid "<<daemonPid);
	
	server = new ServerController(configFilePath);

	server->start();

	ev::default_loop().loop();
	
	delete server;

	std::clog<<std::flush;
	std::clog.rdbuf(originalCLogStreambuf);
	if(logfileStream)
	{
		logfileStream->close();
		delete logfileStream;
	}

	return 0;
}
