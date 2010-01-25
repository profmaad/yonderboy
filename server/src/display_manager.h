//      display_manager.h
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

# ifndef DISPLAY_MANAGER_H
# define DISPLAY_MANAGER_H

# include <string>
# include <map>

class View;
class ViewerHost;
class RendererHost;
class Package;

class DisplayManager
{
public:
	DisplayManager();
	~DisplayManager();

	void registerView(View *theView);
	void registerRenderer(RendererHost *theRenderer);

	void unregisterView(std::string viewID, ViewerHost *host);
	void unregisterRenderer(std::string rendererID);
	void unregisterView(View *theView);
	void unregisterRenderer(RendererHost *theRenderer);

	void connect(View *theView, RendererHost *theRenderer);
	void parsePackage(Package *thePackage, ViewerHost *host);

	void disconnectView(std::string viewID, ViewerHost *host);
	void disconnectRenderer(std::string rendererID);
	void disconnectView(View *theView);
	void disconnectRenderer(RendererHost *theRenderer);

	bool isConnected(View *theView);
	bool isConnected(RendererHost *theRenderer);

	std::string getNextRendererID();

private:
	void disconnect(View *theView, RendererHost *theRenderer);

	Package* constructViewDisconnectPackage(View *theView);
	Package* constructRendererDisconnectPackage(RendererHost *theRenderer);
	Package* constructViewConnectPackage(View *theView, RendererHost *theRenderer);
	Package* constructRendererConnectPackage(RendererHost *theRenderer, View *theView);

	std::map<std::pair<std::string, ViewerHost*>, View*> *views;
	std::map<std::string, RendererHost*> *renderers;

	std::map<View*, RendererHost*> *rendererByView;
	std::map<RendererHost*, View*> *viewByRenderer;

	unsigned int nextRendererNumber;
};

# endif /*DISPLAY_MANAGER_H*/
