//      command_parser.cpp
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
# include <map>
# include <vector>
# include <iostream>
# include <utility>

# include <cstdlib>
# include <cstring>

# include <yaml.h>
# include <popt.h>

# include "package.h"
# include "package_factories.h"

# include "command_parser.h"

CommandParser::CommandParser(const YAML::Node &node) : valid(false), parameters(NULL), boolOptionArguments(NULL), stringOptionArguments(NULL), rendererIDRequired(false), viewerIDRequired(false), viewIDRequired(false), rendererID(NULL), viewerID(NULL), viewID(NULL), optionsCount(0), generatorPosition(0)
{
	parameters = new std::vector<requiredParameter>();
	boolOptionArguments = new std::map<std::string, int*>();
	stringOptionArguments = new std::map<std::string, char**>();

	reset();
	
	if(node["command"].GetType() != YAML::CT_SCALAR)
	{
		valid = false;
		return;
	}

	std::vector<poptOption> tempOptions;

	try
	{
		node["command"] >> command;
		node["description"] >> description;

		if(const YAML::Node *requiredNode = node.FindValue("required"))
		{
			if(requiredNode->GetType() == YAML::CT_SEQUENCE)
			{
				for(int i=0;i<requiredNode->size(); i++)
				{
					if((*requiredNode)[i].GetType() == YAML::CT_MAP)
					{
						std::string name, type, description;
						(*requiredNode)[i]["name"] >> name;

						if(const YAML::Node *typeNode = (*requiredNode)[i].FindValue("type"))
						{
							*typeNode >> type;
						}
						else
						{
							type = "string";
						}

						if(const YAML::Node *descNode = (*requiredNode)[i].FindValue("description"))
						{
							*descNode >> description;
						}

						if(type == "renderer")
						{
							if(rendererIDRequired) { continue; }
							
							poptOption option = { "renderer", 'r', POPT_ARG_STRING, &rendererID, 0, "renderer to use", "renderer id"};
							tempOptions.push_back(option);
							rendererIDRequired = true;

						}
						else if(type == "viewer")
						{
							if(viewerIDRequired) { continue; }
							
							poptOption option = { "viewer", 'v', POPT_ARG_STRING, &viewerID, 0, "viewer to use", "viewer id"};
							tempOptions.push_back(option);
							viewerIDRequired = true;
						}
						else if(type == "view")
						{
							if(viewIDRequired) { continue; }
							
							poptOption option = { "view", 'V', POPT_ARG_STRING, &viewID, 0, "view to use", "view id"};
							tempOptions.push_back(option);
							viewIDRequired = true;
						}
						else if(type == "boolean")
						{
							(*boolOptionArguments)[name] = (int*)malloc(sizeof(int));
							*(*boolOptionArguments)[name] = 0;
							poptOption option = { name.c_str(), '\0', POPT_ARG_NONE, (*boolOptionArguments)[name], 0, (description.empty()?NULL:description.c_str()), NULL };
							tempOptions.push_back(option);
						}
						else
						{
							requiredParameter parameter;
							parameter.name = name;
							parameter.description = description;
							parameters->push_back(parameter);
						}
					}
				}
			}
		}
		if(const YAML::Node *optionalNode = node.FindValue("optional"))
		{
			if(optionalNode->GetType() == YAML::CT_SEQUENCE)
			{
				for(int i=0;i<optionalNode->size(); i++)
				{
					if((*optionalNode)[i].GetType() == YAML::CT_MAP)
					{
						std::string name, type, description;
						(*optionalNode)[i]["name"] >> name;
						
						if(const YAML::Node *typeNode = (*optionalNode)[i].FindValue("type"))
						{
							*typeNode >> type;
						}
						else
						{
							type = "string";
						}
						
						if(const YAML::Node *descNode = (*optionalNode)[i].FindValue("description"))
						{
							*descNode >> description;
						}

						if(type == "renderer")
						{
							if(rendererIDRequired) { continue; }
							
							poptOption option = { "renderer", 'r', POPT_ARG_STRING, &rendererID, 0, "renderer to use", "renderer id"};
							tempOptions.push_back(option);
							rendererIDRequired = true;
						}
						else if(type == "viewer")
						{
							if(viewerIDRequired) { continue; }
							
							poptOption option = { "viewer", 'v', POPT_ARG_STRING, &viewerID, 0, "viewer to use", "viewer id"};
							tempOptions.push_back(option);
							viewerIDRequired = true;
						}
						else if(type == "view")
						{
							if(viewIDRequired) { continue; }
							
							poptOption option = { "view", 'V', POPT_ARG_STRING, &viewID, 0, "view to use", "view id"};
							tempOptions.push_back(option);
							viewIDRequired = true;
						}
						else if(type == "boolean")
						{
							(*boolOptionArguments)[name] = (int*)malloc(sizeof(int));
							*(*boolOptionArguments)[name] = 0;
							poptOption option = { name.c_str(), '\0', POPT_ARG_NONE, (*boolOptionArguments)[name], 0, (description.empty()?NULL:description.c_str()), NULL };
							tempOptions.push_back(option);
						}
						else
						{
							(*stringOptionArguments)[name] = (char**)malloc(sizeof(char*));
							*(*stringOptionArguments)[name] = NULL;
							poptOption option = { name.c_str(), '\0', POPT_ARG_STRING, (*stringOptionArguments)[name], 0, (description.empty()?NULL:description.c_str()), NULL };
							tempOptions.push_back(option);
						}
					}
				}				
			}
		}
	}
	catch(const YAML::Exception &e)
	{
		std::cerr<<"error parsing command '"<<command<<"': "<<e.what()<<std::endl;
	}

	options = (poptOption*)malloc(sizeof(poptOption)*(tempOptions.size()+1));
	for(int i=0;i < tempOptions.size(); i++)
	{
		options[i] = tempOptions[i];
	}
	options[tempOptions.size()] = POPT_TABLEEND;
	optionsCount = tempOptions.size();
}
CommandParser::~CommandParser()
{
	for(std::map<std::string, int*>::iterator iter = boolOptionArguments->begin(); iter != boolOptionArguments->end(); iter++)
	{
		free(iter->second);
	}
	delete boolOptionArguments;
	for(std::map<std::string, char**>::iterator iter = stringOptionArguments->begin(); iter != stringOptionArguments->end(); iter++)
	{
		free(*(iter->second));
		free(iter->second);
	}
	delete stringOptionArguments;

	free(rendererID);
	free(viewerID);
	free(viewID);

	delete parameters;

	free(options);
}
void CommandParser::reset()
{
	for(std::map<std::string, int*>::iterator iter = boolOptionArguments->begin(); iter != boolOptionArguments->end(); iter++)
	{
		*(iter->second) = 0;
	}
	for(std::map<std::string, char**>::iterator iter = stringOptionArguments->begin(); iter != stringOptionArguments->end(); iter++)
	{
		free(*(iter->second));
		*(iter->second) = NULL;
	}

	free(rendererID);
	free(viewerID);
	free(viewID);
	rendererID = NULL;
	viewerID = NULL;
	viewID = NULL;
}

