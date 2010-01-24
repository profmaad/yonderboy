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
# include "view.h"
# include "viewer_host.h"
# include "renderer_host.h"

# include "display_manager.h"

DisplayManager::DisplayManager() : views(NULL), renderers(NULL), viewByRenderer(NULL), rendererByView(NULL)
{
	views = new std::map<std::string, View*>();
	renderers = new std::map<std::string, RendererHost*>();

	viewByRenderer = new std::map<RendererHost*, View*>();
	rendererByView = new std::map<View*, RendererHost*>();
}
DisplayManager::~DisplayManager()
{
	for(std::map<std::string, View*>::iterator iter = views->begin(); iter != views->end(); ++iter)
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
		views->insert(std::make_pair(theView->getID(), theView));
	}
}
void DisplayManager::registerRenderer(RendererHost *theRenderer)
{
	if(theRenderer)
	{
		renderers->insert(std::make_pair(theRenderer->getID(), theRenderer));
	}
}

void DisplayManager::unregisterView(std::string viewID)
{
	View *theView = NULL;

	std::map<std::string, View*>::iterator viewIter = views->find(viewID);
	if(viewIter == views->end()) { return; }

	theView = viewIter->second;
	disconnectView(theView);
	views->erase(viewIter);
}
void DisplayManager::unregisterRenderer(std::string rendererID)
{
	RendererHost *theRenderer = NULL;
	
	std::map<std::string, RendererHost*>::iterator rendererIter = renderers->find(rendererID);
	if(rendererIter == renderers->end()) { return; }

	theRenderer = rendererIter->second;
	disconnectRenderer(theRenderer);
	renderers->erase(rendererIter);
}
void DisplayManager::unregisterView(View *theView)
{
	if(theView) { unregisterView(theView->getID()); }
}
void DisplayManager::unregisterRenderer(RendererHost *theRenderer)
{
	if(theRenderer) { unregisterRenderer(theRenderer->getID()); }
}

void DisplayManager::connect(View *theView, RendererHost *theRenderer)
{
	if(theView->getDisplayInformationType() != theRenderer->getDisplayInformationType()) { return; }

	rendererByView->erase(theView);
	viewByRenderer->erase(theRenderer);

	theView->getHost()->sendPackage(constructViewConnectPackage(theView, theRenderer));
	theRenderer->sendPackage(constructRendererConnectPackage(theRenderer, theView));
	
	rendererByView->insert(std::make_pair(theView, theRenderer));
	viewByRenderer->insert(std::make_pair(theRenderer, theView));
}
void DisplayManager::parsePackage(Package *thePackage)
{
	View *theView = NULL;
	RendererHost *theRenderer = NULL;

	if(thePackage->getType() == Command && thePackage->getValue("command") == "connect-view" && thePackage->isSet("view-id") && thePackage->isSet("renderer-id"))
	{
		std::map<std::string, View*>::const_iterator viewIter = views->find(thePackage->getValue("view-id"));
		std::map<std::string, RendererHost*>::const_iterator rendererIter = renderers->find(thePackage->getValue("renderer-id"));

		if(viewIter != views->end() && rendererIter != renderers->end())
		{
			connect(viewIter->second, rendererIter->second);
		}
	}
}

void DisplayManager::disconnectView(std::string viewID)
{
	std::map<std::string, View*>::const_iterator iter = views->find(viewID);
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

Package* DisplayManager::constructViewConnectPackage(View *theView, RendererHost *theRenderer)
{
	Package *result = NULL;
	std::map<std::string, std::string> *kvMap = new std::map<std::string, std::string>();

	kvMap->insert(std::make_pair("type", "command"));
	kvMap->insert(std::make_pair("command", "connect-to-renderer"));
	kvMap->insert(std::make_pair("display-information-type", theRenderer->getDisplayInformationType()));
	kvMap->insert(std::make_pair("display-information", theRenderer->getDisplayInformation()));

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

