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

# include <stdexcept>
# include <map>
# include <string>

# include <cerrno>
# include <cstring>
# include <sys/socket.h>

# include "ev_cpp.h"

# include "macros.h"
# include "log.h"

# include "package.h"
# include "package_factories.h"
# include "job.h"
# include "job_manager.h"
# include "hosts_manager.h"
# include "view.h"
# include "package_router.h"
# include "configuration_manager.h"
# include "display_manager.h"
# include "renderer_host.h"

# include "viewer_host.h"

ViewerHost::ViewerHost(int hostSocket) : AbstractHost(hostSocket), views(NULL)
{
	type = ServerComponentViewerHost;
	views = new std::map<std::string, View*>();

	LOG_DEBUG("initialized with socket "<<hostSocket);
}
ViewerHost::~ViewerHost()
{
	for(std::map<std::string, View*>::const_iterator iter = views->begin(); iter != views->end(); ++iter)
	{
		RendererHost *renderer = server->displayManagerInstance()->rendererForView(iter->second);
		if(renderer) { renderer->shutdownHost(); }
		delete iter->second;
	}
	delete views;

	LOG_DEBUG("shutting down viewer host");
}

View* ViewerHost::retrieveView(std::string viewID)
{
	View *result = NULL;

	std::map<std::string, View*>::const_iterator iter = views->find(viewID);
	if(iter != views->end())
	{
		result = iter->second;
	}

	return result;
}

void ViewerHost::doJob(Job *theJob)
{
}

void ViewerHost::handlePackage(Package* thePackage)
{
	LOG_INFO("received package of type "<<thePackage->getType());

	// check for init packages and handle them
	if(state == Connected && thePackage->getValue("command") == "initialize")
	{
		displaysStati = thePackage->isSet("can-display-stati");
		displaysPopups = thePackage->isSet("can-display-popups");
		clientName = thePackage->getValue("client-name");
		clientVersion = thePackage->getValue("client-version");
		
		server->packageRouterInstance()->addStatiReceiver(this);
		
		sendPackageAndDelete(constructAcknowledgementPackage(thePackage));
		
		state = Established;
		
		LOG_INFO("connection successfully established, viewer can"<<(displaysStati?"":"'t")<<" display status messages and can"<<(displaysPopups?"":"'t")<<" display popups");
	}
	else if(state == Established && thePackage->getValue("command") == "register-view" && thePackage->isSet("view-id"))
	{
		View *theView = new View(this, thePackage);
		std::string error;

		if(theView->isValid())
		{
			views->insert(std::make_pair(thePackage->getValue("view-id"),theView));

			if(thePackage->isSet("create-renderer"))
			{
				RendererHost *theRenderer = server->hostsManagerInstance()->createRenderer(server->configurationManagerInstance()->retrieve("server", "renderer-binary", "/bin/false"), theView);
			}
			else
			{
				error = "failed";
			}
		}
		else
		{
			error = "failed";
		}

		sendPackageAndDelete(constructAcknowledgementPackage(thePackage, error));
	}
	else if(state == Established && thePackage->getValue("command") == "unregister-view" && thePackage->isSet("view-id"))
	{
		View *theView = retrieveView(thePackage->getValue("view-id"));
		if(theView)
		{
			if(!thePackage->isSet("keep-renderer"))
			{
				RendererHost *renderer = server->displayManagerInstance()->rendererForView(theView);
				renderer->shutdownHost();
			}

			views->erase(thePackage->getValue("view-id"));
			
			sendPackageAndDelete(constructAcknowledgementPackage(thePackage));

			delete theView;
		}
	}
	else if(state == Established && thePackage->getValue("command") == "got-focus" && thePackage->isSet("view-id"))
	{
		View *theView = retrieveView(thePackage->getValue("view-id"));
		
		if(theView) { server->hostsManagerInstance()->setFocus(theView); }
	}
	else
	{
		sendPackageAndDelete(constructAcknowledgementPackage(thePackage, "invalid"));
	}

	delete thePackage;
}
