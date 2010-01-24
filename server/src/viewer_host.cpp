//      viewer_host.cpp
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
# include "log.h"

# include "package.h"
# include "package_factories.h"

# include "viewer_host.h"

ViewerHost::ViewerHost(int hostSocket) : AbstractHost(hostSocket)
{
	LOG_DEBUG("initialized with socket "<<hostSocket)
}
ViewerHost::~ViewerHost()
{
	LOG_DEBUG("shutting down viewer host")
}

void ViewerHost::handlePackage(Package* thePackage)
{
	LOG_INFO("received package of type "<<thePackage->getType())

	if(state == Connected && thePackage->getType() == ConnectionManagement)
	{
		// check for init packages and handle them
		if(thePackage->getValue("command") == "initialize" && thePackage->isSet("id"))
		{
			displaysStati = thePackage->isSet("can-display-stati");
			displaysPopups = thePackage->isSet("can-display-popups");
			sendPackage(constructAcknowledgementPackage(thePackage));

			state = Established;

			LOG_INFO("connection successfully established");
		}
	}
	else if(state == Established)
	{
		switch(thePackage->getType())
		{
		}
	}
}
