//      job.cpp
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

# include <vector>
# include <string>
# include <map>

# include "log.h"
# include "macros.h"

# include "abstract_host.h"
# include "package.h"
# include "package_factories.h"

# include "job.h"

Job::Job(AbstractHost *host, Package *originalPackage) : Package(originalPackage), dependencies(NULL), dependentJobs(NULL), dependenciesUsed(false), host(host)
{
	dependencies = new std::vector<Job*>();
	dependentJobs = new std::vector<Job*>();

	delete originalPackage;
}
Job(AbstractHost *host, Package *originalPackage, std::string newID) : Job(host, originalPackage)
{
	kvMap->erase("id");
	kvMap->insert(std::make_pair("id", newID));
}
Job::~Job()
{
	delete dependencies;
	delete dependentJobs;
}

bool Job::hasDependencies() const
{
	return !dependencies->empty();
}
bool Job::hasDependentJobs() const
{
	return !dependentJobs->empty();
}

void Job::addDependency(Job *dependency)
{
	if(!dependency) { return; }
	dependenciesUsed = true;

	dependencies->push_back(dependency);
}
void Job::removeDependency(Job *dependency)
{
	if(!dependency) { return; }

	for(std::vector<Job*>::iterator iter = dependencies->begin(); iter != dependencies->end(); ++iter)
	{
		if(*iter == dependency)
		{
			dependencies->erase(iter);
			return;
		}
	}
}
void Job::clearDependencies()
{
	dependencies->clear();
	dependenciesUsed = false;
}
void Job::addDependentJob(Job *dependentJob)
{
	if(!dependentJob) { return; }
	dependentJobs->push_back(dependentJob);
}
void Job::removeDependentJob(Job *dependentJob)
{
	if(!dependentJob) { return; }

	for(std::vector<Job*>::iterator iter = dependentJobs->begin(); iter != dependentJobs->end(); ++iter)
	{
		if(*iter == dependentJob)
		{
			dependentJobs->erase(iter);
			return;
		}
	}
}

void Job::sendAcknowledgement()
{
	Package *acknowledgement = constructAcknowledgementPackage(getID());

	if(acknowledgement)
	{
		host->sendPackage(acknowledgement);
	}
}
