//      package_router.cpp
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
# include <set>
# include <fstream>

# include <yaml.h>

# include "log.h"
# include "macros.h"

# include "package.h"
# include "package_factories.h"
# include "status_change_package.h"
# include "job.h"
# include "abstract_host.h"
# include "job_manager.h"
# include "hosts_manager.h"
# include "meta_decision_maker.h"
# include "display_manager.h"
# include "configuration_manager.h"
# include "renderer_host.h"
# include "viewer_host.h"
# include "controller_host.h"

# include "package_router.h"

PackageRouter::PackageRouter(std::string specFilename) : routingTable(NULL), statiReceiver(NULL), allowedControllerCommands(NULL), allowedRendererCommands(NULL)
{
	routingTable = new std::map<std::string, ServerComponent>();
	allowedControllerCommands = new std::set<std::string>();
	allowedRendererCommands = new std::set<std::string>();
	statiReceiver = new std::set<AbstractHost*>();

	std::ifstream specFile(specFilename.c_str());
	
	if(!specFile.fail())
	{
		try
		{
			YAML::Parser specParser(specFile);
			YAML::Node specDoc;
			specParser.GetNextDocument(specDoc);
			
			// parse controller information
			for(int i=0; i<specDoc["commands"].size(); i++)
			{
				if(specDoc["commands"][i]["command"].GetType() == YAML::CT_SCALAR && specDoc["commands"][i]["target"].GetType() == YAML::CT_SCALAR && specDoc["commands"][i]["source"].GetType() == YAML::CT_SEQUENCE)
				{
					// we got a valid command, lets add it
					std::string command, target;
					specDoc["commands"][i]["command"] >> command;
					specDoc["commands"][i]["target"] >> target;
						
					routingTable->insert(std::make_pair(command, stringToServerComponent(target)));

					for(int s=0; s<specDoc["commands"][i]["source"].size(); s++)
					{
						std::string source;
						specDoc["commands"][i]["source"][s] >> source;

						if(source == "controller")
						{
							allowedControllerCommands->insert(command);
						}
						else if(source == "renderer")
						{
							allowedRendererCommands->insert(command);
						}
					}
				}
			}
		}
		catch(const YAML::Exception &e)
		{
			LOG_ERROR("error parsing network spec file: "<<e.what());
		}
	}

	specFile.close();
	
	LOG_INFO("initialized");
}
PackageRouter::~PackageRouter()
{
	delete routingTable;
	delete allowedControllerCommands;
	delete allowedRendererCommands;
	delete statiReceiver;
}

Job* PackageRouter::processPackage(AbstractHost *host, Package *thePackage)
{
	Job *result = NULL;

	if(!thePackage || !host)
	{
		return result;
	}

	if(thePackage->getType() == Acknowledgement)
	{
		LOG_DEBUG("received ack for id "<<thePackage->getID()<<" from "<<host);
		Job *acknowledgedJob = server->jobManagerInstance()->retrieveJob(host, thePackage->getID());
		if (acknowledgedJob) { server->jobManagerInstance()->jobDone(acknowledgedJob); }
		delete thePackage;
	}
	else if(thePackage->getType() == Response)
	{
		LOG_DEBUG("got response "<<thePackage->getID()<<"@"<<host);

		server->jobManagerInstance()->requestAnswered(host, thePackage);
		delete thePackage;
	}
	else if(thePackage->getType() == StatusChange)
	{
		deliverStatusChange(host, thePackage);
	}
	else
	{
		result = new Job(host, thePackage);

		if(thePackage->needsAcknowledgement() && !thePackage->getType() == Request) { server->jobManagerInstance()->addJob(result); }
		
		LOG_INFO("received new job "<<result->getID()<<"@"<<host);

		routeJob(result);
	}
	
	return result;
}

void PackageRouter::addStatiReceiver(AbstractHost *host)
{
	statiReceiver->insert(host);
}
void PackageRouter::removeStatiReceiver(AbstractHost *host)
{
	statiReceiver->erase(host);
}

bool PackageRouter::isAllowed(ServerComponent receivingComponent, Package *thePackage)
{
	if(thePackage->getType() == Acknowledgement || thePackage->getType() == ConnectionManagement) { return true; }

	if(receivingComponent == ServerComponentControllerHost)
	{
		if(thePackage->getType() == Command) 
		{
			return (allowedControllerCommands->count(thePackage->getValue("command")) > 0);
		}
		else if(thePackage->getType() == Request)
		{
			return (allowedControllerCommands->count(thePackage->getValue("request-type")) > 0);
		}
		else if(thePackage->getType() == Response) { return true; }
		else { return false; }
	}
	else if(receivingComponent == ServerComponentViewerHost)
	{
		return false;
	}
	else if(receivingComponent == ServerComponentRendererHost)
	{
		if(thePackage->getType() == Request)
		{
			return (allowedRendererCommands->count(thePackage->getValue("request-type")) > 0);
		}
		else if(thePackage->getType() == StatusChange) { return true; }
		else if(thePackage->getType() == Response) { return true; }
		else { return false; }
	}

	return false;
}

