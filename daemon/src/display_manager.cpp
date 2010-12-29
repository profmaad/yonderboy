//      display_manager.cpp
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

# include <string>
# include <map>

# include "log.h"
# include "macros.h"

# include "package.h"
# include "job.h"
# include "view.h"
# include "viewer_host.h"
# include "renderer_host.h"
# include "job_manager.h"
# include "hosts_manager.h"

# include "display_manager.h"

DisplayManager::DisplayManager() : views(NULL), renderers(NULL), viewByRenderer(NULL), rendererByView(NULL)
{
	views = new std::map<std::pair<std::string, ViewerHost*>, View*>();
	renderers = new std::map<std::string, RendererHost*>();

	viewByRenderer = new std::map<RendererHost*, View*>();
	rendererByView = new std::map<View*, RendererHost*>();
}
DisplayManager::~DisplayManager()
{
	for(std::map<std::pair<std::string, ViewerHost*>, View*>::iterator iter = views->begin(); iter != views->end(); ++iter)
	{
		disconnectView(iter->second);
	}
	for(std::map<std::string, RendererHost*>::iterator iter = renderers->begin(); iter != renderers->end(); ++iter)
	{
		disconnectRenderer(iter->second);
	}

	delete views;
	delete renderers;
	delete viewByRenderer;
	delete rendererByView;
}

void DisplayManager::registerView(View *theView)
{
	if(theView)
	{
		views->insert(std::make_pair(std::make_pair(theView->getID(), theView->getHost()), theView));

		LOG_INFO("view "<<theView->getID()<<"@"<<theView->getHost()<<" registered");
	}
}
void DisplayManager::registerRenderer(RendererHost *theRenderer)
{
	if(theRenderer)
	{
		renderers->insert(std::make_pair(theRenderer->getID(), theRenderer));

		LOG_INFO("renderer "<<theRenderer->getID()<<" registered");
	}
}

void DisplayManager::unregisterView(std::string viewID, ViewerHost *host)
{
	View *theView = NULL;

	std::map<std::pair<std::string, ViewerHost*>, View*>::iterator viewIter = views->find(std::make_pair(viewID, host));
	if(viewIter == views->end()) { return; }

	theView = viewIter->second;
	disconnectView(theView);
	views->erase(viewIter);

	LOG_INFO("view "<<theView->getID()<<"@"<<theView->getHost()<<" unregistered");
}
void DisplayManager::unregisterRenderer(std::string rendererID)
{
	RendererHost *theRenderer = NULL;
	
	std::map<std::string, RendererHost*>::iterator rendererIter = renderers->find(rendererID);
	if(rendererIter == renderers->end()) { return; }

	theRenderer = rendererIter->second;
	disconnectRenderer(theRenderer);
	renderers->erase(rendererIter);

	LOG_INFO("renderer "<<theRenderer->getID()<<" unregistered");
}
void DisplayManager::unregisterView(View *theView)
{
	if(theView) { unregisterView(theView->getID(), theView->getHost()); }
}
void DisplayManager::unregisterRenderer(RendererHost *theRenderer)
{
	if(theRenderer) { unregisterRenderer(theRenderer->getID()); }
}

void DisplayManager::connect(View *theView, RendererHost *theRenderer, Job *connectJob)
{
	if(theView->getDisplayInformationType() != theRenderer->getDisplayInformationType()) { return; }
	if(theView->isAssigned() && !theView->isReassignable()) { return; }

	rendererByView->erase(theView);
	viewByRenderer->erase(theRenderer);

	Package *viewerPackage = constructViewConnectPackage(theView, theRenderer);
	Package *rendererPackage = constructRendererConnectPackage(theRenderer, theView);
	
	Job *viewerJob = new Job(theView->getHost(), viewerPackage, theView->getHost()->getNextPackageID());
	Job *rendererJob = new Job(theRenderer, rendererPackage, theRenderer->getNextPackageID());

	server->jobManagerInstance()->addJob(viewerJob);
	server->jobManagerInstance()->addJob(rendererJob);
     
	if(connectJob)
	{
		server->jobManagerInstance()->addDependency(connectJob, viewerJob);
		server->jobManagerInstance()->addDependency(connectJob, rendererJob);
	}

	theView->getHost()->sendPackage(viewerJob);
	theRenderer->sendPackage(rendererJob);
	
	rendererByView->insert(std::make_pair(theView, theRenderer));
	viewByRenderer->insert(std::make_pair(theRenderer, theView));

	theView->setAssigned(true);
}
void DisplayManager::doJob(Job *theJob)
{
	View *theView = NULL;
	RendererHost *theRenderer = NULL;

	if(theJob->getType() == Command && theJob->getValue("command") == "connect-view-to-renderer" && theJob->isSet("view-id") && theJob->isSet("renderer-id") && theJob->isSet("viewer-id"))
	{
		ViewerHost *viewerHost = server->hostsManagerInstance()->getViewerHost(theJob->getValue("viewer-id"));
		View *view = NULL;
		if(theJob->getValue("view-id") == "focused")
		{
			view = server->hostsManagerInstance()->getFocusedView();
		}
		else
		{
			std::map<std::pair<std::string, ViewerHost*>, View*>::const_iterator viewIter = views->find(std::make_pair(theJob->getValue("view-id"), viewerHost));
			if(viewIter != views->end())
			{
				view = viewIter->second;
			}
		}
		std::map<std::string, RendererHost*>::const_iterator rendererIter = renderers->find(theJob->getValue("renderer-id"));

		if(view && rendererIter != renderers->end())
		{
			connect(view, rendererIter->second, theJob);
		}
		else
		{
			delete theJob;
		}
	}
	else
	{
		delete theJob;
	}
}

