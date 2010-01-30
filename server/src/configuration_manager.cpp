//      configuration_manager.cpp
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

# include <map>
# include <string>
# include <vector>
# include <utility>
# include <stdexcept>
# include <sstream>

# include "macros.h"
# include "log.h"

# include "server_controller.h"
# include "file_persistence_manager.h"
# include "persistent_storage.h"
# include "package.h"
# include "job.h"

# include "configuration_manager.h"

ConfigurationManager::ConfigurationManager(std::string configFile) : persistenceManager(NULL), storage(NULL)
{
	configBasePath = getBasePath(configFile);
	configFileName = getFileName(configFile);
	
	persistenceManager = new FilePersistenceManager(configBasePath, false);
	if(!persistenceManager)
	{
		LOG_ERROR("failed to create file persistence manager for path "<<configBasePath)
		throw std::runtime_error("failed to create file persistence manager");
	}
	
	storage = persistenceManager->retrieveKeyValueStorage("", configFileName);
	if(!storage)
	{
		persistenceManager->close();
		delete persistenceManager;
		LOG_ERROR("failed to retrieve storage for configuration file "<<configFile)
		throw std::runtime_error("failed to retrieve storage for configuration file");
	}
	
	entries = new EntriesMap();
	retrieveEntries();
}
ConfigurationManager::~ConfigurationManager()
{
	persistenceManager->releaseStorage("", configFileName);
	persistenceManager->close();
	delete persistenceManager;
}

void ConfigurationManager::doJob(Job *theJob)
{
}

void ConfigurationManager::retrieveEntries()
{
	std::vector<KeyType> storageKeys = storage->listKeys();
	
	for(std::vector<KeyType>::const_iterator iter = storageKeys.begin(); iter != storageKeys.end(); ++iter)
	{
		KeyValueRecord record = storage->retrieve(*iter);
		
		entries->insert(std::make_pair(getKeyPair(record.first), record.second));
	}
}

bool ConfigurationManager::isSet(std::string namespaceName, std::string identifier)
{
	bool result = false;
	
	EntriesMap::const_iterator iter = entries->find(std::make_pair(namespaceName, identifier));
	if(iter != entries->end())
	{
		result = true;
	}
	
	return result;
}
std::string ConfigurationManager::retrieve(std::string namespaceName, std::string identifier, std::string defaultValue)
{
	std::string result = defaultValue;
	
	EntriesMap::const_iterator iter = entries->find(std::make_pair(namespaceName, identifier));
	if(iter != entries->end())
	{
		result = iter->second;
	}
	
	return result;
}
bool ConfigurationManager::retrieveAsBool(std::string namespaceName, std::string identifier, bool defaultValue)
{
	bool result = defaultValue;
	
	EntriesMap::const_iterator iter = entries->find(std::make_pair(namespaceName, identifier));
	if(iter != entries->end())
	{
		result = valueAsBool(iter->second,defaultValue);
	}
	
	return result;
}
long long ConfigurationManager::retrieveAsLongLong(std::string namespaceName, std::string identifier, long long defaultValue)
{
	long long result = defaultValue;
	
	EntriesMap::const_iterator iter = entries->find(std::make_pair(namespaceName, identifier));
	if(iter != entries->end())
	{
		result = valueAsLongLong(iter->second,defaultValue);
	}
	
	return result;
}
double ConfigurationManager::retrieveAsDouble(std::string namespaceName, std::string identifier, double defaultValue)
{
	double result = defaultValue;
	
	EntriesMap::const_iterator iter = entries->find(std::make_pair(namespaceName, identifier));
	if(iter != entries->end())
	{
		result = valueAsDouble(iter->second,defaultValue);
	}
	
	return result;
}
LogLevel ConfigurationManager::retrieveAsLogLevel(std::string namespaceName, std::string identifier, LogLevel defaultValue)
{
	LogLevel result = defaultValue;
	
	EntriesMap::const_iterator iter = entries->find(std::make_pair(namespaceName, identifier));
	if(iter != entries->end())
	{
		result = valueAsLogLevel(iter->second,defaultValue);
	}
	
	return result;
}

std::string ConfigurationManager::getBasePath(std::string path)
{
	size_t lastSlash = path.find_last_of('/');
	
	if(lastSlash == std::string::npos)
	{
		return "";
	}
	else
	{
		return path.substr(0,lastSlash);
	}
}
std::string ConfigurationManager::getFileName(std::string path)
{
	size_t lastSlash = path.find_last_of('/');
	
	if(lastSlash == std::string::npos)
	{
		return path;
	}
	else
	{
		return path.substr(lastSlash+1);
	}
}
std::pair<std::string, std::string> ConfigurationManager::getKeyPair(std::string key)
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


bool ConfigurationManager::valueAsBool(std::string value, bool defaultValue)
{
	bool result = defaultValue;
	
	Package::trimString(value);
	
	if(value == "true" || value == "t" || value == "1")
	{
		result = true;
	}
	
	return result;
}
long long ConfigurationManager::valueAsLongLong(std::string value, long long defaultValue)
{
	long long result = defaultValue;
	std::stringstream conversionStream(value);
	
	conversionStream >> result; 
	
	return result;
}
double ConfigurationManager::valueAsDouble(std::string value, double defaultValue)
{
	double result = defaultValue;
	std::stringstream conversionStream(value);
	
	conversionStream >> result;
	
	return result;
}
LogLevel ConfigurationManager::valueAsLogLevel(std::string value, LogLevel defaultValue)
{
	LogLevel result = defaultValue;
	
	Package::trimString(value);
	
	if(value == "debug") { result = LogLevelDebug; }
	else if(value == "info") { result = LogLevelInfo; }
	else if(value == "warning") { result = LogLevelWarning; }
	else if(value == "error") { result = LogLevelError; }
	else if(value == "fatal") { result = LogLevelFatal; }
	
	return result;
}
