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
# include "viewer_host.h"
# include "view.h"

# include "renderer_host.h"

RendererHost::RendererHost(int hostSocket, View *viewToConnectTo, std::string initialURI) : AbstractHost(hostSocket)
{
	type = ServerComponentRendererHost;
	this->viewToConnectTo = viewToConnectTo;
	this->initialURI = initialURI;

	LOG_DEBUG("initialized with socket "<<hostSocket);
}
RendererHost::~RendererHost()
{
	server->displayManagerInstance()->unregisterRenderer(this->getID());
	
	LOG_DEBUG("shutting down renderer host");
}

void RendererHost::handlePackage(Package *thePackage)
{
	LOG_INFO("received package of type "<<thePackage->getType());
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
			
			LOG_DEBUG(displayInformationType);
			LOG_DEBUG(displayInformation);

			if(viewToConnectTo)
			{
				server->displayManagerInstance()->connect(viewToConnectTo, this);
				viewToConnectTo = NULL;
			}

			sendPackageAndDelete(constructAcknowledgementPackage(thePackage));
			
			state = Established;

			LOG_INFO("connection successfully established");

			if(!initialURI.empty())
			{
				sendPackageAndDelete(constructPackage("command", "command", "open-uri", "uri", initialURI.c_str(), NULL));
			}
		}
		else
		{
			sendPackageAndDelete(constructAcknowledgementPackage(thePackage, "invalid"));
		}
	}
	if(state == Established)
	{
		if(thePackage->getValue("command") == "new-renderer-requested" && thePackage->hasValue("uri"))
		{
			View *theView = server->displayManagerInstance()->viewForRenderer(this);
			if(theView && theView->getHost())
			{
				theView->getHost()->createView(thePackage->getValue("uri"));
			}
		}
		else
		{
			sendPackageAndDelete(constructAcknowledgementPackage(thePackage, "invalid"));
		}
	}
	else
	{
		sendPackageAndDelete(constructAcknowledgementPackage(thePackage, "invalid"));
	}

	delete thePackage;
}

void RendererHost::doJob(Job *theJob)
{
//	if(theJob->getValue("command") == "open-uri" && theJob->hasValue("uri"))
//	{
		forwardJob(theJob);
//	}
}

RendererHost* RendererHost::spawnRenderer(std::string binaryPath, View *viewToConnectTo, std::string initialURI)
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
		
		host = new RendererHost(sockets[0], viewToConnectTo, initialURI);
		return host;
	}
	else // error
	{
		strerror_r(errno, errorBuffer, 128);
		throw std::runtime_error("spawning the renderer process failed: "+std::string(errorBuffer));
	}
}
