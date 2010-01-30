//      package_router_tables.h
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

# ifndef PACKAGE_ROUTER_TABLES_H
# define PACKAGE_ROUTER_TABLES_H

const char* routingTableArrayForDecisionMaker[] = {NULL};
const char* routingTableArrayForDisplayManager[] = {"connect-view-to-renderer", NULL};
const char* routingTableArrayForJobManager[] = {NULL};
const char* routingTableArrayForConfigurationManager[] = {NULL};
const char* routingTableArrayForPersistenceManager[] = {NULL};
const char* routingTableArrayForHostsManager[] = {"spawn-renderer", NULL};
const char* routingTableArrayForPackageRouter[] = {NULL};
const char* routingTableArrayForServerController[] = {"quit-server", NULL};
const char* routingTableArrayForRendererHost[] = {"open-uri", NULL};
const char* routingTableArrayForViewerHost[] = {NULL};
const char* routingTableArrayForControllerHost[] = {NULL};

const char* allowedControllerCommandsArray[] = {"spawn-renderer", "open-uri", "connect-view-to-renderer", "quit-server", NULL};
const char* allowedRendererRequestsArray[] = {NULL};

# endif /*PACKAGE_ROUTER_TABLES_H*/
