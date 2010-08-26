//      package_router.h
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

# ifndef PACKAGE_ROUTER_H
# define PACKAGE_ROUTER_H

# include <set>
# include <map>
# include <string>

# include "macros.h"

class Package;
class Job;
class AbstractHost;

class PackageRouter
{
public:
	PackageRouter(std::string specFilename);
	~PackageRouter();

	Job* processPackage(AbstractHost *host, Package *thePackage);

	bool isAllowed(ServerComponent receivingComponent, Package *thePackage);

	void addStatiReceiver(AbstractHost *host);
	void removeStatiReceiver(AbstractHost *host);

private:
	void routeJob(Job *theJob);
	void deliverStatusChange(Package *thePackage);

	static ServerComponent stringToServerComponent(std::string string);

	void addArrayToRoutingTable(ServerComponent component, const char* array[]);
	std::set<std::string>* constructSetFromArray(const char* array[]);

	std::map<std::string, ServerComponent> *routingTable;

	std::set<std::string> *allowedControllerCommands;
	std::set<std::string> *allowedRendererRequests;

	std::set<AbstractHost*> *statiReceiver;
};

# endif /*PACKAGE_ROUTER_H*/
