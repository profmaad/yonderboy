//      status_change_package.h
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

# ifndef STATUS_CHANGE_PACKAGE_H
# define STATUS_CHANGE_PACKAGE_H

# include <string>

# include "macros.h"

# include "package.h"

class AbstractHost;

class StatusChangePackage : public Package
{
public:
	StatusChangePackage(Package *originalPackage, AbstractHost *source);
	ServerComponent getSourceType() { return sourceType; }
	std::string getSourceID();
	
private:
	ServerComponent sourceType;
};

# endif /*STATUS_CHANGE_PACKAGE_H*/
