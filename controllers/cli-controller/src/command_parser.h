//      command_parser.h
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

# ifndef COMMAND_PARSER_H
# define COMMAND_PARSER_H

# include <string>
# include <map>

# include <yaml.h>
# include <popt.h>

class Package;

class CommandParser
{
public:
	CommandParser(const YAML::Node &node);
	CommandParser();
	~CommandParser();

	virtual Package* constructPackageFromLine(int argc, const char **argv, std::string packageID);
	virtual char* completionGenerator(const char *text, int state);

	std::string getCommand() { return command; }
	std::string getDescription() { return description; }
	virtual void printParameterHelp();

protected:
	void reset();

	bool valid;
	std::string command;
	std::string description;
	
	poptOption *options;
	unsigned int optionsCount;

	struct requiredParameter
	{
		std::string name;
		std::string description;
	};
	std::vector<requiredParameter> *parameters;

	std::map<std::string, int*> *boolOptionArguments;
	std::map<std::string, char**> *stringOptionArguments;
	char *rendererID;
	bool rendererIDRequired;
	char *viewerID;
	bool viewerIDRequired;
	char *viewID;
	bool viewIDRequired;

	unsigned int generatorPosition;
};

# endif /*COMMAND_PARSER_H*/
