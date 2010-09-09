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

Controller::Controller(int serverSocket) : AbstractHost(serverSocket), stdinWatcher(NULL), commands(NULL), lastSendPackageID(0), waitingForAck(false)
{
	std::cout<<std::endl;
	std::cout<<"      _ ._  _| _ ._ |_  _   "<<std::endl;
	std::cout<<"   \\/(_)| |(_|(/_|  |_)(_)\\/"<<std::endl;
	std::cout<<"   /                      / "<<std::endl;
	std::cout<<std::endl;

	commands = new std::map<std::string, CommandParser*>();
	parseSpecFile(NETSPEC_PATH); //HC

	// setup stdin read watcher and readline library
	rl_attempted_completion_function = &Controller::completionCallback;
	using_history();
	
	stdinWatcher = new ev::io();
	stdinWatcher->set<Controller, &Controller::stdinCallback>(this);

	Package *initPackage = constructPackage("connection-management", "id", getNextPackageID().c_str(), "command", "initialize", "client-name", PROJECT_NAME, "client-version", PROJECT_VERSION, "can-display-stati", "", "interactive", "", "can-handle-requests", "",  NULL);
	lastSendPackageID = initPackage->getID();
	waitingForAck = true;
	sendPackageAndDelete(initPackage);
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
	stopReadline();
	std::cout<<std::endl;

	ev::get_default_loop().unloop(ev::ALL);
}
void Controller::parseSpecFile(std::string file)
{
	std::string expandedFilename;
	if(file[0] == '~')
	{
		file.erase(0,1);
		expandedFilename = std::string(getenv("HOME"));
		expandedFilename += file;
	}
	else
	{
		expandedFilename = file;
	}

	std::ifstream specFile(expandedFilename.c_str());
	
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
	else
	{
		std::cerr<<"failed to read net-spec file from '"<<expandedFilename<<"'"<<std::endl;
		specFile.close();
		quit();
		return;
	}

	specFile.close();
}
void Controller::startReadline()
{
	rl_callback_handler_install(PROMPT, &Controller::readlineCallback);
	stdinWatcher->start(STDIN_FILENO, ev::READ);	
}
void Controller::stopReadline()
{
	rl_callback_handler_remove();
	stdinWatcher->stop();
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
			std::cerr<<"failed: "<<thePackage->getValue("error")<<std::endl;
		}
		if(waitingForAck && thePackage->getID() == lastSendPackageID)
		{
			waitingForAck = false;
			startReadline();
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
				lastSendPackageID = commandPackage->getID();
				sendPackageAndDelete(commandPackage);
			}
		}
		else
		{
			std::cerr<<"unknown command: "<<argv[0]<<std::endl;
		}
	}

	add_history(line);

	free(argv);
	free(line);
}

char** Controller::completionCallback(const char *text, int start, int end)
{
	return controllerInstance->generateCompletionMatches(text, start, end);
}
char** Controller::generateCompletionMatches(const char *text, int start, int end)
{
	char **matches = NULL;

	if(start == 0) // complete command
	{
		matches = rl_completion_matches(text, &Controller::commandCompletionGeneratorWrapper);
	}
	else
	{
		matches = rl_completion_matches(text, &Controller::parametersCompletionGeneratorWrapper);
	}

	rl_attempted_completion_over = 1;

	return matches;
}
char* Controller::commandCompletionGeneratorWrapper(const char *text, int state)
{
	return controllerInstance->commandCompletionGenerator(text, state);
}
char* Controller::commandCompletionGenerator(const char *text, int state)
{
	if(state == 0)
	{
		commandCompletionIterator = commands->begin();
	}

	while(commandCompletionIterator != commands->end())
	{
		std::string command = commandCompletionIterator->first;
		commandCompletionIterator++;

		if(command.compare(0, strlen(text), text) == 0)
		{
			return strdup(command.c_str());
		}
	}

	return NULL;
}
char* Controller::parametersCompletionGeneratorWrapper(const char *text, int state)
{
	return controllerInstance->parametersCompletionGenerator(text, state);
}
char* Controller::parametersCompletionGenerator(const char *text, int state)
{
	// first we need to know which command is currently entered and get the appropriate CommandParser
	int argc = 0;
	const char **argv = NULL;
	CommandParser *parser = NULL;
	int result = -1;

	result = poptParseArgvString(rl_line_buffer, &argc, &argv);
	if(result < 0) { return NULL; } // line not parseable, so we can't do anything useful
	else if(argc < 1) { free(argv); return NULL; } // no command entered, same problem

	parser = retrieveCommandParser(std::string(argv[0]));
	free(argv);
	if(!parser) { return NULL; }

	return parser->completionGenerator(text, state);
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
