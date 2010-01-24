//      renderer_host.cpp
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

# include <stdexcept>
# include <string>

# include <cerrno>
# include <cstring>
# include <sys/socket.h>

# include "ev_cpp.h"

# include "macros.h"
# include "log.h"

# include "package.h"
# include "package_factories.h"

# include "renderer_host.h"

RendererHost::RendererHost(int hostSocket) : AbstractHost(hostSocket)
{
	LOG_DEBUG("initialized with socket "<<hostSocket);
}
RendererHost::~RendererHost()
{
	LOG_DEBUG("shutting down renderer host");
}

void RendererHost::handlePackage(Package* thePackage)
{
	LOG_INFO("received package of type "<<thePackage->getType());

	if(state == Connected)
	{
		// check for init packages and handle them
		if(thePackage->getValue("command") == "initialize" && thePackage->isSet("id"))
		{
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
