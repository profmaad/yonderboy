//      controller_host.h
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

# ifndef CONTROLLER_HOST_H
# define CONTROLLER_HOST_H

# include "ev_cpp.h"
# include "abstract_host.h"

class Job;

class ControllerHost : public AbstractHost
{
public:
	ControllerHost(int hostSocket);
	~ControllerHost();

	void doJob(Job *theJob);

	bool isInteractive() { return interactive; }
	bool canHandleSynchronousRequests() { return handlesSynchronousRequests; }
	bool canDisplayStati() { return displaysStati; }

protected:
	void handlePackage(Package *thePackage);

private:
	bool interactive;
	bool handlesSynchronousRequests;
	bool displaysStati;
};

# endif /*CONTROLLER_HOST_H*/
