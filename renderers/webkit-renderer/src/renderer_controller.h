//      renderer_controller.h
//      
//      Copyright 2010 Prof. MAAD <prof.maad@lambda-bb.de>
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

# ifndef RENDERER_CONTROLLER_H
# define RENDERER_CONTROLLER_H

# include <string>

# include <gtk/gtk.h>
# include <webkit/webkit.h>

# include <client_controller.h>

class Package;

class RendererController : public ClientController
{
public:
	RendererController(int socket);
	~RendererController();

protected:
	void handlePackage(Package *thePackage);
	void signalSocketClosed();

private:
	std::string handleCommand(Package *thePackage);	
	void handleRequest(Package *thePackage);

	// Callbacks
	void plugEmbeddedCallback(GtkPlug *plug);
	void loadStatusCallback();
	void progressCallback();
	void hoveringLinkCallback(WebKitWebView *view, gchar *uri, gchar *title);

	GtkWidget *backendPlug;
	GtkWidget *backendScrolledWindow;
	GtkWidget *backendWebView;

	gdouble lastProgress;
};

# endif
