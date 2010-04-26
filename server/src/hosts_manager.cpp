//      hosts_manager.cpp
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

# include <map>
# include <string>
# include <vector>
# include <sstream>
# include <iomanip>
# include <stdexcept>

# include "ev_cpp.h"
# include "log.h"
# include "macros.h"

# include "abstract_host.h"
# include "renderer_host.h"
# include "viewer_host.h"
# include "controller_host.h"
# include "configuration_manager.h"
# include "job_manager.h"
# include "display_manager.h"
# include "job.h"

# include "hosts_manager.h"

HostsManager::HostsManager() : hosts(NULL), hostsScheduledForDeletion(NULL), idleTimer(NULL), nextRendererID(0), nextViewerID(0), nextControllerID(0), mainControllerID("")
{
	hosts = new std::map<std::string, std::pair<ServerComponent, AbstractHost*> >();

	hostsScheduledForDeletion = new std::vector<AbstractHost*>();

	idleTimer = new ev::idle();
	idleTimer->set<HostsManager, &HostsManager::idleCallback>(this);

	LOG_INFO("initialized")
}
HostsManager::~HostsManager()
{
	if(hosts)
	{
		for(std::map<std::string, std::pair<ServerComponent, AbstractHost*> >::iterator iter = hosts->begin(); iter != hosts->end(); ++iter)
		{
			delete iter->second.second;
		}
		
		delete hosts;
	}
	if(hostsScheduledForDeletion)
	{
		for(std::vector<AbstractHost*>::iterator iter = hostsScheduledForDeletion->begin(); iter != hostsScheduledForDeletion->end(); ++iter)
		{
			delete *iter;
		}
		
		delete hostsScheduledForDeletion;
	}

	delete idleTimer;
}

void HostsManager::doJob(Job *theJob)
{
	if(theJob->getValue("command") == "spawn-renderer")
	{
		RendererHost *newRenderer = NULL;
		try
		{
			newRenderer = RendererHost::spawnRenderer(server->configurationManagerInstance()->retrieve("server", "renderer-binary", "/bin/false"));

			if(newRenderer)
			{
				registerHost(newRenderer);
				server->displayManagerInstance()->registerRenderer(newRenderer);
				server->jobManagerInstance()->jobDone(theJob);
				LOG_DEBUG("new renderer has id "<<newRenderer->getID());
			}
		}
		catch(std::runtime_error e)
		{
			server->jobManagerInstance()->jobFailed(theJob, e.what());
		}
	}
}

std::string HostsManager::registerHost(RendererHost *host)
{
	if(!host) { return ""; }

	std::string newID = getNextRendererID();
	host->setID(newID);
	
	hosts->insert(std::make_pair(newID, std::make_pair(ServerComponentRendererHost, host)));
	
	return newID;
}
std::string HostsManager::registerHost(ViewerHost *host)
{
	if(!host) { return ""; }

	std::string newID = getNextViewerID();
	host->setID(newID);
       
	hosts->insert(std::make_pair(newID, std::make_pair(ServerComponentViewerHost, host)));
	
	return newID;
}
std::string HostsManager::registerHost(ControllerHost *host)
{
	if(!host) { return ""; }

	std::string newID = getNextControllerID();
	host->setID(newID);
	
	hosts->insert(std::make_pair(newID, std::make_pair(ServerComponentControllerHost, host)));

	if(mainControllerID.empty() && host->isInteractive() && host->canHandleSynchronousRequests())
	{
		mainControllerID = host->getID();
	}
	
	return newID;
}

std::pair<ServerComponent, AbstractHost*> HostsManager::getHost(std::string id)
{
	std::map<std::string, std::pair<ServerComponent, AbstractHost*> >::iterator iter = hosts->find(id);

	if(iter != hosts->end())
	{
		return iter->second;
	}

	return std::pair<ServerComponent, AbstractHost*>(static_cast<ServerComponent>(-1), NULL);
}
RendererHost* HostsManager::getRendererHost(std::string id)
{
	std::pair<ServerComponent, AbstractHost*> result = getHost(id);
	if(result.second && result.first == ServerComponentRendererHost) { return static_cast<RendererHost*>(result.second); }

	return NULL;
}
ViewerHost* HostsManager::getViewerHost(std::string id)
{
	std::pair<ServerComponent, AbstractHost*> result = getHost(id);
	if(result.second && result.first == ServerComponentViewerHost) { return static_cast<ViewerHost*>(result.second); }

	return NULL;
}
ControllerHost* HostsManager::getControllerHost(std::string id)
{
	std::pair<ServerComponent, AbstractHost*> result = getHost(id);
	if(result.second && result.first == ServerComponentControllerHost) { return static_cast<ControllerHost*>(result.second); }

	return NULL;
}

void HostsManager::scheduleHostForDeletion(std::string id)
{
	AbstractHost *host = getHost(id).second;

	if(host)
	{
		hosts->erase(id);
		
		hostsScheduledForDeletion->push_back(host);

		if(! idleTimer->is_active()) idleTimer->start();
	}
}

void HostsManager::idleCallback(ev::idle &watcher, int revents)
{
	for(std::vector<AbstractHost*>::iterator iter = hostsScheduledForDeletion->begin(); iter != hostsScheduledForDeletion->end(); )
	{
		if((*iter) && (*iter)->getState() == Disconnected)
		{
			delete (*iter);
			iter = hostsScheduledForDeletion->erase(iter);
		}
		else
		{
			++iter;
		}
	}

	if(hostsScheduledForDeletion->empty())
	{
		idleTimer->stop();
	}
}

std::string HostsManager::getNextRendererID()
{
	return composeID("renderer",nextRendererID++);
}
std::string HostsManager::getNextViewerID()
{
	return composeID("viewer",nextViewerID++);
}
std::string HostsManager::getNextControllerID()
{
	return composeID("controller",nextControllerID++);
}

std::string HostsManager::composeID(std::string prefix, unsigned long number)
{
	std::ostringstream conversionStream("");
	conversionStream<<prefix.c_str();
	conversionStream<<number;

	return conversionStream.str();
}


