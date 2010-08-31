//      renderer_host.h
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

# ifndef RENDERER_HOST_H
# define RENDERER_HOST_H

# include <string>

# include "ev_cpp.h"
# include "abstract_host.h"

class Job;
class View;

class RendererHost : public AbstractHost
{
public:
	RendererHost(int hostSocket, View *viewToConnectTo = NULL);
	~RendererHost();
	static RendererHost* spawnRenderer(std::string binaryPath, View *viewToConnectTo = NULL);

	void doJob(Job *theJob);

	std::string getDisplayInformation() { return displayInformation; }
	std::string getDisplayInformationType() { return displayInformationType; }

protected:
	void handlePackage(Package *thePackage);

private:
	std::string displayInformation;
	std::string displayInformationType;
	std::string backendName;
	std::string backendVersion;

	View *viewToConnectTo;
};

# endif /*RENDERER_HOST_H*/
