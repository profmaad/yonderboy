//      configuration_reader.h
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

# ifndef CONFIGURATION_READER_H
# define CONFIGURATION_READER_H

# include <map>
# include <string>
# include <vector>
# include <utility>

# include "macros.h"

typedef std::map<std::pair<std::string, std::string>, std::string> EntriesMap;

class ConfigurationReader
{
public:
	ConfigurationReader(std::string configFile);
	~ConfigurationReader();
	bool isReady() { return ready; }

	bool isSet(std::string namespaceName, std::string identifier);
	
	std::string retrieve(std::string namespaceName, std::string identifier, std::string defaultValue = "");
	bool retrieveAsBool(std::string namespaceName, std::string identifier, bool defaultValue = false);
	long long retrieveAsLongLong(std::string namespaceName, std::string identifier, long long defaultValue = 0);
	double retrieveAsDouble(std::string namespaceName, std::string identifier, double defaultValue = 0.0);
	std::string retrieveAsAbsolutePath(std::string namespaceName, std::string identifier, std::string defaultValue = "");
	std::string retrieveShellExpanded(std::string namespaceName, std::string identifier, std::string defaultValue = "");
	
	static bool valueAsBool(std::string value, bool defaultValue);
	static long long valueAsLongLong(std::string value, long long defaultValue);
	static double valueAsDouble(std::string value, double defaultValue);
	
private:
	bool retrieveEntries();
	std::pair<std::string, std::string> parseKeyValueRecord(std::string record);
	
	std::pair<std::string, std::string> getKeyPair(std::string key);
	
	std::string configFile;
	EntriesMap *entries; 
	bool ready;
	std::string workingDir;
};

# endif /*CONFIGURATION_READER_H*/
