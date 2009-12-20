//      abstract_persistence_manager.cpp
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
# include <vector>
# include <utility>

# include <pthread.h>

# include "macros.h"

# include "abstract_persistence_manager.h"
# include "persistent_storage.h"

AbstractPersistenceManager::AbstractPersistenceManager()
{
	listStorages = new std::map<std::string, std::map<std::string, PersistentListStorage*> >();
	keyValueStorages = new std::map<std::string, std::map<std::string, PersistentKeyValueStorage*> >();
	tableStorages = new std::map<std::string, std::map<std::string, PersistentTableStorage*> >();
	
	changedListStorages = new std::deque<PersistentListStorage*>();
	changedKeyValueStorages = new std::deque<PersistentKeyValueStorage*>();
	changedTableStorages = new std::deque<PersistentTableStorage*>();
	
	pthread_mutex_init(&syncingStorageMutex, NULL);
	pthread_mutex_init(&changedStoragesMutex, NULL);
	pthread_mutex_init(&changedStoragesAvailableMutex, NULL);
	pthread_mutex_init(&syncingShouldEndMutex, NULL);
	pthread_cond_init(&changedStoragesAvailable, NULL);
	
	syncingShouldEnd = false;
	pthread_t *syncingThread;
	pthread_create(syncingThread,NULL,syncRecordsStartMethod,this);
}
AbstractPersistenceManager::~AbstractPersistenceManager()
{
	//release all storages
	for(std::map<std::string, std::map<std::string, PersistentListStorage*> >::const_iterator groupIter = listStorages->begin(); groupIter != listStorages->end(); ++groupIter)
	{
		std::string group = groupIter->first;
		for(std::map<std::string, PersistentListStorage*>::const_iterator iter = groupIter->second.begin(); iter != groupIter->second.end(); ++iter)
		{
			releaseStorage(group, iter->first);
		}
	}
	for(std::map<std::string, std::map<std::string, PersistentKeyValueStorage*> >::const_iterator groupIter = keyValueStorages->begin(); groupIter != keyValueStorages->end(); ++groupIter)
	{
		std::string group = groupIter->first;
		for(std::map<std::string, PersistentKeyValueStorage*>::const_iterator iter = groupIter->second.begin(); iter != groupIter->second.end(); ++iter)
		{
			releaseStorage(group, iter->first);
		}
	}
	for(std::map<std::string, std::map<std::string, PersistentTableStorage*> >::const_iterator groupIter = tableStorages->begin(); groupIter != tableStorages->end(); ++groupIter)
	{
		std::string group = groupIter->first;
		for(std::map<std::string, PersistentTableStorage*>::const_iterator iter = groupIter->second.begin(); iter != groupIter->second.end(); ++iter)
		{
			releaseStorage(group, iter->first);
		}
	}
	
	//wait until all syncs are completed
	pthread_mutex_lock(&syncingShouldEndMutex);
	syncingShouldEnd = true;
	pthread_mutex_unlock(&syncingShouldEndMutex);
	
	pthread_mutex_lock(&changedStoragesAvailableMutex);
	pthread_cond_signal(&changedStoragesAvailable);
	pthread_mutex_unlock(&changedStoragesAvailableMutex);
	
	pthread_join(*syncingThread, NULL);
	
	//clear up
	delete listStorages;
	delete keyValueStorages;
	delete tableStorages;
	
	delete changedListStorages;
	delete changedKeyValueStorages;
	delete changedTableStorages;
	
	pthread_mutex_destroy(&syncingStorageMutex);
	pthread_mutex_destroy(&changedStoragesMutex);
	pthread_mutex_destroy(&changedStoragesAvailableMutex);
	pthread_mutex_destroy(&syncingShouldEndMutex);
	pthread_cond_destroy(&changedStoragesAvailable);
}

