//      renderer_controller.cpp
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
# include <sstream>
# include <stdexcept>
# include <iostream>

# include <cerrno>
# include <cstring>
# include <sys/types.h>
# include <sys/socket.h>
# include <unistd.h>

# include "renderer_controller.h"

# include "defaults.h"
# include <ev_cpp.h>

# include <package.h>
# include <package_factories.h>

RendererController::RendererController(int socket) : ClientController(socket), backendWebView(NULL), backendPlug(NULL)
{
// initialize webkit backend (webview inside plug)
	backendWebView = webkit_web_view_new();
	backendScrolledWindow = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(backendScrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	backendPlug = gtk_plug_new(0);
	gtk_container_add(GTK_CONTAINER(backendScrolledWindow), backendWebView);
	gtk_container_add(GTK_CONTAINER(backendPlug), backendScrolledWindow);

	// setup signals
	g_signal_connect_swapped(backendPlug, "embedded", G_CALLBACK(&RendererController::plugEmbeddedCallback),this);

	g_signal_connect_swapped(backendWebView, "notify::load-status", G_CALLBACK(&RendererController::loadStatusCallback),this);
	g_signal_connect_swapped(backendWebView, "notify::progress", G_CALLBACK(&RendererController::progressCallback),this);
	g_signal_connect_swapped(backendWebView, "hovering-over-link", G_CALLBACK(&RendererController::hoveringLinkCallback),this);
	g_signal_connect_swapped(backendWebView, "navigation-policy-decision-requested", G_CALLBACK(&RendererController::navigationPolicyDecisionCallback),this);

	// show everything
	gtk_widget_show_all(backendPlug);

	// init scalars
	lastProgress = 1;

	// send init package
	std::stringstream conversionStream;
	conversionStream<<gtk_plug_get_id(GTK_PLUG(backendPlug));
	
	Package* initPackage = constructPackage("connection-management", "id", getNextPackageID().c_str(), "command", "initialize", "client-name", PROJECT_NAME, "client-version", PROJECT_VERSION, "backend-name", BACKEND_NAME, "backend-version", BACKEND_VERSION, "display-information-type", DISPLAY_INFORMATION_TYPE, "display-information", conversionStream.str().c_str(), NULL);
	sendPackageAndDelete(initPackage);
}
RendererController::~RendererController()
{
	
}

void RendererController::handlePackage(Package *thePackage)
{
	std::cerr<<"___WEBKITRENDERER::RendererController___got Package with id "<<thePackage->getID()<<std::endl;

	std::string error;

	switch(thePackage->getType())
	{
	case Command:
		error = handleCommand(thePackage);
		sendPackageAndDelete(constructAcknowledgementPackage(thePackage, error));
		break;
	case Request:
		handleRequest(thePackage);
		break;
	case Response:
		break;
	case ConnectionManagement:
		break;
	}

	delete thePackage;
}
std::string RendererController::handleCommand(Package *thePackage)
{
	std::string command = thePackage->getValue("command");
	WebKitWebView *view = WEBKIT_WEB_VIEW(backendWebView);

	std::string error;

	if(command == "open-uri" && thePackage->hasValue("uri"))
	{
		webkit_web_view_load_uri(view, thePackage->getValue("uri").c_str());
	}
	else if(command == "reload-page")
	{
		if(thePackage->isSet("bypass-cache"))
		{
			webkit_web_view_reload_bypass_cache(view);
		}
		else
		{
			webkit_web_view_reload(view);
		}
	}
	else if(command == "go-back")
	{
		webkit_web_view_go_back(view);
	}
	else if(command == "go-forward")
	{
		webkit_web_view_go_forward(view);
	}
	else if(command == "search-in-page" && thePackage->hasValue("text"))
	{
		gboolean result = webkit_web_view_search_text(view, thePackage->getValue("text").c_str(), thePackage->isSet("case-sensitive"), !(thePackage->isSet("backwards")), thePackage->isSet("wrap-around"));
		if(!result) { error = "failed"; }
	}
	else
	{
		error = "invalid";
	}

	return error;
}	
void RendererController::handleRequest(Package *thePackage)
{
	std::string request = thePackage->getValue("request-type");
	WebKitWebView *view = WEBKIT_WEB_VIEW(backendWebView);

	if(request == "search-in-page" && thePackage->hasValue("text"))
	{
		gboolean result = webkit_web_view_search_text(view, thePackage->getValue("text").c_str(), thePackage->isSet("case-sensitive"), !(thePackage->isSet("backwards")), thePackage->isSet("wrap-around"));
		
		if(result)
		{
			Package *response = constructPackage("response", "id", thePackage->getValue("id").c_str(), "text-found", "", NULL);
			sendPackageAndDelete(response);
		}
		else
		{			
			sendPackageAndDelete(constructPackage("response", "id", thePackage->getValue("id").c_str(), NULL));
		}
	}
}

void RendererController::signalSocketClosed()
{
	std::cerr<<"___WEBKITRENDERER___going down in signalSocketClosed"<<std::endl;

	closeServerConnection();

	gtk_main_quit();
}

void RendererController::plugEmbeddedCallback(GtkPlug *plug)
{
	std::cerr<<"___WEBKITRENDERER___plug got embedded into socket"<<std::endl;
}
void RendererController::loadStatusCallback()
{
	Package *statusPackage;

	switch(webkit_web_view_get_load_status(WEBKIT_WEB_VIEW(backendWebView)))
	{
	case WEBKIT_LOAD_COMMITTED:
		statusPackage = constructPackage("status-change", "status", "load-started", "uri", webkit_web_view_get_uri(WEBKIT_WEB_VIEW(backendWebView)), NULL);
		break;
	case WEBKIT_LOAD_FINISHED:
		statusPackage = constructPackage("status-change", "status", "load-finished", "uri", webkit_web_view_get_uri(WEBKIT_WEB_VIEW(backendWebView)), NULL);
		break;
	case WEBKIT_LOAD_FAILED:
		statusPackage = constructPackage("status-change", "status", "load-failed", NULL);
		break;
	}

	if(statusPackage) { sendPackageAndDelete(statusPackage); }
}
void RendererController::progressCallback()
{
	double progress = webkit_web_view_get_progress(WEBKIT_WEB_VIEW(backendWebView));

	if(progress < lastProgress || progress >= (lastProgress + 0.1))
	{
		std::ostringstream conversionStream;
		conversionStream<<progress;

		sendPackageAndDelete(constructPackage("status-change", "status", "progress-changed", "progress", conversionStream.str().c_str(), NULL));

		lastProgress = progress;
	}
}
void RendererController::hoveringLinkCallback(gchar *title, gchar *uri, WebKitWebView *view)
{
	if(uri && title)
	{
		sendPackageAndDelete(constructPackage("status-change", "status", "hovering-over-link", "uri", uri, "title", title, NULL));
	}
	else if(uri)
	{
		sendPackageAndDelete(constructPackage("status-change", "status", "hovering-over-link", "uri", uri, NULL));
	}
	else
	{
		sendPackageAndDelete(constructPackage("status-change", "status", "not-hovering-over-link", NULL));
	}
}
gboolean RendererController::navigationPolicyDecisionCallback(WebKitWebFrame *frame, WebKitNetworkRequest *request, WebKitWebNavigationAction *navigationAction, WebKitWebPolicyDecision *policyDecision, WebKitWebView *view)
{
	if(webkit_web_navigation_action_get_reason(navigationAction) !=  WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED) { return false; }
	if(webkit_web_navigation_action_get_button(navigationAction) == 2 || (webkit_web_navigation_action_get_button(navigationAction) == 1 && webkit_web_navigation_action_get_modifier_state(navigationAction) == GDK_CONTROL_MASK))
	{
		std::cerr<<"___WEBKITRENDERER__OPEN NEW TAB FOR: "<<webkit_network_request_get_uri(request)<<std::endl;

		sendPackageAndDelete(constructPackage("connection-management", "command", "new-renderer-requested", "uri", webkit_network_request_get_uri(request), NULL));

		webkit_web_policy_decision_ignore(policyDecision);
		
		return true;
	}

	return false;
}
