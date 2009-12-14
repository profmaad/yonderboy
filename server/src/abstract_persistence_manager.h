//      abstract_persistence_manager.h
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

# ifndef ABSTRACT_PERSISTENCE_MANAGER_H
# define ABSTRACT_PERSISTENCE_MANAGER_H

# include <map>
# include <vector>
# include <string>
# include <utility>

# include "macros.h"

template<typename R>
class PersistentStorage;

class AbstractPersistenceManager
{
public:
	AbstractPersistenceManager();
	virtual ~AbstractPersistenceManager();

	// PersistenceManager API
	PersistentStorage<std::string>* retrieveListStorage(std::string group, std::string id);
	PersistentStorage< std::pair<std::string, std:.string> >* retrieveKeyValueStorage(std::string group, std::string id);
	PersistetnStorage< std::vector<std::string> >* retrieveTableStorage(std::string group, std:.string id);
	void releaseStorage(std::string group, std::string id);

	// PersistentStorage API
	bool swapRequested(std::string group, std::string id);
	virtual void* retrieveRecord(std::string group, std::string id, KeyType key) = 0;
	virtual KeyType nextKey(std::string group, std::string id) = 0;

protected:

private:

};

# endif /*PERSISTENCE_MANAGER_H*/
