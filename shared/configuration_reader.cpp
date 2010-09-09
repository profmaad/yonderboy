//      configuration_reader.cpp
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

# include <map>
# include <string>
# include <vector>
# include <utility>
# include <stdexcept>
# include <sstream>
# include <fstream>
# include <iostream>

# include "macros.h"

# include "package.h"

# include "configuration_reader.h"

ConfigurationReader::ConfigurationReader(std::string configFile) : configFile(configFile), ready(false)
{
	entries = new EntriesMap();
	ready = retrieveEntries();
	workingDir = retrieve("server", "working-dir"); //HC
}
ConfigurationReader::~ConfigurationReader()
{
	delete entries;
}

bool ConfigurationReader::retrieveEntries()
{
	std::string line;
	std::ifstream fileStream(configFile.c_str());
	if(fileStream.is_open())
	{
		while(!fileStream.eof())
		{
			getline(fileStream, line);
			std::pair<std::string, std::string> record = parseKeyValueRecord(line);

			entries->insert(std::make_pair(getKeyPair(record.first), record.second));
		}
	}
	else
	{
		std::cerr<<"failed to open config file at '"<<configFile<<"'"<<std::endl;
		return false;
	}

	fileStream.close();
	return true;
}
std::pair<std::string, std::string> ConfigurationReader::parseKeyValueRecord(std::string record)
{
	size_t separatorPosition = std::string::npos;
	
	separatorPosition = record.find("=");
	if(separatorPosition != std::string::npos)
	{
		std::string keyString = record.substr(0,separatorPosition);
		std::string valueString = record.substr(separatorPosition+1);
		
		return std::make_pair(Package::trimString(keyString), Package::trimString(valueString));
	}
	
	return std::make_pair(Package::trimString(record), std::string());
}

bool ConfigurationReader::isSet(std::string namespaceName, std::string identifier)
{
	bool result = false;
	
	EntriesMap::const_iterator iter = entries->find(std::make_pair(namespaceName, identifier));
	if(iter != entries->end())
	{
		result = true;
	}
	
	return result;
}
std::string ConfigurationReader::retrieve(std::string namespaceName, std::string identifier, std::string defaultValue)
{
	std::string result = defaultValue;
	
	EntriesMap::const_iterator iter = entries->find(std::make_pair(namespaceName, identifier));
	if(iter != entries->end())
	{
		result = iter->second;
	}
	
	return result;
}
std::string ConfigurationReader::retrieveAsAbsolutePath(std::string namespaceName, std::string identifier, std::string defaultValue)
{
	std::string result = defaultValue;
	
	EntriesMap::const_iterator iter = entries->find(std::make_pair(namespaceName, identifier));
	if(iter != entries->end())
	{
		result = workingDir;
		result += iter->second;
	}
	
	return result;	
}
bool ConfigurationReader::retrieveAsBool(std::string namespaceName, std::string identifier, bool defaultValue)
{
	bool result = defaultValue;
	
	EntriesMap::const_iterator iter = entries->find(std::make_pair(namespaceName, identifier));
	if(iter != entries->end())
	{
		result = valueAsBool(iter->second,defaultValue);
	}
	
	return result;
}
long long ConfigurationReader::retrieveAsLongLong(std::string namespaceName, std::string identifier, long long defaultValue)
{
	long long result = defaultValue;
	
	EntriesMap::const_iterator iter = entries->find(std::make_pair(namespaceName, identifier));
	if(iter != entries->end())
	{
		result = valueAsLongLong(iter->second,defaultValue);
	}
	
	return result;
}
double ConfigurationReader::retrieveAsDouble(std::string namespaceName, std::string identifier, double defaultValue)
{
	double result = defaultValue;
	
	EntriesMap::const_iterator iter = entries->find(std::make_pair(namespaceName, identifier));
	if(iter != entries->end())
	{
		result = valueAsDouble(iter->second,defaultValue);
	}
	
	return result;
}

std::pair<std::string, std::string> ConfigurationReader::getKeyPair(std::string key)
{
	std::string namespacePart;
	std::string identifierPart;
	size_t firstDot = -1;
	
	firstDot = key.find_first_of('.');
	
	if(firstDot == std::string::npos)
	{
		return std::pair<std::string, std::string>("",key);
	}
	else
	{
		return std::pair<std::string, std::string>(key.substr(0,firstDot),key.substr(firstDot+1));
	}
}

bool ConfigurationReader::valueAsBool(std::string value, bool defaultValue)
{
	bool result = defaultValue;
	
	Package::trimString(value);
	
	if(value == "true" || value == "t" || value == "1")
	{
		result = true;
	}
	
	return result;
}
long long ConfigurationReader::valueAsLongLong(std::string value, long long defaultValue)
{
	long long result = defaultValue;
	std::stringstream conversionStream(value);
	
	conversionStream >> result; 
	
	return result;
}
double ConfigurationReader::valueAsDouble(std::string value, double defaultValue)
{
	double result = defaultValue;
	std::stringstream conversionStream(value);
	
	conversionStream >> result;
	
	return result;
}
