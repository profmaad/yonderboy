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
	Package(Package *thePackage);
	virtual ~Package();

	PackageType getType() const { return type; }
	unsigned long long getID() const;
	bool hasID() const;
	bool isValid() const { return valid; }
	bool needsAcknowledgement() const { return acknowledgementNeeded; }

	std::string getValue(std::string key) const;
	bool isSet(std::string key) const;
	bool hasValue(std::string key) const;

	std::string serialize() const;
	
	static std::string& trimString(std::string &line);

protected:
	std::map<std::string, std::string> *keyValueMap;

private:
	void initialize();
	std::map<std::string, std::string>* constructKeyValueMap(std::string &serializedData);
	PackageType extractType(std::map<std::string, std::string> *keyValueMap);
	void convertID();

	PackageType type;
	bool valid;
	bool acknowledgementNeeded;
	unsigned long long id;
	bool hasValidID;
};

# endif /*PACKAGE_H*/
