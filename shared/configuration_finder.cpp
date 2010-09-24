//      configuration_finder.cpp
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

# include <string>
# include <vector>
# include <stdexcept>

# include <unistd.h>
# include <wordexp.h>
# include <sys/types.h>

# include "configuration_finder.h"

std::string findConfigurationFile()
{
	// setup files to look at
	// it is somewhat dumb to reinit this every time this function gets called
	// well, guess we shouldn't call it to often then ;-)

	const char *options[] = { "$XDG_CONFIG_HOME/yonderboy/config", "$HOME/.config/yonderboy/config", "$HOME/.yonderboyrc", "/etc/yonderboy/config" }; //HC
	unsigned int optionsCount = 4; //HC

	for(int i=0;i<optionsCount;i++)
	{
		wordexp_t expansion;
		int result = 0;		

		result = wordexp(options[i], &expansion, (WRDE_NOCMD | WRDE_UNDEF));
		if(result != 0) { continue; }
		if(expansion.we_wordc < 1) { wordfree(&expansion); continue; }

		result = access(expansion.we_wordv[0], R_OK);
		if(result < 0) { wordfree(&expansion); continue; }

		std::string configFile = std::string(expansion.we_wordv[0]);

		wordfree(&expansion);

		return configFile;
	}

	// if we reach this, we didn't find a config file
	throw std::runtime_error("no configuration file found");
}
