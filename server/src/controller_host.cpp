//      controller_host.cpp
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
# include <stdexcept>

# include <cerrno>
# include <cstring>
# include <sys/socket.h>

# include "ev_cpp.h"
# include "macros.h"
# include "package.h"
# include "package_factories.h"

# include "controller_host.h"

ControllerHost::ControllerHost(int hostSocket) : AbstractHost(hostSocket), interactive(false), handlesSynchronousRequests(false)
{
	std::cout<<"[ControllerHost "<<hostSocket<<"] initialized"<<std::endl;
}
ControllerHost::~ControllerHost()
{
	std::cout<<"[ControllerHost] shut down"<<std::endl;
}

void ControllerHost::handlePackage(Package* thePackage)
{
	std::cout<<"[ControllerHost] received package of type "<<thePackage->getType()<<std::endl;

	if(state == Connected && thePackage->getType() == ConnectionManagement)
	{
		// check for init packages and handle them
		if(thePackage->getValue("command") == "initialize" && thePackage->isSet("id"))
		{
			interactive = thePackage->isSet("interactive");
			handlesSynchronousRequests = thePackage->isSet("can-handle-requests");

			sendPackage(constructAcknowledgementPackage(thePackage));

			state = Established;

			std::cout<<"[ControllerHost] connection successfully established, controller is "<<(interactive?"":"not ")<<"interactive and can"<<(handlesSynchronousRequests?"":"'t")<<" handle synchronous requests"<<std::endl;
 		}
	}
	else if(state == Established)
	{
		switch(thePackage->getType())
		{
		case Command:
			std::cout<<"[ControllerHost] received command '"<<thePackage->getValue("command")<<"' from controller"<<std::endl;
			sendPackage(constructAcknowledgementPackage(thePackage));
			break;
		}
	}
}
