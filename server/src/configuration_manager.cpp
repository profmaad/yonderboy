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

# include <iostream>

# include "macros.h"
# include "log.h"

# include "server_controller.h"
# include "file_persistence_manager.h"
# include "persistent_storage.h"

# include "configuration_manager.h"

ConfigurationManager::ConfigurationManager(std::string configFile) : persistenceManager(NULL), storage(NULL)
{
	configBasePath = getBasePath(configFile);
	configFileName = getFileName(configFile);
	
	persistenceManager = new FilePersistenceManager(configBasePath);
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
}
ConfigurationManager::~ConfigurationManager()
{
	persistenceManager->releaseStorage("", configFileName);
	persistenceManager->close();
	delete persistenceManager;
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