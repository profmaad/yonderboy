//      package.h
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

# ifndef PACKAGE_H
# define PACKAGE_H

# include <map>
# include <string>

class Package
{
public:
	explicit Package(std::string serializatedData);
	Package(std::map<std::string, std::string> *kvMap);
	virtual ~Package();

	PackageType getType() { return type; } 
	std::string getID();
	bool hasID();
	bool isValid() { return valid; }

	std::string getValue(std::string key);
	bool isSet(std::string key);
	bool hasValue(std::string key);

	std::string serialize();

private:
	void initialize();
	std::map<std::string, std::string>* constructKeyValueMap(std::string &serializedData);
	PackageType extractType(std::map<std::string, std::string> *keyValueMap);

	std::string& trimString(std::string &line);

	PackageType type;
	bool valid;
	
	std::map<std::string, std::string> *keyValueMap;
};

# endif /*PACKAGE_H*/
