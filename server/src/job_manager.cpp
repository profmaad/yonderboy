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

# include "log.h"
# include "macros.h"

# include "abstract_host.h"
# include "package.h"
# include "job.h"

# include "job_manager.h"

JobManager::JobManager() : unfinishedJobs(NULL)
{
	unfinishedJobs = new std::map<std::pair<AbstractHost*, std::string>, Job*>();

	LOG_INFO("initialized");
}
JobManager::~JobManager()
{
	delete unfinishedJobs;
}

void JobManager::addJob(Job *theJob)
{
	if(!theJob) { return; }

	unfinishedJobs->insert(std::make_pair(std::make_pair(theJob->getHost(), theJob->getID()), theJob));
}
Job* JobManager::processSendPackage(AbstractHost *host, Package *thePackage)
{
	Job *result = NULL;
	
	if(thePackage && host && thePackage->isValid() && thePackage->needsAcknowledgement())
	{
		result = new Job(host, thePackage, true);
		unfinishedJobs->insert(std::make_pair(std::make_pair(host, thePackage->getID()), result));
		
		LOG_INFO("received new external job "<<thePackage->getID()<<"@"<<host);
	}

	return result;
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
void JobManager::finishJob(Job *theJob)
{
	for(std::vector<Job*>::iterator iter = theJob->dependentJobs->begin(); iter != theJob->dependentJobs->end(); ++iter)
	{
		(*iter)->removeDependency(theJob);
		if((*iter)->usesDependencies() && !(*iter)->hasDependencies())
		{
			finishJob(*iter);
		}
	}

	delete theJob;

	LOG_INFO("finished job "<<theJob->getID()<<"@"<<theJob->getHost());
}

Job* JobManager::retrieveJob(AbstractHost *host, std::string id)
{
	Job *result = NULL;

	std::map<std::pair<AbstractHost*, std::string>, Job*>::const_iterator iter = unfinishedJobs->find(std::make_pair(host,id));
	if(iter != unfinishedJobs->end())
	{
		result = iter->second;
	}

	return result;
}
