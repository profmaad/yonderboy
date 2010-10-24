//      view.cpp
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

# include "log.h"
# include "macros.h"

# include "viewer_host.h"
# include "package.h"

# include "display_manager.h"

# include "view.h"

View::View(ViewerHost *host, Package *infos) : host(host), reassignable(false), popup(false), assigned(false)
{
	reassignable = infos->isSet("can-be-reassigned");
	popup = infos->isSet("is-popup");
	id = infos->getValue("view-id");
	displayInformation = infos->getValue("display-information");
	displayInformationType = infos->getValue("display-information-type");

	if(isValid())
	{
		server->displayManagerInstance()->registerView(this);

		LOG_INFO("new view '"<<id<<"' created for viewer "<<host<<", its "<<(popup?"":"not ")<<"a popup and its "<<(reassignable?"":"not ")<<"reassignable");
	}
}
View::~View()
{
	server->displayManagerInstance()->unregisterView(this);

	LOG_INFO("view '"<<id<<"' destroyed");
}

bool View::isValid()
{
//	return !id.empty() && !displayInformation.empty() && !displayInformationType.empty() && host;
	return true;
}
