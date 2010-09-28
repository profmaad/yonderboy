//      viewer_host.h
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

# ifndef VIEWER_HOST_H
# define VIEWER_HOST_H

# include <map>
# include <string>

# include "ev_cpp.h"
# include "abstract_host.h"

class View;
class Job;

class ViewerHost : public AbstractHost
{
public:
	ViewerHost(int hostSocket);
	~ViewerHost();

	void doJob(Job *theJob);
	
	void createView();

protected:
	void handlePackage(Package *thePackage);

private:
	View* retrieveView(std::string viewID);

	bool displaysStati;
	bool displaysPopups;
	bool canHaveMultipleViews;

	std::map<std::string, View*> *views;
};

# endif /*VIEWER_HOST_H*/