PersistentListStorage* AbstractPersistenceManager::retrieveListStorage(std::string group, std::string id)
{
	std::map<std::string, std::map<std::string, PersistentListStorage*> >::iterator groupIter = listStorages->find(group);
	if(groupIter == listStorages->end())
	{
		groupIter = listStorages->insert(std::make_pair(group,std::map<std::string, PersistentListStorage*>())).first;
	}
	
	std::map<std::string, PersistentListStorage*>::iterator iter = groupIter->second.find(id);
	if(iter != groupIter->second.end())
	{
		return iter->second;
	}
	else
	{
		PersistentListStorage* storage = createListStorage(group, id); // this also creates the reader thread for us
		
		groupIter->second.insert(std::make_pair(id,storage));
		
		return storage;
	}
}
PersistentKeyValueStorage* AbstractPersistenceManager::retrieveKeyValueStorage(std::string group, std::string id)
{
	std::map<std::string, std::map<std::string, PersistentKeyValueStorage*> >::iterator groupIter = keyValueStorages->find(group);
	if(groupIter == keyValueStorages->end())
	{
		groupIter = keyValueStorages->insert(std::make_pair(group,std::map<std::string, PersistentKeyValueStorage*>())).first;
	}
	
	std::map<std::string, PersistentKeyValueStorage*>::iterator iter = groupIter->second.find(id);
	if(iter != groupIter->second.end())
	{
		return iter->second;
	}
	else
	{
		PersistentKeyValueStorage* storage = createKeyValueStorage(group, id); // this also creates the reader thread for us
		
		groupIter->second.insert(std::make_pair(id,storage));
		
		return storage;
	}
}
PersistentTableStorage* AbstractPersistenceManager::retrieveTableStorage(std::string group, std::string id)
{
	std::map<std::string, std::map<std::string, PersistentTableStorage*> >::iterator groupIter = tableStorages->find(group);
	if(groupIter == tableStorages->end())
	{
		groupIter = tableStorages->insert(std::make_pair(group,std::map<std::string, PersistentTableStorage*>())).first;
	}
	
	std::map<std::string, PersistentTableStorage*>::iterator iter = groupIter->second.find(id);
	if(iter != groupIter->second.end())
	{
		return iter->second;
	}
	else
	{
		PersistentTableStorage* storage = createTableStorage(group, id); // this also creates the reader thread for us
		
		groupIter->second.insert(std::make_pair(id,storage));
		
		return storage;
	}
}
void AbstractPersistenceManager::releaseStorage(std::string group, std::string id)
{
	PersistentListStorage *listStorage = NULL;
	PersistentKeyValueStorage *keyValueStorage = NULL;
	PersistentTableStorage *tableStorage = NULL;
	
	std::map<std::string, std::map<std::string, PersistentListStorage*> >::iterator listIter;
	std::map<std::string, std::map<std::string, PersistentKeyValueStorage*> >::iterator keyValueIter;
	std::map<std::string, std::map<std::string, PersistentTableStorage*> >::iterator tableIter;
	
	switch(getStorageType(group, id))
	{
		case ListStorage:
			listStorage = getListStorage(group, id);
			if(!listStorage) { return; }
			
			listIter = listStorages->find(group);
			if(listIter != listStorages->end()) { listIter->second.erase(id); }
			
			listStorage->lockForWriting();
			pthread_mutex_lock(&changedStoragesMutex);
			if(!dequeContainsElement(changedListStorages, listStorage))
			{
				destroyStorage(group, id);
				delete listStorage;
			}
			else
			{
				releasedStorages->insert(static_cast<void*>(listStorage));
			}
			pthread_mutex_unlock(&changedStoragesMutex);
			break;
		case KeyValueStorage:
			keyValueStorage = getKeyValueStorage(group, id);
			if(!keyValueStorage) { return; }
			
			keyValueIter = keyValueStorages->find(group);
			if(keyValueIter != keyValueStorages->end()) { keyValueIter->second.erase(id); }
			
			keyValueStorage->lockForWriting();
			pthread_mutex_lock(&changedStoragesMutex);
			if(!dequeContainsElement(changedKeyValueStorages, keyValueStorage))
			{
				destroyStorage(group, id);
				delete keyValueStorage;
			}
			else
			{
				releasedStorages->insert(static_cast<void*>(keyValueStorage));
			}
			pthread_mutex_unlock(&changedStoragesMutex);
			break;
		case TableStorage:
			tableStorage = getTableStorage(group, id);
			if(!tableStorage) { return; }
			
			tableIter = tableStorages->find(group);
			if(tableIter != tableStorages->end()) { tableIter->second.erase(id); }
			
			tableStorage->lockForWriting();
			pthread_mutex_lock(&changedStoragesMutex);
			if(!dequeContainsElement(changedTableStorages, tableStorage))
			{
				destroyStorage(group, id);
				delete tableStorage;
			}
			else
			{
				releasedStorages->insert(static_cast<void*>(tableStorage));
			}
			pthread_mutex_unlock(&changedStoragesMutex);
			break;
	}
}

