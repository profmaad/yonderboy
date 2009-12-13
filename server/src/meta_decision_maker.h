//      meta_decision_maker.h
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

# ifndef META_DECISION_MAKER_H
# define META_DECISION_MAKER_H

# include <map>
# include <stack>
# include <string>

# include "macros.h"

class AbstractDecisionMaker;

class MetaDecisionMaker
{
public:
	Decision decide(Entity entity, KeyValueMap *information);

	void addDecisionMaker(Entity entity, AbstractDecisionMaker *decisionMaker);
	void setDecisionStack(Entity entity, std::stack<AbstractDecisionMaker*> newStack);
	void resetDecisionStack(Entity entity);

// Singleton management	
	static MetaDecisionMaker* instance();
	static void deleteInstance();

private:
	std::map<Entity, std::stack<AbstractDecisionMaker*> > *decisionMakers;

// Singleton management
	MetaDecisionMaker();
	virtual ~MetaDecisionMaker();
	
	static MetaDecisionMaker* _instance;
};

# endif /*META_DECISION_MAKER_H*/
