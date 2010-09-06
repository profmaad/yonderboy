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
# include <iostream>

# include <cstdlib>
# include <unistd.h>

# include <readline/readline.h>
# include <readline/history.h>

# include <package.h>
# include <package_factories.h>
# include <ev_cpp.h>

# include "defaults.h"

# include "controller.h"

extern Controller *controllerInstance;

Controller::Controller(int serverSocket) : AbstractHost(serverSocket), stdinWatcher(NULL)
{
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
}
void Controller::quit()
{
	stdinWatcher->stop();

	ev::get_default_loop().unloop(ev::ALL);
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
void Controller::handleLine(char *rawLine)
{
	if(rawLine == NULL) // we got an EOF, that means we should quit
	{
		quit();
		return;
	}

	std::string line(rawLine);
	free(rawLine);

	std::cerr<<"handleLine: "<<line<<std::endl;
}
