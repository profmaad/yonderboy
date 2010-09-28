//      hosts_manager.h
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

# ifndef HOSTS_MANAGER_H
# define HOSTS_MANAGER_H

# include <map>
# include <vector>
# include <string>

# include "ev_cpp.h"
# include "macros.h"

class AbstractHost;
class RendererHost;
class ViewerHost;
class View;
class ControllerHost;
class Job;

class HostsManager
{
public:
	HostsManager();
	virtual ~HostsManager();

	void doJob(Job *theJob);

	RendererHost* createRenderer(std::string binary, View *viewToConnectTo = NULL, Job *theJob = NULL, std::string initialURI = std::string());

	std::string registerHost(RendererHost *host);
	std::string registerHost(ViewerHost *host);
	std::string registerHost(ControllerHost *host);

	std::pair<ServerComponent, AbstractHost*> getHost(std::string id);
        RendererHost* getRendererHost(std::string id);
	ViewerHost* getViewerHost(std::string id);
	ControllerHost* getControllerHost(std::string id);
	ControllerHost* getMainController() { return getControllerHost(mainControllerID); }
	View* getFocusedView() { return focusedView; }
	ViewerHost* getFocusedViewer();
	RendererHost* getFocusedRenderer();

	void scheduleHostForDeletion(std::string id);
	void setFocus(View *theView) { focusedView = theView; };

private:
	void idleCallback(ev::idle &watcher, int revents);

	std::string getNextRendererID();
	std::string getNextViewerID();
	std::string getNextControllerID();

	std::string composeID(std::string prefix, unsigned long number);

	std::map<std::string, std::pair<ServerComponent, AbstractHost*> > *hosts;
	std::string mainControllerID;
	View *focusedView;

	std::vector<AbstractHost*> *hostsScheduledForDeletion;

	unsigned long nextRendererID;
	unsigned long nextViewerID;
	unsigned long nextControllerID;

	ev::idle *idleTimer;
};

# endif /*HOSTS_MANAGER_H*/
