//      viewer_controller.h
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

# ifndef VIEWER_CONTROLLER_H
# define VIEWER_CONTROLLER_H

# include <string>
# include <vector>
# include <map>

# include <gtk/gtk.h>
# include <vte/vte.h>

# include <client_controller.h>

class Package;

class ViewerController : public ClientController
{
public:
	ViewerController(int socket);
	~ViewerController();

protected:
	void handlePackage(Package *thePackage);
	void signalSocketClosed();

private:
	void setupHotkeys();
	void setupVTE();

	std::string handleCommand(Package *thePackage);
	void handleStatusChange(Package *thePackage);

	gint createNewTab(bool createRenderer, std::string initialURI = std::string());
	void closeTab(guint pageNum, bool keepRenderer);
	GtkSocket* retrieveSocket(std::string viewID);
	gdouble getProgressFromTab(GtkWidget *tab);
	void setProgressOnTab(GtkWidget *tab, gdouble progress);
	const char* getStatusFromTab(GtkWidget *tab);
	void setStatusOnTab(GtkWidget *tab, std::string status);
	const char* getTitleFromTab(GtkWidget *tab);
	void setTitleOnTab(GtkWidget *tab, std::string title);

	void updateStatusBar(guint currentPage);
	void updateFocusInfo(guint currentPage);

	// Callbacks
	void gtkDestroyCallback(GtkObject *object);
	void windowToplevelFocusCallback();
	void tabBarSwitchPageCallback(GtkNotebookPage *page, guint pageNum, GtkNotebook *notebook);
	void nextTabCallback();
	void previousTabCallback();
	void closeTabCallback();
	void closeTabKeepRendererCallback();
	void newTabCallback();
	void newTabWithoutRendererCallback();
	void focusTabCallback();
	void focusVTECallback();

	void pageAddedCallback(GtkWidget *child, guint pageNum, GtkNotebook *notebook);
	void pageRemovedCallback(GtkWidget *child, guint pageNum, GtkNotebook *notebook);
	void plugAddedCallback(GtkSocket *socket);
	gboolean plugRemovedCallback(GtkSocket *socket);

	// GTK stuff
	GtkWidget *mainWindow;
	GtkWidget *mainVBox;
	GtkWidget *mainVPaned;
	GtkWidget *tabBar;
	GtkWidget *statusBar;
	GtkWidget *statusBarProgress;
	guint statusBarContextLocal;
	guint statusBarContextServer;
	guint statusBarContextTab;
	GtkAccelGroup *globalHotkeys;

	// VTE stuff
	GtkWidget *terminal;

	// mappings
	std::map<std::string, GtkSocket*> *socketByID;
	std::map<GtkSocket*, std::string> *idBySocket;
	std::map<std::string, std::string> *viewByRenderer;

	// state
	bool initialised;
	std::string initID;
	unsigned long long nextViewID;
};

# endif
