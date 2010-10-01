//      help_command_parser.h
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

# ifndef HELP_COMMAND_PARSER_H
# define HELP_COMMAND_PARSER_H

# include <string>
# include <map>

# include "command_parser.h"

class HelpCommandParser : public CommandParser
{
public:
	HelpCommandParser(std::map<std::string, CommandParser*> *commands, unsigned int maxCommandNameLength);

	Package* constructPackageFromLine(int argc, const char **argv, std::string packageID);
	char* completionGenerator(const char *text, int state);

private:
	CommandParser* retrieveCommandParser(std::string command);

	std::map<std::string, CommandParser*> *commands;
	std::map<std::string, CommandParser*>::const_iterator commandCompletionIterator;
	unsigned int maxCommandNameLength;
};

# endif /*HELP_COMMAND_PARSER_H*/
