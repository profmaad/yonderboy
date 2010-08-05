//      job_manager.h
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

# ifndef JOB_MANAGER_H
# define JOB_MANAGER_H

# include <string>
# include <map>

class AbstractHost;
class Package;
class Job;

class JobManager
{
public:
	JobManager();
	~JobManager();

	void addJob(Job *theJob);

	void addDependency(Job *dependentJob, Job *dependency);
	void clearDependencies(Job *dependentJob);

	void jobDone(Job *theJob);
	void jobFailed(Job *theJob, std::string reason);
	void finishJob(Job *theJob);

	Job* retrieveJob(AbstractHost *host, unsigned long long);
	
private:
	void removeJob(Job *theJob);
	std::map<std::pair<AbstractHost*, unsigned long long>, Job*> *unfinishedJobs;
};

# endif /*JOB_MANAGER_H*/
