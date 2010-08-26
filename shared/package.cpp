//      package.cpp
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

# include <iostream>
# include <string>
# include <sstream>
# include <map>

# include "macros.h"
# include "package.h"

Package::Package(std::string serializedData) : type(Unknown), keyValueMap(NULL), valid(false), acknowledgementNeeded(false), id(0), hasValidID(false)
{
	keyValueMap = constructKeyValueMap(serializedData);
	
	initialize();
}
Package::Package(std::map<std::string, std::string> *kvMap) : type(Unknown), keyValueMap(NULL), valid(false), acknowledgementNeeded(false), id(0), hasValidID(false)
{
	if(!kvMap)
	{
		keyValueMap = new std::map<std::string, std::string>();
	}
	else
	{
		keyValueMap = kvMap;
	}

	initialize();
}
Package::Package(Package *thePackage) : type(Unknown), keyValueMap(NULL), valid(false), acknowledgementNeeded(false), id(0), hasValidID(false)
{
	type = thePackage->type;
	valid = thePackage->valid;
	acknowledgementNeeded = thePackage->acknowledgementNeeded;
	id = thePackage->getID();
	hasValidID = thePackage->hasID();

	keyValueMap = new std::map<std::string, std::string>(*(thePackage->keyValueMap));

	initialize();
}
void Package::initialize()
{
	type = extractType(keyValueMap);
	acknowledgementNeeded = false;
	convertID();

	switch(type)
	{
	case Command:
		valid = isSet("command");
		acknowledgementNeeded = hasID();
		break;
	case StatusChange:
		valid = isSet("status");
		break;
	case Request:
		valid = isSet("request-type") && hasID();
		break;
	case Response:
		valid = hasID();
		break;
	case Acknowledgement:
		valid = hasID();
		break;
	case ConnectionManagement:
		valid = isSet("command") && hasID();
		acknowledgementNeeded = true;
		break;
	default:
		valid = false;
		acknowledgementNeeded = false;
	}
}
Package::~Package()
{
	delete keyValueMap;
}

std::string Package::getValue(std::string key) const
{
	std::map<std::string, std::string>::const_iterator iter = keyValueMap->find(key);

	if(iter != keyValueMap->end()) { return iter->second; }

	return "";
}
bool Package::isSet(std::string key) const
{
	std::map<std::string, std::string>::const_iterator iter = keyValueMap->find(key);

	if(iter != keyValueMap->end()) { return true; }

	return false;
}
bool Package::hasValue(std::string key) const
{
	if(isSet(key)) { return getValue(key).size() > 0; }
	else { return false; }
}
unsigned long long Package::getID() const
{
	return id;
}
bool Package::hasID() const
{
	return hasValidID;
}

std::string Package::serialize() const
{
	std::string result;

	for(std::map<std::string, std::string>::const_iterator iter = keyValueMap->begin(); iter != keyValueMap->end(); ++iter)
	{
		result += iter->first;
		result += " = ";
		result += iter->second;
		result += '\n';
	}

	return result;
}

std::map<std::string, std::string>* Package::constructKeyValueMap(std::string &serializedData)
{
	std::map<std::string,std::string> *result = new std::map<std::string, std::string>();
	std::string line;
	size_t lastNewline = std::string::npos;
	size_t separatorPosition = std::string::npos;

	while((lastNewline = serializedData.find("\n")) != std::string::npos)
	{
		line = serializedData.substr(0,lastNewline);
		serializedData.erase(0,lastNewline+1);

		separatorPosition = line.find("=");

		if(separatorPosition != std::string::npos)
		{
			std::string keyString = line.substr(0,separatorPosition);
			std::string valueString = line.substr(separatorPosition+1);

			result->insert(std::make_pair(trimString(keyString),trimString(valueString)));
		}
		else
		{
			result->insert(std::make_pair(trimString(line),std::string()));
		}
	}

	return result;
}
PackageType Package::extractType(std::map<std::string, std::string> *kvMap)
{
	PackageType result = Unknown;

	std::map<std::string, std::string>::iterator iter = kvMap->find("type");
	if(iter != kvMap->end())
	{
		if(iter->second == "command") { result = Command; }
		else if(iter->second == "status-change") { result = StatusChange; }
		else if(iter->second == "request") { result = Request; }
		else if(iter->second == "response") { result = Response; }
		else if(iter->second == "ack") { result = Acknowledgement; }
		else if(iter->second == "connection-management") { result = ConnectionManagement; }
	}

	return result;
}
void Package::convertID()
{
	std::stringstream conversionStream(getValue("id"));

	hasValidID = !(conversionStream >> id).fail();
}

std::string& Package::trimString(std::string &line)
{
	size_t lastPosition = std::string::npos;

	// left trim
	lastPosition = line.find_first_not_of(" \t\n\r\f\v");
	if(lastPosition != std::string::npos) { line.erase(0,lastPosition); }

	// right trim
	lastPosition = line.find_last_not_of(" \t\n\r\f\v");
	if(lastPosition != std::string::npos) { line.erase(lastPosition+1); }

	return line;
}