void PackageRouter::routeJob(Job *theJob)
{
	std::string targetID;
	std::map<std::string, ServerComponent>::const_iterator iter;
	RendererHost *rendererHost = NULL;
	ViewerHost *viewerHost = NULL;
	ControllerHost *controllerHost = NULL;
	Job *originalRequest = NULL;

	switch(theJob->getType())
	{
	case Command:
		iter = routingTable->find(theJob->getValue("command"));
		break;
	case Request:
		iter = routingTable->find(theJob->getValue("request-type"));
		break;
	case Response:
		originalRequest = server->jobManagerInstance()->retrieveJob(theJob->getHost(), theJob->getID());
		if(originalRequest)
		{
			originalRequest->getHost()->sendPackageAndDelete(theJob);
			server->jobManagerInstance()->finishJob(originalRequest);
		}
		else
		{
			LOG_WARNING("encountered unmatched response with id "<<theJob->getID()<<", discarding it");
			theJob->getHost()->sendPackageAndDelete(constructAcknowledgementPackage(theJob, "unmatched"));
			delete theJob;
		}
		return;
		break;
	}

	if(iter == routingTable->end())
	{
		LOG_INFO("unknown command in job "<<theJob);
		theJob->getHost()->sendPackageAndDelete(constructAcknowledgementPackage(theJob, "unknown"));
		delete theJob;
		return;
	}

	switch(iter->second)
	{
	case ServerComponentDecisionMaker:
		server->metaDecisionMakerInstance()->doJob(theJob);
		break;
	case ServerComponentDisplayManager:
		server->displayManagerInstance()->doJob(theJob);
		break;
	case ServerComponentConfigurationManager:
		server->configurationManagerInstance()->doJob(theJob);
		break;
	case ServerComponentPersistenceManager:
		break;
	case ServerComponentHostsManager:
		server->hostsManagerInstance()->doJob(theJob);
		break;
	case ServerComponentServerController:
		server->doJob(theJob);
		break;
	case ServerComponentRendererHost:
		targetID = theJob->getValue("renderer-id");
		rendererHost = server->hostsManagerInstance()->getRendererHost(targetID);
		if(rendererHost) { rendererHost->doJob(theJob); }
		break;
	case ServerComponentViewerHost:
		targetID = theJob->getValue("viewer-id");
		viewerHost = server->hostsManagerInstance()->getViewerHost(targetID);
		if(viewerHost) { viewerHost->doJob(theJob); }
		break;
	case ServerComponentControllerHost:
		targetID = theJob->getValue("controller-id");
		if(targetID.empty()) { controllerHost = server->hostsManagerInstance()->getMainController(); }
		else { controllerHost = server->hostsManagerInstance()->getControllerHost(targetID); }
		if(controllerHost) { controllerHost->doJob(theJob); }
		break;
	default:
		LOG_WARNING("encountered package with invalid routing target: "<<iter->second);
		theJob->getHost()->sendPackageAndDelete(constructAcknowledgementPackage(theJob, "unknown"));
		delete theJob;
	}
}
void PackageRouter::deliverStatusChange(AbstractHost *source, Package *thePackage)
{
	StatusChangePackage *statusChange = new StatusChangePackage(thePackage, source);

	for(std::set<AbstractHost*>::const_iterator iter = statiReceiver->begin(); iter != statiReceiver->end(); ++iter)
	{
		if((*iter)->getType() == ServerComponentControllerHost)
		{
			(*iter)->sendPackage(statusChange);
		}
		else if((*iter)->getType() == ServerComponentViewerHost && statusChange->getSourceType() == ServerComponentRendererHost)
		{
			if(server->displayManagerInstance()->areConnected(dynamic_cast<ViewerHost*>(*iter), dynamic_cast<RendererHost*>(source)))
			{
				(*iter)->sendPackage(statusChange);
			}
		}
	}
      
	delete thePackage;
	delete statusChange;
}

void PackageRouter::addArrayToRoutingTable(ServerComponent component, const char* array[])
{
	int index = 0;
	
	while(array[index] != NULL)
	{
		routingTable->insert(std::make_pair(std::string(array[index]), component));

		index++;
	}		
}
std::set<std::string>* PackageRouter::constructSetFromArray(const char* array[])
{
	std::set<std::string> *result = new std::set<std::string>();
	int index = 0;

	while(array[index] != NULL)
	{
		result->insert(std::string(array[index]));

		index++;
	}

	return result;
}
ServerComponent PackageRouter::stringToServerComponent(std::string string)
{
	if(string == "decision-maker"){ return ServerComponentDecisionMaker; }
	else if(string == "display-manager"){ return ServerComponentDisplayManager; }
	else if(string == "job-manager"){ return ServerComponentJobManager; }
	else if(string == "configuration-manager"){ return ServerComponentConfigurationManager; }
	else if(string == "persistence-manager"){ return ServerComponentPersistenceManager; }
	else if(string == "hosts-manager"){ return ServerComponentHostsManager; }
	else if(string == "package-router"){ return ServerComponentPackageRouter; }
	else if(string == "server-controller"){ return ServerComponentServerController; }
	else if(string == "renderer-host"){ return ServerComponentRendererHost; }
	else if(string == "viewer-host"){ return ServerComponentViewerHost; }
	else if(string == "controller-host"){ return ServerComponentControllerHost; }
	else { return ServerComponent(-1); }
}
