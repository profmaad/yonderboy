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
# include <sstream>

# include <cerrno>
# include <cstring>
# include <sys/types.h>
# include <sys/socket.h>
# include <unistd.h>

# include "ev_cpp.h"

# include "macros.h"
# include "log.h"

# include "package.h"
# include "package_factories.h"
# include "job.h"
# include "job_manager.h"
# include "display_manager.h"
# include "hosts_manager.h"
# include "package_router.h"
# include "job.h"

# include "renderer_host.h"

RendererHost::RendererHost(int hostSocket) : AbstractHost(hostSocket)
{
	LOG_DEBUG("initialized with socket "<<hostSocket);
}
RendererHost::~RendererHost()
{
	server->displayManagerInstance()->unregisterRenderer(this->getID());
	
	LOG_DEBUG("shutting down renderer host");
}

void RendererHost::handlePackage(Package* thePackage)
{
	LOG_INFO("received package of type "<<thePackage->getType());

	if(!server->packageRouterInstance()->isAllowed(ServerComponentRendererHost, thePackage))
	{
		sendPackageAndDelete(constructAcknowledgementPackage(thePackage, "forbidden"));
		delete thePackage;
		return;
	}

	if(state == Connected)
	{
		// check for init packages and handle them
		if(thePackage->getValue("command") == "initialize")
		{
			clientName = thePackage->getValue("client-name");
			clientVersion = thePackage->getValue("client-version");
			backendName = thePackage->getValue("backend-name");
			backendVersion = thePackage->getValue("backend-version");
			displayInformation = thePackage->getValue("display-information");
			displayInformationType = thePackage->getValue("display-information-type");

			LOG_DEBUG(displayInformation);
			LOG_DEBUG(displayInformationType);

			sendPackageAndDelete(constructAcknowledgementPackage(thePackage));
			delete thePackage;
			
			state = Established;

			LOG_INFO("connection successfully established");
		}
	}
	else if(state == Established)
	{
		server->packageRouterInstance()->processPackage(this, thePackage);
	}
	else
	{
		delete thePackage;
	}
}

void RendererHost::doJob(Job *theJob)
{
	if(theJob->getValue("command") == "open-uri" && theJob->hasValue("uri"))
	{
		sendPackage(theJob);
		server->jobManagerInstance()->jobDone(theJob);
	}
}

RendererHost* RendererHost::spawnRenderer(std::string binaryPath)
{
	RendererHost *host = NULL;
	int sockets[2] = { -1, -1 };
	int result = -1;
	pid_t forkResult = -1;
	char errorBuffer[128] = {'\0'};

	// create connected socket pair
	result = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	if(result < 0)
	{
		strerror_r(errno, errorBuffer, 128);
		throw std::runtime_error("socket pair creation for renderer failed: "+std::string(errorBuffer));
	}

	forkResult = fork();
	if(forkResult == 0) // child
	{
		close(sockets[0]);

		std::string binaryName = binaryPath;	
		size_t lastSlash = binaryPath.find_last_of('/');
		if(lastSlash != std::string::npos)
		{
			binaryName = binaryPath.substr(lastSlash);
		}

		std::ostringstream conversionStream;
		conversionStream<<(sockets[1]);
		
		result = execlp(binaryPath.c_str(), binaryName.c_str(), conversionStream.str().c_str(), static_cast<char*>(NULL));

		// when we reach this code, execlp failed
		close(sockets[1]);
		strerror_r(errno, errorBuffer, 128);
		throw std::runtime_error("executing the renderer binary failed: "+std::string(errorBuffer));
	}
	else if(forkResult > 0) // parent
	{
		LOG_INFO("forked renderer with pid "<<forkResult);

		close(sockets[1]);
		
		host = new RendererHost(sockets[0]);
		return host;
	}
	else // error
	{
		strerror_r(errno, errorBuffer, 128);
		throw std::runtime_error("spawning the renderer process failed: "+std::string(errorBuffer));
	}
}
