//      controller.h
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

# ifndef CONTROLLER_H
# define CONTROLLER_H

# include <string>
# include <map>

# include <abstract_host.h>
# include <ev_cpp.h>

class Package;
class CommandParser;

class Controller : public AbstractHost
{
public:
	Controller(int serverSocket);
	~Controller();

protected:
	void handlePackage(Package *thePackage);
	void socketClosed();

private:
	void parseSpecFile(std::string file);
	void quit();

	std::string handleCommand(Package *thePackage);
	void handleStatusChange(Package *thePackage);

	static void readlineCallback(char *line);
	void handleLine(char *line);

	static char** completionCallback(const char *text, int start, int end);
	char** generateCompletionMatches(const char *text, int start, int end);
	static char* commandCompletionGeneratorWrapper(const char *text, int state);
	char* commandCompletionGenerator(const char *text, int state);
	static char* parametersCompletionGeneratorWrapper(const char *text, int state);
	char* parametersCompletionGenerator(const char *text, int state);

	CommandParser* retrieveCommandParser(std::string command);

	void stdinCallback(ev::io &watcher, int revents);

	ev::io *stdinWatcher;

	std::map<std::string, CommandParser*> *commands;
	std::map<std::string, CommandParser*>::const_iterator commandCompletionIterator;

	unsigned long long lastSendPackageID;
};

# endif /*CONTROLLER_H*/
