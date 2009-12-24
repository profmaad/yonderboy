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
# include <deque>
# include <set>
# include <string>
# include <utility>
# include <pthread.h>
# include <semaphore.h>

# include "macros.h"

template<typename R>
class PersistentStorage;

typedef std::string ListRecord;
typedef std::pair<std::string, std::string> KeyValueRecord;
typedef std::vector<std::string> TableRecord;

typedef PersistentStorage<ListRecord> PersistentListStorage;
typedef PersistentStorage<KeyValueRecord> PersistentKeyValueStorage;
typedef PersistentStorage<TableRecord> PersistentTableStorage;

class AbstractPersistenceManager
{
public:
	AbstractPersistenceManager();
	virtual ~AbstractPersistenceManager();

	// PersistenceManager API
	PersistentListStorage* retrieveListStorage(std::string group, std::string id);
	PersistentKeyValueStorage* retrieveKeyValueStorage(std::string group, std::string id);
	PersistentTableStorage* retrieveTableStorage(std::string group, std::string id);
	void releaseStorage(std::string group, std::string id);

	// PersistentStorage API
	bool swapRequested(std::string group, std::string id);
	virtual void* retrieveRecord(std::string group, std::string id, KeyType key) = 0;
	virtual KeyType nextKey(std::string group, std::string id) = 0;
	void recordsChanged(void *storage, StorageType type);
	void close();

protected:
	// Implementation API
	virtual PersistentListStorage* createListStorage(std::string group, std::string id) = 0;
	virtual PersistentKeyValueStorage* createKeyValueStorage(std::string group, std::string id) = 0;
	virtual PersistentTableStorage* createTableStorage(std::string group, std::string id) = 0;
	
	virtual void destroyStorage(std::string group, std::string id) = 0;
	
	virtual void writeListRecord(std::string group, std::string id, KeyType key, ListRecord record) = 0;
	virtual void writeKeyValueRecord(std::string group, std::string id, KeyType key, KeyValueRecord record) = 0;
	virtual void writeTableRecord(std::string group, std::string id, KeyType key, TableRecord record) = 0;
	
	virtual void deleteRecord(std::string group, std::string id, KeyType key) = 0;
	
	StorageType getStorageType(std::string group, std::string id);

private:
	void releaseStorageButDontRemove(std::string group, std::string id);
	
	PersistentListStorage* getListStorage(std::string group, std::string id);
	PersistentKeyValueStorage* getKeyValueStorage(std::string group, std::string id);
	PersistentTableStorage* getTableStorage(std::string group, std::string id);
	
	bool storageHasChanges(PersistentListStorage* storage) { return storageHasChanges(static_cast<void*>(storage)); }
	bool storageHasChanges(PersistentKeyValueStorage* storage) { return storageHasChanges(static_cast<void*>(storage)); }
	bool storageHasChanges(PersistentTableStorage* storage) { return storageHasChanges(static_cast<void*>(storage)); }
	bool storageHasChanges(void* storage);
	
	static void* syncRecordsStartMethod(void* persistenceManagerPointer);
	void* syncRecords();
	
	std::map<std::string, std::map<std::string, PersistentListStorage*> > *listStorages;
	std::map<std::string, std::map<std::string, PersistentKeyValueStorage*> > *keyValueStorages;
	std::map<std::string, std::map<std::string, PersistentTableStorage*> > *tableStorages;
	
	std::deque<std::pair<void*, StorageType> > *changedStorages;
	std::set<void*> *releasedStorages;
	
	std::string syncingGroup;
	std::string syncingID;
	
	pthread_t *syncingThread;
	bool syncingShouldEnd;
	
	pthread_mutex_t syncingStorageMutex; // protects syncingGroup, syncingID
	pthread_mutex_t changedStoragesMutex; // protects changedStorages, releasedStorages, changedStoragesAvailable
	pthread_cond_t changedStoragesAvailable; // gets signaled whenever entries where added to changedXStorages
	pthread_mutex_t syncingShouldEndMutex; // protects syncingShouldEnd;
};

# endif /*ABSTRACT_PERSISTENCE_MANAGER_H*/
