//      main.cpp
//      
//      Copyright 2010 Prof. MAAD <prof.maad@lambda-bb.de>
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

# include <sys/types.h>
# include <sys/socket.h>
# include <sys/un.h>
# include <unistd.h>
# include <cstdlib>
# include <cstring>
# include <cerrno>
# include <getopt.h>

# include "shared.h"
# include "defaults.h"
# include "config.h"

# include <ev_cpp.h>

# include <configuration_reader.h>
# include "controller.h"

Controller *controllerInstance = NULL;
ConfigurationReader *configuration = NULL;

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
void printHelpMessage(const char *executable)
{
	printVersion(true);
	std::cout<<std::endl;
	
	std::cout<<"usage: "<<executable<<" [-h/--help] [-v/--version] <config file>"<<std::endl;
	
	std::cout<<"options:"<<std::endl;
	std::cout<<" -h/--help\t\tshow this help and exit"<<std::endl;
	std::cout<<" -v/--version\t\tshow version and exit"<<std::endl;
}

int main(int argc, char** argv)
{
	int serverSocket = -1;
	char *configFile = NULL;
	const char *socketPath = NULL;
	struct sockaddr_un socketAddress;
	char errorBuffer[128] = {'\0'};

	int option = -1;
	struct option longOptions[] = {
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'v'},
		{NULL, 0, NULL, 0}
	};
	
	opterr = 0;
	while( (option = getopt_long(argc, argv, ":hv", longOptions, NULL)) != -1 )
	{
		switch(option)
		{
			case '?':
				std::cerr<<"unknown option '"<<optopt<<"' encountered, ignoring"<<std::endl;
				break;
			case ':':
				std::cerr<<"missing argument for option '"<<optopt<<"'"<<std::endl;
				printHelpMessage(argv[0]);
				exit(1);
				break;
			case 'v':
				printVersion(false);
				exit(0);
				break;
			case 'h':
			default:
				printHelpMessage(argv[0]);
				exit(0);
		}
	}

	if(argc < 2)
	{
		printHelpMessage(argv[0]);
		return 1;
	}
	else
	{
		configFile = argv[1];
	}

	configuration = new ConfigurationReader(std::string(configFile));
	if(!configuration || !configuration->isReady())
	{
		std::cerr<<"failed to read config"<<std::endl;
		return 1;
	}

	socketPath = configuration->retrieveAsAbsolutePath("general", "controller-socket").c_str();

	if(strlen(socketPath) > 107)
	{
		std::cerr<<"socket path longer then 107 characters, that is not allowed"<<std::endl;
		return 1;
	}

	// open connection to server
	memset(&socketAddress, 0, sizeof(sockaddr_un));
	// first, create socket
	serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
	if(serverSocket < 0)
	{
		# ifdef STRERROR_R_CHAR_P
		std::cerr<<"failed to create socket: "<<strerror_r(errno,errorBuffer,128)<<std::endl;
		# else
		strerror_r(errno, errorBuffer, 128);
		std::cerr<<"failed to create socket: "<<errorBuffer<<std::endl;
		# endif
		return 1;
	}

	// second, connect to the server
	socketAddress.sun_family = AF_UNIX;
	strncpy(socketAddress.sun_path, socketPath, 107);
	socketAddress.sun_path[107] = '\0';

	if(connect(serverSocket, (sockaddr*)&socketAddress, sizeof(sockaddr_un)) < 0)
	{
		# ifdef STRERROR_R_CHAR_P
		std::cerr<<"failed to connect to server: "<<strerror_r(errno,errorBuffer,128)<<std::endl;
		# else
		strerror_r(errno, errorBuffer, 128);
		std::cerr<<"failed to connect to server: "<<errorBuffer<<std::endl;
		# endif
		close(serverSocket);
		return 1;
	}

	controllerInstance = new Controller(serverSocket);

	ev::default_loop().loop();

	delete controllerInstance;
	delete configuration;

	return 0;
}