void DisplayManager::disconnectView(std::string viewID, ViewerHost *host)
{
	std::map<std::pair<std::string, ViewerHost*>, View*>::const_iterator iter = views->find(std::make_pair(viewID, host));
	if(iter != views->end())
	{
		disconnectView(iter->second);
	}
}
void DisplayManager::disconnectRenderer(std::string rendererID)
{
	std::map<std::string, RendererHost*>::const_iterator iter = renderers->find(rendererID);
	if(iter != renderers->end())
	{
		disconnectRenderer(iter->second);
	}
}
void DisplayManager::disconnectView(View *theView)
{
	std::map<View*, RendererHost*>::iterator iter = rendererByView->find(theView);
	if(iter != rendererByView->end())
	{
		disconnect(theView, iter->second);
	}
}
void DisplayManager::disconnectRenderer(RendererHost *theRenderer)
{
	std::map<RendererHost*, View*>::iterator iter = viewByRenderer->find(theRenderer);
	if(iter != viewByRenderer->end())
	{
		disconnect(iter->second, theRenderer);
	}
}
void DisplayManager::disconnect(View *theView, RendererHost *theRenderer)
{
	viewByRenderer->erase(theRenderer);
	rendererByView->erase(theView);
	
	theView->getHost()->sendPackage(constructViewDisconnectPackage(theView));
	theRenderer->sendPackage(constructRendererDisconnectPackage(theRenderer));
}

bool DisplayManager::isConnected(View *theView)
{
	std::map<View*, RendererHost*>::const_iterator iter = rendererByView->find(theView);
	if(iter == rendererByView->end()) { return false; }
	else { return true; }
}
bool DisplayManager::isConnected(RendererHost *theRenderer)
{
	std::map<RendererHost*, View*>::const_iterator iter = viewByRenderer->find(theRenderer);
	if(iter == viewByRenderer->end()) { return false; }
	else { return true; }
}
bool DisplayManager::areConnected(ViewerHost *theViewer, RendererHost *theRenderer)
{
	std::map<RendererHost*, View*>::const_iterator iter = viewByRenderer->find(theRenderer);
	if(iter != viewByRenderer->end())
	{
		return (iter->second->getHost() == theViewer);
	}

	return false;
}

RendererHost* DisplayManager::rendererForView(View *theView)
{
	std::map<View*, RendererHost*>::iterator iter = rendererByView->find(theView);
	if(iter != rendererByView->end())
	{
		return iter->second;
	}
	
	return NULL;
}
View* DisplayManager::viewForRenderer(RendererHost *theRenderer)
{
	std::map<RendererHost*, View*>::iterator iter = viewByRenderer->find(theRenderer);
	if(iter != viewByRenderer->end())
	{
		return iter->second;
	}
	
	return NULL;
}

Package* DisplayManager::constructViewConnectPackage(View *theView, RendererHost *theRenderer)
{
	Package *result = NULL;
	std::map<std::string, std::string> *kvMap = new std::map<std::string, std::string>();

	kvMap->insert(std::make_pair("type", "command"));
	kvMap->insert(std::make_pair("command", "connect-to-renderer"));
	kvMap->insert(std::make_pair("display-information-type", theRenderer->getDisplayInformationType()));
	kvMap->insert(std::make_pair("display-information", theRenderer->getDisplayInformation()));
	kvMap->insert(std::make_pair("view-id", theView->getID()));
	kvMap->insert(std::make_pair("renderer-id", theRenderer->getID()));

	result = new Package(kvMap);

	return result;
}
Package* DisplayManager::constructRendererConnectPackage(RendererHost *theRenderer, View *theView)
{
	Package *result = NULL;
	std::map<std::string, std::string> *kvMap = new std::map<std::string, std::string>();

	kvMap->insert(std::make_pair("type", "command"));
	kvMap->insert(std::make_pair("command", "connect-to-view"));
	kvMap->insert(std::make_pair("display-information-type", theView->getDisplayInformationType()));
	kvMap->insert(std::make_pair("display-information", theView->getDisplayInformation()));

	result = new Package(kvMap);

	return result;
}
Package* DisplayManager::constructViewDisconnectPackage(View *theView)
{
	Package *result = NULL;
	std::map<std::string, std::string> *kvMap = new std::map<std::string, std::string>();

	kvMap->insert(std::make_pair("type", "command"));
	kvMap->insert(std::make_pair("command", "disconnect-from-renderer"));
	kvMap->insert(std::make_pair("view-id", theView->getID()));
	
	std::map<View*, RendererHost*>::const_iterator iter = rendererByView->find(theView);
	if(iter != rendererByView->end())
	{
		kvMap->insert(std::make_pair("renderer-id", iter->second->getID()));
	}

	result = new Package(kvMap);

	return result;
}
Package* DisplayManager::constructRendererDisconnectPackage(RendererHost *theRenderer)
{
	Package *result = NULL;
	std::map<std::string, std::string> *kvMap = new std::map<std::string, std::string>();

	kvMap->insert(std::make_pair("type", "command"));
	kvMap->insert(std::make_pair("command", "disconnect-from-view"));

	result = new Package(kvMap);

	return result;
}
