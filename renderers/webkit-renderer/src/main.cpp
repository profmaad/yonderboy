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

# include <sys/socket.h>
# include <unistd.h>
# include <cstdlib>
# include <getopt.h>

# include "shared.h"
# include "defaults.h"
# include "ev_cpp.h"

# include "renderer_controller.h"

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
	
	std::cout<<"usage: "<<executable<<" [-h/--help] [-v/--version] <socket>"<<std::endl;
	
	std::cout<<"options:"<<std::endl;
	std::cout<<" -h/--help\t\tshow this help and exit"<<std::endl;
	std::cout<<" -v/--version\t\tshow version and exit"<<std::endl;
}

int main(int argc, char** argv)
{
	int socket = -1;

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
		socket = atoi(argv[1]);
	}

	if(socket < 0)
	{
		std::cerr<<"invalid socket given"<<std::endl;
		exit(1);
	}

	RendererController renderer(socket);

	ev::default_loop().loop();

	return 0;
}