Package* CommandParser::constructPackageFromLine(int argc, const char **argv, std::string packageID)
{
	poptContext context;
	int result = -1;
	Package *package = NULL;

	// create context for option parsing
	context = poptGetContext("yonderboy-cli-controller", argc, argv, options, 0);

	// parse options into arg pointers
	result = poptGetNextOpt(context); // since all options are directly handled through arg pointers, we only need to call this once
	if(result < -1)
	{
		std::cerr<<poptBadOption(context, POPT_BADOPTION_NOALIAS)<<": "<<poptStrerror(result)<<std::endl;
		return package;
	}

	// construct package from info in arg pointers
	std::map<std::string, std::string> *kvMap = new std::map<std::string, std::string>();

	kvMap->insert(std::make_pair("type", "command"));
	kvMap->insert(std::make_pair("id", packageID));
	kvMap->insert(std::make_pair("command", command));

	for(int i=0;i < parameters->size(); i++)
	{
		kvMap->insert(std::make_pair(parameters->at(i).name, poptPeekArg(context)));
		free((void*)poptGetArg(context));
	}

	for(std::map<std::string, int*>::const_iterator iter = boolOptionArguments->begin(); iter != boolOptionArguments->end(); iter++)
	{
		if(*(iter->second) != 0)
		{
			kvMap->insert(std::make_pair(iter->first, ""));
		}
	}
	for(std::map<std::string, char**>::const_iterator iter = stringOptionArguments->begin(); iter != stringOptionArguments->end(); iter++)
	{
		if(*(iter->second))
		{
			kvMap->insert(std::make_pair(iter->first, *(iter->second)));
		}
	}

	if(rendererIDRequired && rendererID)
	{
		kvMap->insert(std::make_pair("renderer-id", rendererID));
	}
	if(viewerIDRequired && viewerID)
	{
		kvMap->insert(std::make_pair("viewer-id", viewerID));
	}
	if(viewIDRequired && viewID)
	{
		kvMap->insert(std::make_pair("view-id", viewID));
	}

	package = new Package(kvMap);

	// clean up and free memory
	poptFreeContext(context);
	reset();

	return package;
}
char* CommandParser::completionGenerator(const char *text, int state)
{
	const char *comparisonText = NULL;

	if(state == 0)
	{
		generatorPosition = 0;
	}

	if((strlen(text) > 0 && text[0] != '-') || (strlen(text) > 1 && text[1] != '-'))
	{
		return NULL;
	}
	else if(strlen(text) == 0) { comparisonText = text; }
	else if(strlen(text) == 1) { comparisonText = &text[1]; }
	else { comparisonText = &text[2]; }

	while(generatorPosition < optionsCount)
	{
		poptOption option = options[generatorPosition];
		std::string longOptionName = option.longName;
		generatorPosition++;

		if(longOptionName.compare(0, strlen(comparisonText), comparisonText) == 0)
		{
			char *completion = (char*)malloc(sizeof(char)*(longOptionName.length()+3)); //"--" + longOptionName + '\0'
			completion[0] = '-';
			completion[1] = '-';
			strcpy(&completion[2], longOptionName.c_str());
			completion[longOptionName.length()+2] = '\0';
			return completion;
			
		}
	}

	return NULL;
}
