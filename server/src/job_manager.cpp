//      job_manager.cpp
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
# include <algorithm>

# include "log.h"
# include "macros.h"

# include "abstract_host.h"
# include "package.h"
# include "package_factories.h"
# include "job.h"

# include "job_manager.h"

JobManager::JobManager() : unfinishedJobs(NULL)
{
	unfinishedJobs = new std::map<std::pair<AbstractHost*, unsigned long long>, Job*>();

	LOG_INFO("initialized");
}
JobManager::~JobManager()
{
	for(std::map<std::pair<AbstractHost*, unsigned long long>, Job*>::iterator iter = unfinishedJobs->begin(); iter != unfinishedJobs->end(); ++iter)
	{
		delete iter->second;
	}

	delete unfinishedJobs;
}

void JobManager::addJob(Job *theJob)
{
	if(!theJob) { return; }

	unfinishedJobs->insert(std::make_pair(std::make_pair(theJob->getHost(), theJob->getID()), theJob));
	
	LOG_DEBUG("got new job "<<theJob->getID()<<"@"<<theJob->getHost());	
}

void JobManager::addDependency(Job *dependentJob, Job *dependency)
{
	dependentJob->addDependency(dependency);
	dependency->addDependentJob(dependentJob);
}
void JobManager::clearDependencies(Job *dependentJob)
{
	for(std::vector<Job*>::iterator iter = dependentJob->dependencies->begin(); iter != dependentJob->dependencies->end(); ++iter)
	{
		(*iter)->removeDependentJob(dependentJob);
	}	
}

void JobManager::jobDone(Job *theJob)
{
	theJob->sendAcknowledgement();

	finishJob(theJob);
}
void JobManager::jobFailed(Job *theJob, std::string reason)
{
	theJob->getHost()->sendPackageAndDelete(constructAcknowledgementPackage(theJob, reason));
	
	finishJob(theJob);
}
void JobManager::finishJob(Job *theJob)
{
	for(std::vector<Job*>::iterator iter = theJob->dependentJobs->begin(); iter != theJob->dependentJobs->end(); ++iter)
	{
		(*iter)->removeDependency(theJob);
		if((*iter)->usesDependencies() && !(*iter)->hasDependencies())
		{
			jobDone(*iter);
		}
	}

	removeJob(theJob);	

	LOG_INFO("finished job "<<theJob->getID()<<"@"<<theJob->getHost()<<", "<<unfinishedJobs->size()<<" unfinished jobs left");

	delete theJob;
}

void JobManager::removeJob(Job *theJob)
{
	unfinishedJobs->erase(std::make_pair(theJob->getHost(),theJob->getID()));
}
Job* JobManager::retrieveJob(AbstractHost *host, unsigned long long id)
{
	Job *result = NULL;

	std::map<std::pair<AbstractHost*, unsigned long long>, Job*>::const_iterator iter = unfinishedJobs->find(std::make_pair(host,id));
	if(iter != unfinishedJobs->end())
	{
		result = iter->second;
	}

	return result;
}
