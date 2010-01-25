//      view.h
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

# ifndef VIEW_H
# define VIEW_H

# include <string>

class ViewerHost;
class Package;

class View
{
public:
	View(ViewerHost *host, Package *infos);
	~View();

	std::string getID() { return id; }
	std::string getDisplayInformation() { return displayInformation; }
	std::string getDisplayInformationType() { return displayInformationType; }
	bool isReassignable() { return reassignable; }
	bool isPopup() { return popup; }
	bool isAssigned() {  return assigned; }
	ViewerHost* getHost() { return host; }

	bool isValid();

private:
	ViewerHost *host;
	
	std::string id;
	bool reassignable;
	bool popup;
	bool assigned;

	std::string displayInformation;
	std::string displayInformationType;
};       

#endif /*VIEW_H*/

