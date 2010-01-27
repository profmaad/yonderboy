//      job.h
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

# ifndef JOB_H
# define JOB_H

# include <vector>
# include <string>

# include "macros.h"

class AbstractHost;
class Package;

class Job
{
public:
	Job(AbstractHost *host, Package *originalPackage, bool external);
	~Job();

	bool usesDependencies() const { return dependenciesUsed; }
	bool hasDependencies() const;
	bool hasDependentJobs() const;

	const Package* getPackage() const { return originalPackage; }
	std::string getID() const { return originalPackage->getID(); }
	PackageType getType() const { return originalPackage->getType(); }
	AbstractHost* getHost() const { return host; }
	bool isExternal() const { return external; }

private:
	void addDependency(Job *dependency);
	void removeDependency(Job *dependency);
	void clearDependencies();
	void addDependentJob(Job *dependentJob);
	void removeDependentJob(Job *dependentJob);

	void sendAcknowledgement();

	AbstractHost *host;
	bool external;
	const Package *originalPackage;

	std::vector<Job*> *dependencies;
	std::vector<Job*> *dependentJobs;
	bool dependenciesUsed;

	friend class JobManager;
};

# endif /*JOB_H*/