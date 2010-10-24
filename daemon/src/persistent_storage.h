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

// this class should be both thread-safe and reentrant

# ifndef PERSISTENT_STORAGE_H
# define PERSISTENT_STORAGE_H

# include <string>
# include <map>
# include <vector>
# include <utility>

# include <pthread.h>

# include "macros.h"
# include "log.h"

class AbstractPersistenceManager;

template<typename R>
class PersistentStorage
{
public:
	PersistentStorage(std::string group, std::string id, AbstractPersistenceManager *persistenceManager, StorageType type);
	~PersistentStorage();

	R retrieve(KeyType key, bool *notFound = NULL);
	KeyType store(R record, bool isChange = true);
	void modify(KeyType key, R record, bool isChange = true);
	void remove(KeyType key);
	bool containsRecord(R record);
	bool containsKey(KeyType key);
	std::vector<KeyType> listKeys();
	
	void lockForWriting();
	void allRecordsMadeAvailable();
	
	std::map<KeyType, R>* getRecordsToSync();
	std::vector<KeyType>* getDeletedRecordsToSync();
	
	std::string getGroup() { return group; }
	std::string getID() { return id; }

private:
	void swapRecordsForSyncing();
	void swapDeletedRecordsForSyncing();
	
	std::map<KeyType, R> *records;
	std::map<KeyType, R> *changedRecords;
	std::map<KeyType, R> *syncingRecords;
	
	std::vector<KeyType> *deletedRecords;
	std::vector<KeyType> *syncingDeletedRecords;
	
	pthread_rwlock_t recordsLock;
	pthread_mutex_t changedRecordsMutex;
	pthread_mutex_t deletedRecordsMutex;

	std::string group;
	std::string id;
	bool lockedForWriting;
	bool allRecordsAvailable;
	AbstractPersistenceManager* persistenceManager;
	StorageType type;
};

template<typename R>
PersistentStorage<R>::PersistentStorage(std::string group, std::string id, AbstractPersistenceManager *persistenceManager, StorageType type) : group(group), id(id), persistenceManager(persistenceManager), type(type)
{
	lockedForWriting = false;
	allRecordsAvailable = false;
	
	records = new std::map<KeyType, R>();
	changedRecords = new std::map<KeyType, R>();
	syncingRecords = new std::map<KeyType, R>();

	deletedRecords = new std::vector<KeyType>();
	syncingDeletedRecords = new std::vector<KeyType>();

	pthread_rwlock_init(&recordsLock, NULL);
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

	pthread_rwlock_destroy(&recordsLock);
	pthread_mutex_destroy(&changedRecordsMutex);
	pthread_mutex_destroy(&deletedRecordsMutex);
}

template<typename R>
void PersistentStorage<R>::swapRecordsForSyncing()
{
	delete syncingRecords;
	
	pthread_mutex_lock(&changedRecordsMutex);

	syncingRecords = changedRecords;
	changedRecords = new std::map<KeyType, R>();

	pthread_mutex_unlock(&changedRecordsMutex);
}
template<typename R>
void PersistentStorage<R>::swapDeletedRecordsForSyncing()
{
	delete syncingDeletedRecords;
	
	pthread_mutex_lock(&deletedRecordsMutex);

	syncingDeletedRecords = deletedRecords;
	deletedRecords = new std::vector<KeyType>();

	pthread_mutex_unlock(&deletedRecordsMutex);
}
template<typename R>
std::map<KeyType, R>* PersistentStorage<R>::getRecordsToSync()
{
	if(!persistenceManager->swapRequested(group,id)) { return NULL; }
	
	swapRecordsForSyncing();
	return syncingRecords;
}
template<typename R>
std::vector<KeyType>* PersistentStorage<R>::getDeletedRecordsToSync()
{
	if(!persistenceManager->swapRequested(group,id)) { return NULL; }

	swapDeletedRecordsForSyncing();
	return syncingDeletedRecords;
}

