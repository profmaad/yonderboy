//      controller.cpp
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

# include <string>
# include <map>
# include <utility>
# include <iostream>
# include <fstream>

# include <cstring>
# include <cstdlib>
# include <unistd.h>

# include <readline/readline.h>
# include <readline/history.h>
# include <popt.h>
# include <yaml.h>

# include <package.h>
# include <package_factories.h>
# include <ev_cpp.h>

# include "defaults.h"
# include "command_parser.h"

# include "controller.h"

extern Controller *controllerInstance;

Controller::Controller(int serverSocket) : AbstractHost(serverSocket), stdinWatcher(NULL), commands(NULL)
{
	commands = new std::map<std::string, CommandParser*>();
	parseSpecFile("/home/profmaad/.cli-browser/net-spec.yml"); //HC

	// setup stdin read watcher and readline library
	rl_callback_handler_install("yonderboy> ", &Controller::readlineCallback); //HC
	
	stdinWatcher = new ev::io();
	stdinWatcher->set<Controller, &Controller::stdinCallback>(this);
	stdinWatcher->start(STDIN_FILENO, ev::READ);

	sendPackageAndDelete(constructPackage("connection-management", "id", getNextPackageID().c_str(), "command", "initialize", "client-name", PROJECT_NAME, "client-version", PROJECT_VERSION, "can-display-stati", "", "interactive", "", "can-handle-requests", "",  NULL));
}
Controller::~Controller()
{
	rl_callback_handler_remove();

	delete stdinWatcher;

	for(std::map<std::string, CommandParser*>::iterator iter = commands->begin(); iter != commands->end(); iter++)
	{
		delete iter->second;
	}
	delete commands;
}
void Controller::quit()
{
	stdinWatcher->stop();

	ev::get_default_loop().unloop(ev::ALL);
}
void Controller::parseSpecFile(std::string file)
{
	std::ifstream specFile(file.c_str());
	
	if(!specFile.fail())
	{
		YAML::Parser specParser(specFile);
		YAML::Node specDoc;
		specParser.GetNextDocument(specDoc);
		
		for(int i=0; i<specDoc["commands"].size(); i++)
		{
			if(specDoc["commands"][i]["command"].GetType() == YAML::CT_SCALAR && specDoc["commands"][i]["source"].GetType() == YAML::CT_SEQUENCE)
			{
				std::string command;
				specDoc["commands"][i]["command"] >> command;
				
				// lets check whether this command is for us
				for(int s=0; s<specDoc["commands"][i]["source"].size(); s++)
				{
					std::string source;
					specDoc["commands"][i]["source"][s] >> source;
					
					if(source == "controller")
					{
						CommandParser *parser = new CommandParser(specDoc["commands"][i]);
						commands->insert(std::make_pair(command, parser));
					}
				}
			}
		}
	}		

	specFile.close();
}

void Controller::handlePackage(Package *thePackage)
{
	std::string error;

	switch(thePackage->getType())
	{
	case Command:
		error = handleCommand(thePackage);
		sendPackageAndDelete(constructAcknowledgementPackage(thePackage, error));
		break;
	case StatusChange:
		handleStatusChange(thePackage);
		break;
	case Acknowledgement:
		if(thePackage->hasValue("error"))
		{
			std::cerr<<"got negative ack for "<<thePackage->getID()<<": "<<thePackage->getValue("error")<<std::endl;
		}
		break;
	case ConnectionManagement:
		break;
	}

	delete thePackage;
}
std::string Controller::handleCommand(Package *thePackage)
{
	std::string command = thePackage->getValue("command");
	std::string error;

	std::cerr<<"got command: "<<command<<std::endl;
}
void Controller::handleStatusChange(Package *thePackage)
{
	std::string status = thePackage->getValue("status");

	std::cout<<"status change > "<<status<<std::endl;
}
void Controller::socketClosed()
{
	std::cerr<<"connection closed by server"<<std::endl;

	quit();
}

void Controller::stdinCallback(ev::io &watcher, int revents)
{
	rl_callback_read_char();
}

void Controller::readlineCallback(char *line)
{
	controllerInstance->handleLine(line);
}
void Controller::handleLine(char *line)
{
	if(line == NULL) // we got an EOF, that means we should quit
	{
		quit();
		return;
	}
	if(strlen(line) == 0) { return; }

	int argc = 0;
	const char **argv = NULL;
	int result = -1;

	// parse line into argv structure
	result = poptParseArgvString(line, &argc, &argv);
	if(result < 0)
	{
		std::cerr<<"error parsing line: "<<poptStrerror(result);
		return;
	}
	if(argc > 0)
	{
		CommandParser *parser = retrieveCommandParser(std::string(argv[0]));
		if(parser)
		{
			Package *commandPackage = parser->constructPackageFromLine(argc,argv, getNextPackageID());
			if(commandPackage)
			{
				sendPackageAndDelete(commandPackage);
			}
		}
		else
		{
			std::cerr<<"unknown command: "<<argv[0]<<std::endl;
		}
	}

	free(argv);
	free(line);
}

CommandParser* Controller::retrieveCommandParser(std::string command)
{
	std::map<std::string, CommandParser*>::iterator iter = commands->find(command);
	if(iter != commands->end())
	{
		return iter->second;
	}

	return NULL;
}
