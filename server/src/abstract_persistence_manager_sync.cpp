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
	while(true)
	{
		//iterate over changed storages queues until they are all empty
		while(true)
		{
			bool allQueuesEmpty = true;
			//alternatingly check lists, key-values and tables
			
			// check for list storage
			{
				PersistentListStorage *storage = NULL;
				std::string storageGroup;
				std::string storageID;
			
				pthread_mutex_lock(&changedStoragesMutex);
				if(changedListStorages->size() > 0)
				{
					allQueuesEmpty = false;
					
					storage = changedListStorages->front();
					changedListStorages->pop_front();
				}
				pthread_mutex_unlock(&changedStoragesMutex);
				
				if(storage)
				{
					storageGroup = storage->getGroup();
					storageID = storage->getID();
					
					pthread_mutex_lock(&syncingStorageMutex);
					syncingGroup = storageGroup;
					syncingID = storageID;
					pthread_mutex_unlock(&syncingStorageMutex);
					
					std::map<KeyType, std::string>* changedRecords = storage->getRecordsToSync();
					
					pthread_mutex_lock(&syncingStorageMutex);
					syncingGroup = "";
					syncingID = "";
					pthread_mutex_unlock(&syncingStorageMutex);
					
					if(changedRecords)
					{
						for(std::map<KeyType, std::string>::const_iterator iter = changedRecords->begin(); iter != changedRecords->end(); ++iter)
						{
							writeListRecord(storageGroup, storageID, iter->first, iter->second);
						}
					}
				}
			}
			
			// check for key-value storage
			{
				PersistentKeyValueStorage *storage = NULL;
				std::string storageGroup;
				std::string storageID;
				
				pthread_mutex_lock(&changedStoragesMutex);
				if(changedKeyValueStorages->size() > 0)
				{
					allQueuesEmpty = false;
					
					storage = changedKeyValueStorages->front();
					changedKeyValueStorages->pop_front();
				}
				pthread_mutex_unlock(&changedStoragesMutex);
				
				if(storage)
				{
					storageGroup = storage->getGroup();
					storageID = storage->getID();
					
					pthread_mutex_lock(&syncingStorageMutex);
					syncingGroup = storageGroup;
					syncingID = storageID;
					pthread_mutex_unlock(&syncingStorageMutex);
					
					std::map<KeyType, std::pair<std::string, std::string> >* changedRecords = storage->getRecordsToSync();
					
					pthread_mutex_lock(&syncingStorageMutex);
					syncingGroup = "";
					syncingID = "";
					pthread_mutex_unlock(&syncingStorageMutex);
					
					if(changedRecords)
					{
						for(std::map<KeyType, std::pair<std::string, std::string> >::const_iterator iter = changedRecords->begin(); iter != changedRecords->end(); ++iter)
						{
							writeKeyValueRecord(storageGroup, storageID, iter->first, iter->second);
						}
					}
				}
			}
			
			// check for table storage
			{
				PersistentTableStorage *storage = NULL;
				std::string storageGroup;
				std::string storageID;
				
				pthread_mutex_lock(&changedStoragesMutex);
				if(changedTableStorages->size() > 0)
				{
					allQueuesEmpty = false;
					
					storage = changedTableStorages->front();
					changedTableStorages->pop_front();
				}
				pthread_mutex_unlock(&changedStoragesMutex);
				
				if(storage)
				{
					storageGroup = storage->getGroup();
					storageID = storage->getID();
					
					pthread_mutex_lock(&syncingStorageMutex);
					syncingGroup = storageGroup;
					syncingID = storageID;
					pthread_mutex_unlock(&syncingStorageMutex);
					
					std::map<KeyType, std::vector<std::string> >* changedRecords = storage->getRecordsToSync();
					
					pthread_mutex_lock(&syncingStorageMutex);
					syncingGroup = "";
					syncingID = "";
					pthread_mutex_unlock(&syncingStorageMutex);
					
					if(changedRecords)
					{
						for(std::map<KeyType, std::vector<std::string> >::const_iterator iter = changedRecords->begin(); iter != changedRecords->end(); ++iter)
						{
							writeTableRecord(storageGroup, storageID, iter->first, iter->second);
						}
					}
				}
			}
			
			if(allQueuesEmpty) { break; }
		}
		
		//sleep until there is more work to do
		pthread_mutex_lock(&changedStoragesAvailableMutex);
		pthread_cond_wait(&changedStoragesAvailable, &changedStoragesAvailableMutex);
		pthread_mutex_unlock(&changedStoragesAvailableMutex);
		
		//check whether we are supposed to quit
		pthread_mutex_lock(&syncingShouldEndMutex);
		if(syncingShouldEnd)
		{
			pthread_mutex_unlock(&syncingShouldEndMutex);
			break;
		}
		pthread_mutex_unlock(&syncingShouldEndMutex);
	}
}