bool AbstractPersistenceManager::swapRequested(std::string group, std::string id)
{
	bool result = false;
	
	pthread_mutex_lock(&syncingStorageMutex);
	if(syncingGroup == group && syncingID == id)
	{
		result = true;
	}
	pthread_mutex_unlock(&syncingStorageMutex);
	
	return result;
}
void AbstractPersistenceManager::recordsChanged(void *storage, StorageType type)
{
	bool addedToDeque = false;
	
	pthread_mutex_lock(&changedStoragesMutex);
	
	switch(type)
	{
		case ListStorage:
			if(!dequeContainsElement(changedListStorages, static_cast<PersistentListStorage*>(storage)))
			{
				changedListStorages->push_back(static_cast<PersistentListStorage*>(storage));
				addedToDeque = true;
			}
			break;
		case KeyValueStorage:
			if(!dequeContainsElement(changedKeyValueStorages, static_cast<PersistentKeyValueStorage*>(storage)))
			{
				changedKeyValueStorages->push_back(static_cast<PersistentKeyValueStorage*>(storage));
				addedToDeque = true;
			}
			break;
		case TableStorage:
			if(!dequeContainsElement(changedTableStorages, static_cast<PersistentTableStorage*>(storage)))
			{
				changedTableStorages->push_back(static_cast<PersistentTableStorage*>(storage));
				addedToDeque = true;
			}
			break;
	}
	
	pthread_mutex_unlock(&changedStoragesMutex);
	
	if(addedToDeque)
	{
		pthread_mutex_lock(&changedStoragesAvailableMutex);
		pthread_cond_signal(&changedStoragesAvailable);
		pthread_mutex_unlock(&changedStoragesAvailableMutex);
	}
}

StorageType AbstractPersistenceManager::getStorageType(std::string group, std::string id)
{
	std::map<std::string, std::map<std::string, PersistentListStorage*> >::const_iterator listIter = listStorages->find(group);
	if(listIter != listStorages->end())
	{
		if(listIter->second.find(id) != listIter->second.end()) { return ListStorage; }
	}
	
	std::map<std::string, std::map<std::string, PersistentKeyValueStorage*> >::const_iterator keyValueIter = keyValueStorages->find(group);
	if(keyValueIter != keyValueStorages->end())
	{
		if(keyValueIter->second.find(id) != keyValueIter->second.end()) { return KeyValueStorage; }
	}
	
	std::map<std::string, std::map<std::string, PersistentTableStorage*> >::const_iterator tableIter = tableStorages->find(group);
	if(tableIter != tableStorages->end())
	{
		if(tableIter->second.find(id) != tableIter->second.end()) { return TableStorage; }
	}
	
	return InvalidStorage;
}
PersistentListStorage* AbstractPersistenceManager::getListStorage(std::string group, std::string id)
{
	std::map<std::string, std::map<std::string, PersistentListStorage*> >::const_iterator groupIter = listStorages->find(group);
	if(groupIter != listStorages->end())
	{
		std::map<std::string, PersistentListStorage*>::const_iterator iter = groupIter->second.find(id);
		if(iter != groupIter->second.end()) { return iter->second; }
	}
	
	return NULL;
}
PersistentKeyValueStorage* AbstractPersistenceManager::getKeyValueStorage(std::string group, std::string id)
{
	std::map<std::string, std::map<std::string, PersistentKeyValueStorage*> >::const_iterator groupIter = keyValueStorages->find(group);
	if(groupIter != keyValueStorages->end())
	{
		std::map<std::string, PersistentKeyValueStorage*>::const_iterator iter = groupIter->second.find(id);
		if(iter != groupIter->second.end()) { return iter->second; }
	}
	
	return NULL;
}
PersistentTableStorage* AbstractPersistenceManager::getTableStorage(std::string group, std::string id)
{
	std::map<std::string, std::map<std::string, PersistentTableStorage*> >::const_iterator groupIter = tableStorages->find(group);
	if(groupIter != tableStorages->end())
	{
		std::map<std::string, PersistentTableStorage*>::const_iterator iter = groupIter->second.find(id);
		if(iter != groupIter->second.end()) { return iter->second; }
	}
	
	return NULL;
}
