//      abstract_persistence_manager_sync.cpp
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

# include <cerrno>

# include "macros.h"

# include "abstract_persistence_manager.h"
# include "persistent_storage.h"

void* AbstractPersistenceManager::syncRecordsStartMethod(void* persistenceManagerPointer)
{
	AbstractPersistenceManager *persistenceManager = static_cast<AbstractPersistenceManager*>(persistenceManagerPointer);

	return persistenceManager->syncRecords();
}
void* AbstractPersistenceManager::syncRecords()
{
	std::pair<void*, StorageType> storagePair;
	
	PersistentListStorage *listStorage = NULL;
	PersistentKeyValueStorage *keyValueStorage = NULL;
	PersistentTableStorage *tableStorage = NULL;

	std::string storageGroup;
	std::string storageID;
	
	std::map<KeyType, ListRecord> *changedListRecords = NULL;
	std::map<KeyType, KeyValueRecord> *changedKeyValueRecords = NULL;
	std::map<KeyType, TableRecord> *changedTableRecords = NULL;
	
	std::map<KeyType, ListRecord>::const_iterator listIter;
	std::map<KeyType, KeyValueRecord>::const_iterator keyValueIter;
	std::map<KeyType, TableRecord>::const_iterator tableIter;
	
	int changedStoragesAvailableValue = -1;
	
	while(true)
	{
		pthread_mutex_lock(&changedStoragesMutex);
		if(changedStorages->size() > 0)
		{
			storagePair = changedStorages->front();
			changedStorages->pop_front();
		}
		pthread_mutex_unlock(&changedStoragesMutex);
		
		if(storagePair.first)
		{
			switch(storagePair.second)
			{
				case ListStorage:
					listStorage = static_cast<PersistentListStorage*>(storagePair.first);
					
					storageGroup = listStorage->getGroup();
					storageID = listStorage->getID();
					break;
				case KeyValueStorage:
					keyValueStorage = static_cast<PersistentKeyValueStorage*>(storagePair.first);
					
					storageGroup = keyValueStorage->getGroup();
					storageID = keyValueStorage->getID();
					break;
				case TableStorage:
					tableStorage = static_cast<PersistentTableStorage*>(storagePair.first);
					
					storageGroup = tableStorage->getGroup();
					storageID = tableStorage->getID();
					break;
			}
				
			pthread_mutex_lock(&syncingStorageMutex);
			syncingGroup = storageGroup;
			syncingID = storageID;
			pthread_mutex_unlock(&syncingStorageMutex);
			
			switch(storagePair.second)
			{
				case ListStorage:
					changedListRecords = listStorage->getRecordsToSync();
					break;
				case KeyValueStorage:
					changedKeyValueRecords = keyValueStorage->getRecordsToSync();
					break;
				case TableStorage:
					changedTableRecords = tableStorage->getRecordsToSync();
					break;
			}
				
			pthread_mutex_lock(&syncingStorageMutex);
			syncingGroup = "";
			syncingID = "";
			pthread_mutex_unlock(&syncingStorageMutex);
			
			switch(storagePair.second)
			{
				case ListStorage:
					if(changedListRecords)
					{
						
						for(listIter = changedListRecords->begin(); listIter != changedListRecords->end(); ++listIter)
						{
							writeListRecord(storageGroup, storageID, listIter->first, listIter->second);
						}
					}
					break;
				case KeyValueStorage:
					if(changedKeyValueRecords)
					{
						
						for(keyValueIter = changedKeyValueRecords->begin(); keyValueIter != changedKeyValueRecords->end(); ++keyValueIter)
						{
							writeKeyValueRecord(storageGroup, storageID, keyValueIter->first, keyValueIter->second);
						}
					}
					break;
				case TableStorage:
					if(changedTableRecords)
					{
						
						for(tableIter = changedTableRecords->begin(); tableIter != changedTableRecords->end(); ++tableIter)
						{
							writeTableRecord(storageGroup, storageID, tableIter->first, tableIter->second);
						}
					}
					break;
			}
		}
		
		//sleep until there is more work to do
		LOG_DEBUG("sync thread waiting on semaphore")
		int success = sem_wait(&changedStoragesAvailable);
		LOG_DEBUG("sync thread passed semaphore: "<<errno)
		
		//check whether we are supposed to quit
		pthread_mutex_lock(&syncingShouldEndMutex);
		if(syncingShouldEnd)
		{
			sem_getvalue(&changedStoragesAvailable, &changedStoragesAvailableValue);
			if(changedStoragesAvailableValue == 0)
			{
				pthread_mutex_unlock(&syncingShouldEndMutex);
				break;
			}
		}
		pthread_mutex_unlock(&syncingShouldEndMutex);
	}
	LOG_DEBUG("sync thread quit")
}
