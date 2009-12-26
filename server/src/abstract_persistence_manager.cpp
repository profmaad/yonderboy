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
# include "log.h"

# include "abstract_persistence_manager.h"
# include "persistent_storage.h"

AbstractPersistenceManager::AbstractPersistenceManager()
{
	listStorages = new std::map<std::string, std::map<std::string, PersistentListStorage*> >();
	keyValueStorages = new std::map<std::string, std::map<std::string, PersistentKeyValueStorage*> >();
	tableStorages = new std::map<std::string, std::map<std::string, PersistentTableStorage*> >();
	
	changedStorages = new std::deque<std::pair<void*, StorageType> >();
	releasedStorages = new std::set<void*>();
	currentlySyncingStorage = NULL;
	
	pthread_mutex_init(&syncingStorageMutex, NULL);
	pthread_mutex_init(&changedStoragesMutex, NULL);
	pthread_cond_init(&changedStoragesAvailable, NULL);
	
	syncingShouldEnd = false;
	pthread_create(&syncingThread,NULL,syncRecordsStartMethod,this);
}
AbstractPersistenceManager::~AbstractPersistenceManager()
{	
	//clear up
	delete listStorages;
	delete keyValueStorages;
	delete tableStorages;
	
	delete changedStorages;
	delete releasedStorages;
	
	pthread_mutex_destroy(&syncingStorageMutex);
	pthread_mutex_destroy(&changedStoragesMutex);
	pthread_cond_destroy(&changedStoragesAvailable);
}
void AbstractPersistenceManager::close()
{
	if(server && !server->allowedToBlock()) { return; }
	
	//release all storages
	for(std::map<std::string, std::map<std::string, PersistentListStorage*> >::const_iterator groupIter = listStorages->begin(); groupIter != listStorages->end(); ++groupIter)
	{
		std::string group = groupIter->first;
		for(std::map<std::string, PersistentListStorage*>::const_iterator iter = groupIter->second.begin(); iter != groupIter->second.end(); ++iter)
		{
			releaseStorageButDontRemove(group, iter->first);
		}
	}
	for(std::map<std::string, std::map<std::string, PersistentKeyValueStorage*> >::const_iterator groupIter = keyValueStorages->begin(); groupIter != keyValueStorages->end(); ++groupIter)
	{
		std::string group = groupIter->first;
		for(std::map<std::string, PersistentKeyValueStorage*>::const_iterator iter = groupIter->second.begin(); iter != groupIter->second.end(); ++iter)
		{
			releaseStorageButDontRemove(group, iter->first);
		}
	}
	for(std::map<std::string, std::map<std::string, PersistentTableStorage*> >::const_iterator groupIter = tableStorages->begin(); groupIter != tableStorages->end(); ++groupIter)
	{
		std::string group = groupIter->first;
		for(std::map<std::string, PersistentTableStorage*>::const_iterator iter = groupIter->second.begin(); iter != groupIter->second.end(); ++iter)
		{
			releaseStorageButDontRemove(group, iter->first);
		}
	}
	
	//wait until all syncs are completed
	pthread_mutex_lock(&changedStoragesMutex);
	syncingShouldEnd = true;
	pthread_cond_signal(&changedStoragesAvailable);
	pthread_mutex_unlock(&changedStoragesMutex);
	
	pthread_join(syncingThread, NULL);
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
void AbstractPersistenceManager::releaseStorageButDontRemove(std::string group, std::string id)
{
	bool wakeupSyncThread = false;
	PersistentListStorage *listStorage = NULL;
	PersistentKeyValueStorage *keyValueStorage = NULL;
	PersistentTableStorage *tableStorage = NULL;
	
	switch(getStorageType(group, id))
	{
		case ListStorage:
			listStorage = getListStorage(group, id);
			if(!listStorage) { return; }
			
			listStorage->lockForWriting();
			pthread_mutex_lock(&changedStoragesMutex);
			if(!storageHasChanges(listStorage))
			{
				destroyStorage(group, id);
				delete listStorage;
			}
			else
			{
				releasedStorages->insert(static_cast<void*>(listStorage));
				wakeupSyncThread = true;
			}
			pthread_mutex_unlock(&changedStoragesMutex);
			break;
		case KeyValueStorage:
			keyValueStorage = getKeyValueStorage(group, id);
			if(!keyValueStorage) { return; }
			
			keyValueStorage->lockForWriting();
			pthread_mutex_lock(&changedStoragesMutex);
			if(!storageHasChanges(keyValueStorage))
			{
				destroyStorage(group, id);
				delete keyValueStorage;
			}
			else
			{
				releasedStorages->insert(static_cast<void*>(keyValueStorage));
				wakeupSyncThread = true;
			}
			pthread_mutex_unlock(&changedStoragesMutex);
			break;
		case TableStorage:
			tableStorage = getTableStorage(group, id);
			if(!tableStorage) { return; }
			
			tableStorage->lockForWriting();
			pthread_mutex_lock(&changedStoragesMutex);
			if(!storageHasChanges(tableStorage))
			{
				destroyStorage(group, id);
				delete tableStorage;
			}
			else
			{
				releasedStorages->insert(static_cast<void*>(tableStorage));
				wakeupSyncThread = true;
			}
			pthread_mutex_unlock(&changedStoragesMutex);
			break;
	}
	
	if(wakeupSyncThread)
	{
		pthread_mutex_lock(&changedStoragesMutex);
		pthread_cond_signal(&changedStoragesAvailable);
		pthread_mutex_unlock(&changedStoragesMutex);
	}
}
void AbstractPersistenceManager::releaseStorage(std::string group, std::string id)
{
	std::map<std::string, std::map<std::string, PersistentListStorage*> >::iterator listIter;
	std::map<std::string, std::map<std::string, PersistentKeyValueStorage*> >::iterator keyValueIter;
	std::map<std::string, std::map<std::string, PersistentTableStorage*> >::iterator tableIter;
	
	releaseStorageButDontRemove(group, id);
	
	switch(getStorageType(group, id))
	{
		case ListStorage:
			listIter = listStorages->find(group);
			if(listIter != listStorages->end()) { listIter->second.erase(id); }
			break;
		case KeyValueStorage:
			keyValueIter = keyValueStorages->find(group);
			if(keyValueIter != keyValueStorages->end()) { keyValueIter->second.erase(id); }
			break;
		case TableStorage:
			tableIter = tableStorages->find(group);
			if(tableIter != tableStorages->end()) { tableIter->second.erase(id); }
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
	if(!storageHasChanges(storage))
	{
		changedStorages->push_back(std::make_pair(storage, type));
		addedToDeque = true;
	}
	pthread_mutex_unlock(&changedStoragesMutex);
	
	if(addedToDeque)
	{
		pthread_mutex_lock(&changedStoragesMutex);
		pthread_cond_signal(&changedStoragesAvailable);
		pthread_mutex_unlock(&changedStoragesMutex);
	}
}
bool AbstractPersistenceManager::storageHasChanges(void* storage)
{
	if(currentlySyncingStorage == storage) { return true; }
	
	for(std::deque<std::pair<void*, StorageType> >::const_iterator iter = changedStorages->begin(); iter != changedStorages->end(); ++iter)
	{
		if(iter->first == storage) { return true; }
	}
	
	return false;
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
