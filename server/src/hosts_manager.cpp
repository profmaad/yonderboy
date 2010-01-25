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

# include <iostream>
# include <map>
# include <utility>

# include "ev_cpp.h"
# include "log.h"

# include "hosts_manager.h"
# include "abstract_host.h"

HostsManager::HostsManager() : hosts(NULL), hostsScheduledForDeletion(NULL), idleTimer(NULL)
{
	hosts = new std::map<int, const AbstractHost*>();
	hostsScheduledForDeletion = new std::map<int, AbstractHost*>();

	idleTimer = new ev::idle();
	idleTimer->set<HostsManager, &HostsManager::idleCallback>(this);

	LOG_INFO("initialized")
}
HostsManager::~HostsManager()
{
	if(hosts)
	{
		for(std::map<int, const AbstractHost*>::iterator iter = hosts->begin(); iter != hosts->end(); ++iter)
		{
			delete iter->second;
		}
		
		delete hosts;
	}
	if(hostsScheduledForDeletion)
	{
		for(std::map<int, AbstractHost*>::iterator iter = hostsScheduledForDeletion->begin(); iter != hostsScheduledForDeletion->end(); ++iter)
		{
			delete iter->second;
		}
		
		delete hostsScheduledForDeletion;
	}

	delete idleTimer;
}

void HostsManager::registerHost(int id, const AbstractHost* host)
{
	if(id < 0 || host == NULL) return;

	hosts->insert(std::make_pair(id,host));
}
void HostsManager::deregisterHost(int id)
{
	hosts->erase(id);
}
const AbstractHost* HostsManager::getHost(int id)
{
	std::map<int, const AbstractHost*>::const_iterator iter = hosts->find(id);

	if(iter != hosts->end())
	{
		return iter->second;
	}

	return NULL;
}
void HostsManager::scheduleHostForDeletion(int id)
{
	AbstractHost *theHost = const_cast<AbstractHost*>(getHost(id));
	if(theHost)
	{
		deregisterHost(id);
		hostsScheduledForDeletion->insert(std::make_pair(id,theHost));

		if(! idleTimer->is_active()) idleTimer->start();
	}
}

void HostsManager::idleCallback(ev::idle &watcher, int revents)
{
	for(std::map<int, AbstractHost*>::iterator iter = hostsScheduledForDeletion->begin(); iter != hostsScheduledForDeletion->end(); ++iter)
	{
		if(iter->second->getState() == Disconnected)
		{
			delete iter->second;
			hostsScheduledForDeletion->erase(iter);
		}
	}

	if(hostsScheduledForDeletion->size() == 0)
	{
		idleTimer->stop();
	}
}
