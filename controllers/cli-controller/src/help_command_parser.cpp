//      help_command_parser.cpp
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
# include <iostream>
# include <iomanip>

# include <cstring>

# include "command_parser.h"

# include "help_command_parser.h"

HelpCommandParser::HelpCommandParser(std::map<std::string, CommandParser*> *commands, unsigned int maxCommandNameLength) : CommandParser()
{
	this->commands = commands;
	this->maxCommandNameLength = maxCommandNameLength;

	this->command = "help";
	this->description = "get help on the commands available";
	this->valid = true;

	requiredParameter commandParameter;
	commandParameter.name = "command";
	commandParameter.description = "the command to get help on";
	parameters->push_back(commandParameter);
}

Package* HelpCommandParser::constructPackageFromLine(int argc, const char **argv, std::string packageID)
{
	if(argc == 1)
	{
		std::cout<<"Possible commands are:"<<std::endl;
		for(std::map<std::string, CommandParser*>::iterator iter = commands->begin(); iter != commands->end(); iter++)
		{
			if(iter->second)
			{
				std::cout<<"\t"<<std::left<<std::setw(maxCommandNameLength)<<iter->second->getCommand()<<"\t"<<std::setw(0)<<iter->second->getDescription()<<std::endl;
			}
		}
	}
	else if(argc > 1)
	{
		CommandParser *parser = retrieveCommandParser(std::string(argv[1]));
		if(parser)
		{
			parser->printParameterHelp();
		}
		else
		{
			std::cout<<"help: the specified command doesn't exist"<<std::endl;
		}
	}
	return NULL;
}
char* HelpCommandParser::completionGenerator(const char *text, int state)
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
			return strndup(command.c_str(), command.size());
		}
	}

	return NULL;

}

CommandParser* HelpCommandParser::retrieveCommandParser(std::string command)
{
	std::map<std::string, CommandParser*>::iterator iter = commands->find(command);
	if(iter != commands->end())
	{
		return iter->second;
	}

	return NULL;
}
