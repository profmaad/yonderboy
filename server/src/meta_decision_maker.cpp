//      meta_decision_maker.cpp
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
# include <stack>
# include <string>
# include <utility>

# include "macros.h"
# include "abstract_decision_maker.h"

# include "meta_decision_maker.h"

MetaDecisionMaker::MetaDecisionMaker()
{
	decisionMakers = new std::map<Entity, std::stack<AbstractDecisionMaker*> >();
}
MetaDecisionMaker::~MetaDecisionMaker()
{
	for(std::map<Entity, std::stack<AbstractDecisionMaker*> >::iterator iter = decisionMakers->begin(); iter != decisionMakers->end(); ++iter)
	{
		while(! iter->second.empty())
		{
			delete iter->second.top();
			iter->second.pop();
		}
	}

	delete decisionMakers;
}

Decision MetaDecisionMaker::decide(Entity entity, KeyValueMap *information)
{
	if(decisionMakers->find(entity) == decisionMakers->end()) { return CantDecide; }

	Decision finalDecision = CantDecide;
	std::stack<AbstractDecisionMaker*> decisionStack = decisionMakers->find(entity)->second;

	while(! decisionStack.empty())
	{
		finalDecision = decisionStack.top()->decide(information);
		decisionStack.pop();

		if(finalDecision >= 0)
		{
			break;
		}
	}

	return finalDecision;
}

void MetaDecisionMaker::addDecisionMaker(Entity entity, AbstractDecisionMaker *decisionMaker)
{
	std::map<Entity, std::stack<AbstractDecisionMaker*> >::iterator iter = decisionMakers->find(entity);

	if(iter == decisionMakers->end())
	{
		std::stack<AbstractDecisionMaker*> decisionStack;
		decisionStack.push(decisionMaker);
		decisionMakers->insert(std::make_pair(entity, decisionStack));
	}
	else
	{
		iter->second.push(decisionMaker);
	}
}
void MetaDecisionMaker::setDecisionStack(Entity entity, std::stack<AbstractDecisionMaker*> newStack)
{
	std::map<Entity, std::stack<AbstractDecisionMaker*> >::iterator iter = decisionMakers->find(entity);

	if(iter == decisionMakers->end())
	{
		decisionMakers->insert(std::make_pair(entity, newStack));
	}
	else
	{
		iter->second = newStack;
	}
}
void MetaDecisionMaker::resetDecisionStack(Entity entity)
{
	std::map<Entity, std::stack<AbstractDecisionMaker*> >::iterator iter = decisionMakers->find(entity);

	if(iter == decisionMakers->end()) { return; }

	while(! iter->second.empty())
	{
		iter->second.pop();
	}
}

// Singleton management
MetaDecisionMaker* MetaDecisionMaker::_instance = NULL;

MetaDecisionMaker* MetaDecisionMaker::instance()
{
	if(_instance == NULL)
	{
		_instance = new MetaDecisionMaker();
	}

	return _instance;
}
void MetaDecisionMaker::deleteInstance()
{
	delete _instance;
	_instance = NULL;
}

