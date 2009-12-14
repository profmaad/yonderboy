//      persistent_storage.h
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

# include <string>
# include <map>
# include <vector>
# include <utility>

# include "pthread.h"

# include "macros.h"

class AbstractPersistenceManager;

template<typename R>
class PersistentStorage
{
public:
	PersistentStorage(std::string group, std::string id, AbstractPersistenceManager *persistenceManager);
	~PersistentStorage();

	R retrieve(KeyType key, bool* notFound = NULL);
	KeyType store(R record);
	void modify(KeyType key, R record);
	void remove(KeyType key);
	bool containsRecord(R record);
	bool containsKey(KeyType key);
	
	std::map<KeyType, R>* getRecordsToSync();
	std::vector<KeyType>* getDeletedRecordsToSync();

private:
	void swapRecordsForSyncing();
	void swapDeletedRecordsForSyncing();
	
	std::map<KeyType, R> *records;
	std::map<KeyType, R> *changedRecords;
	std::map<KeyType, R> *syncingRecords;
	
	std::vector<KeyType> *deletedRecords;
	std::vector<KeyType> *syncingDeletedRecords;
	
	pthread_mutex_t changedRecordsMutex;
	pthread_mutex_t deletedRecordsMutex;

	std::string group;
	std::string id;
	AbstractPersistenceManager* persistenceManager;
};

template<typename R>
PersistentStorage<R>::PersistentStorage(std::string group, std::string id, AbstractPersistenceManager *persistenceManager) : group(group), id(id), persistenceManager(persistenceManager)
{
	records = new std::map<KeyType, R>();
	changedRecords = new std::map<KeyType, R>();
	syncingRecords = new std::map<KeyType, R>();

	deletedRecords = new std::vector<KeyType>();
	syncingDeletedRecords = new std::vector<KeyType>();

	pthread_mutex_init(&changedRecordsMutex, NULL);
	pthread_mutex_init(&deletedRecordsMutex, NULL);
}
template<typename R>
PersistentStorage<R>::~PersistentStorage()
{
	delete records;
	delete changedRecords;
	delete syncingRecords;

	delete deletedRecords;
	delete syncingDeletedRecords;

	pthread_mutex_destroy(&changedRecordsMutex);
	pthread_mutex_destroy(&deletedRecordsMutex);
}

template<typename R>
void PersistentStorage<R>::swapRecordsForSyncing()
{
	if(!persistenceManager->swapRequested(group,id)) { return; }

	delete syncingRecords;
	
	pthread_mutex_lock(&changedRecordsMutex);

	syncingRecords = changedRecords;
	changedRecords = new std::map<KeyType, R>();

	pthread_mutex_unlock(&changedRecordsMutex);
}
template<typename R>
void PersistentStorage<R>::swapDeletedRecordsForSyncing()
{
	if(!persistenceManager->swapRequested(group,id)) { return; }

	delete syncingDeletedRecords;
	
	pthread_mutex_lock(&deletedRecordsMutex);

	syncingDeletedRecords = deletedRecords;
	deletedRecords = new std::vector<KeyType>();

	pthread_mutex_unlock(&deletedRecordsMutex);
}
template<typename R>
std::map<KeyType, R>* PersistentStorage<R>::getRecordsToSync()
{
	swapRecordsForSyncing();
	return syncingRecords;
}
template<typename R>
std::map<KeyType, R>* PersistentStorage<R>::getDeletedRecordsToSync()
{
	swapDeletedRecordsForSyncing();
	return syncingDeletedRecords;
}

template<typename R>
R PersistentStorage<R>::retrieve(K key, bool* notFound)
{
	if(notFound) { *notFound = false; }

	std::map<KeyType, R>::const_iterator iter = records->find(key);
	if(iter == records->end())
	{
		void* voidRecordPointer = persistenceManager->retrieveRecord(group,id,key);
		if(voidRecordPointer)
		{
			R* recordPointer = static_cast<R*>voidRecordPointer;
			records->insert(std::make_pair(key,*recordPointer));
			return *recordPointer;
		}
		else
		{
			if(notFound) { *notFound = true; }
			return R();
		}
	}

	return iter->second;
}
template<typename R>
KeyType PersistentStorage<R>::store(R record)
{
	KeyType key = persistenceManager->nextKey(group,id);

	modify(key,record);

	return key;
}
template<typename R>
void PersistentStorage<R>::modify(KeyType key, R record)
{
	records->erase(key);
	records->insert(std::make_pair(key,record));

	pthread_mutex_lock(&changedRecordsMutex);
	changedRecords->erase(key);
	changedRecords->insert(std::make_pair(key,record));
	pthread_mutex_unlock(&changedRecordsMutex);
}
template<typename R>
void PersistentStorage<R>::remove(KeyType key)
{
	records->erase(key);

	pthread_mutex_lock(&changedRecordsMutex);
	changedRecords->erase(key);
	pthread_mutex_unlock(&changedRecordsMutex);
	
	pthread_mutex_locK(&deletedRecordsMutex);
	deletedRecords->push_back(key);
	pthread_mutex_unlock(&deletedRecordsMutex);
}
template<typename R>
bool PersistentStorage<R>::containsRecord(R record)
{
	for(std::map<KeyType, R>::const_iterator iter = records->begin(); iter != records->end(); ++iter)
	{
		if(iter->second == record)
		{
			return true;
		}
	}

	return false;
}
template<typename R>
bool PersistentStorage<R>::containsKey(KeyType key)
{
	std::map<KeyType, R>::const_iterator iter = records->find(key);
	if(iter == records->end())
	{
		return false;
	}
	else
	{
		return true;
	}
}