template<typename R>
R PersistentStorage<R>::retrieve(KeyType key, bool *notFound)
{
	if(notFound) { *notFound = false; }

	pthread_rwlock_rdlock(&recordsLock);
	typename std::map<KeyType, R>::const_iterator iter = records->find(key);
	if(iter == records->end() && !lockedForWriting)
	{
		pthread_rwlock_unlock(&recordsLock);
		void* voidRecordPointer = persistenceManager->retrieveRecord(group,id,key);
		if(voidRecordPointer)
		{
			R* recordPointer = static_cast<R*>(voidRecordPointer);
			pthread_rwlock_wrlock(&recordsLock);
			records->insert(std::make_pair(key,*recordPointer));
			pthread_rwlock_unlock(&recordsLock);
			return *recordPointer;
		}
		else
		{
			if(notFound) { *notFound = true; }
			return R();
		}
	}
	else if (iter == records->end() && lockedForWriting)
	{
		if(notFound) { *notFound = true; }
		return R();
	}

	R result = iter->second;
	pthread_rwlock_unlock(&recordsLock);
	
	return result;
}
template<typename R>
KeyType PersistentStorage<R>::store(R record, bool isChange)
{
	if(lockedForWriting) { return KEYTYPE_INVALID_VALUE; }

	KeyType key = persistenceManager->nextKey(group,id);

	modify(key,record, isChange);

	return key;
}
template<typename R>
void PersistentStorage<R>::modify(KeyType key, R record, bool isChange)
{
	if(lockedForWriting) { return; }
	
	pthread_rwlock_wrlock(&recordsLock);
	records->erase(key);
	records->insert(std::make_pair(key,record));
	pthread_rwlock_unlock(&recordsLock);

	if(isChange)
	{
		pthread_mutex_lock(&changedRecordsMutex);
		changedRecords->erase(key);
		changedRecords->insert(std::make_pair(key,record));
		pthread_mutex_unlock(&changedRecordsMutex);
	
		persistenceManager->recordsChanged(this, type);
	}
}
template<typename R>
void PersistentStorage<R>::remove(KeyType key)
{
	if(lockedForWriting) { return; }
	
	pthread_rwlock_wrlock(&recordsLock);
	records->erase(key);
	pthread_rwlock_unlock(&recordsLock);

	pthread_mutex_lock(&changedRecordsMutex);
	changedRecords->erase(key);
	pthread_mutex_unlock(&changedRecordsMutex);
	
	pthread_mutex_lock(&deletedRecordsMutex);
	deletedRecords->push_back(key);
	pthread_mutex_unlock(&deletedRecordsMutex);
	
	persistenceManager->recordsChanged(this, type);
}
template<typename R>
bool PersistentStorage<R>::containsRecord(R record)
{
	bool result = false;
	
	pthread_rwlock_rdlock(&recordsLock);
	for(typename std::map<KeyType, R>::const_iterator iter = records->begin(); iter != records->end(); ++iter)
	{
		if(iter->second == record)
		{
			result = true;
			break;
		}
	}
	pthread_rwlock_unlock(&recordsLock);

	return result;
}
template<typename R>
bool PersistentStorage<R>::containsKey(KeyType key)
{
	bool result = false;
	
	pthread_rwlock_rdlock(&recordsLock);
	typename std::map<KeyType, R>::const_iterator iter = records->find(key);
	if(iter != records->end())
	{
		result = true;
	}
	pthread_rwlock_unlock(&recordsLock);
	
	return result;
}
template<typename R>
std::vector<KeyType> PersistentStorage<R>::listKeys()
{
	std::vector<KeyType> result;
	
	for(typename std::map<KeyType, R>::const_iterator iter = records->begin(); iter != records->end(); ++iter)
	{
		result.push_back(iter->first);
	}
	
	return result;
}

template<typename R>
void PersistentStorage<R>::lockForWriting()
{
	lockedForWriting = true;
}

template<typename R>
void PersistentStorage<R>::allRecordsMadeAvailable()
{
	allRecordsAvailable = true;
}

# endif /*PERSISTENT_STORAGE_H*/
