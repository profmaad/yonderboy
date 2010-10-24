//      file_persistence_manager.h
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

# ifndef FILE_PERSISTENCE_MANAGER_H
# define FILE_PERSISTENCE_MANAGER_H

# include <string>
# include <vector>
# include <map>
# include <utility>
# include <fstream>

# include <pthread.h>

# include "macros.h"

# include "abstract_persistence_manager.h"

# define COMMENT_CHARACTER '#'

class FilePersistenceManager : public AbstractPersistenceManager
{
public:
	FilePersistenceManager(std::string folder, bool useCompaction = true);
	~FilePersistenceManager();
	
	void* retrieveRecord(std::string group, std::string id, KeyType key);
	KeyType nextKey(std::string group, std::string id);
	
protected:
	PersistentListStorage* createListStorage(std::string group, std::string id);
	PersistentKeyValueStorage* createKeyValueStorage(std::string group, std::string id);
	PersistentTableStorage* createTableStorage(std::string group, std::string id);
	
	void destroyStorage(std::string group, std::string id);
	
	void writeListRecord(std::string group, std::string id, KeyType key, ListRecord record);
	void writeKeyValueRecord(std::string group, std::string id, KeyType key, KeyValueRecord record);
	void writeTableRecord(std::string group, std::string id, KeyType key, TableRecord record);
	
	void deleteRecord(std::string group, std::string id, KeyType key);
	
private:
	/**
	 * \brief Struct to keep information about one open file
	 */
	struct FileInformation
	{
		std::string path;
		std::fstream *stream;
		pthread_mutex_t streamMutex; ///< Mutex to protect FilePersistenceManager::FileInformation::stream
		std::map<KeyType, std::streampos> *keyPositions;
		KeyType nextKey;
		pthread_mutex_t keysMutex; ///< Mutex to protect both FilePersistenceManager::FileInformation::keyPositions and FilePersistenceManager::FileInformation::nextKey
		
		FileInformation();
	};
	
	struct ReaderThreadData
	{
		FilePersistenceManager *persistenceManager;
		void *storage;
		FileInformation *file;
		StorageType type;
	};
	
	std::fstream* openFile(std::string path);
	void closeFile(FileInformation* file, bool runCompaction = true);
	bool createGroupDirectory(std::string group);
	
	std::string constructPath(std::string group, std::string id);
	
	ListRecord* parseListRecord(std::string raw);
	KeyValueRecord* parseKeyValueRecord(std::string raw);
	TableRecord* parseTableRecord(std::string raw);
	
	void writeRecord(std::string group, std::string id, KeyType key, std::string data);
	void compactFile(std::string path);
	
	static void* readRecordsIntoStorage(void *dataPointer);
	
	std::map<std::pair<std::string, std::string>, FileInformation*> *files;
	pthread_mutex_t filesMutex; ///< Mutex to protect FilePersistenceManager::files
	
	std::string baseFolder;
	bool useCompaction;
};

# endif /*FILE_PERSISTENCE_MANAGER_H*/
