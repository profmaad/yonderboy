//      package_factories.cpp
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
# include <utility>
# include <sstream>

# include <cstdarg>

# include "macros.h"
# include "package.h"

# include "package_factories.h"

Package* constructAcknowledgementPackage(unsigned long long id, std::string error)
{
	Package *result = NULL;
	std::map<std::string, std::string> *kvMap = new std::map<std::string, std::string>();
	kvMap->insert(std::make_pair("type", "ack"));

	std::stringstream conversionStream;
	conversionStream<<id;
	kvMap->insert(std::make_pair("id",conversionStream.str()));
	
	if(!error.empty()) { kvMap->insert(std::make_pair("error", error)); }

	result = new Package(kvMap);

	return result;
}
Package* constructAcknowledgementPackage(Package *packageToAcknowledge, std::string error)
{
	if(packageToAcknowledge->hasID()) { return constructAcknowledgementPackage(packageToAcknowledge->getID(), error); }
	else return NULL;
}
Package* constructPackage(char *type, ...)
{
	Package *result = NULL;
	std::map<std::string, std::string> *kvMap = new std::map<std::string, std::string>();
	
	kvMap->insert(std::make_pair("type",std::string(type)));

	// get variable argument list
	char *key = NULL;
	char *value = NULL;

	va_list vaList;
	va_start(vaList, type);
	while( (key = va_arg(vaList, char*)) != NULL && (value = va_arg(vaList, char*)) != NULL)
	{
		kvMap->insert(std::make_pair(std::string(key), std::string(value)));
	}
	va_end(vaList);

	result = new Package(kvMap);
	
	return result;
}
