//      file_persistence_manager.cpp
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
# include <fstream>
# include <exception>

# include <cerrno>
# include <cstdio>
# include <sys/stat.h>

# include <pthread.h>

# include "macros.h"
# include "log.h"
# include "package.h"

# include "abstract_persistence_manager.h"
# include "persistent_storage.h"

# include "file_persistence_manager.h"

FilePersistenceManager::FilePersistenceManager(std::string folder) : baseFolder(folder)
{
	files = new std::map<std::pair<std::string, std::string>, FileInformation*>();
	pthread_mutex_init(&filesMutex, NULL);
	
	LOG_INFO("initialized for base folder "<<baseFolder)
}
FilePersistenceManager::~FilePersistenceManager()
{
	for(std::map<std::pair<std::string, std::string>, FileInformation*>::iterator iter = files->begin(); iter != files->end(); ++iter)
	{
		closeFile(iter->second);
	}
	
	delete files;
	pthread_mutex_destroy(&filesMutex);
}

FilePersistenceManager::FileInformation::FileInformation()
{
	stream = NULL;
	keyPositions = new std::map<KeyType, std::streampos>();
	nextKey = 0;
	
	pthread_mutex_init(&streamMutex, NULL);
	pthread_mutex_init(&keysMutex, NULL);
}

std::fstream* FilePersistenceManager::openFile(std::string path)
{
	std::fstream *result = NULL;
	
	try
	{
		// open file for writing - this creates the file if it doesn't exist
		result = new std::fstream(path.c_str(), std::fstream::out | std::fstream::app );
		result->close();
		delete result;
		
		result = new std::fstream(path.c_str(), std::fstream::in | std::fstream::out );
		
		if(result->fail())
		{
			delete result;
			result = NULL;
		}
	}
	catch(std::exception e)
	{
		delete result;
		result = NULL;
	}
	
	return result;
}
void FilePersistenceManager::closeFile(FileInformation* file, bool runCompaction)
{
	pthread_mutex_lock(&file->streamMutex);
	file->stream->close();
	pthread_mutex_unlock(&file->streamMutex);
	
	if(runCompaction) { compactFile(file->path); }
	
	delete file->stream;
	delete file->keyPositions;
	pthread_mutex_destroy(&file->streamMutex);
	pthread_mutex_destroy(&file->keysMutex);
	
	delete file;
}
bool FilePersistenceManager::createGroupDirectory(std::string group)
{
	std::string path = constructPath(group, "");
	struct stat infos;
	int success = -1;
	char errorBuffer[128] = { '\0' };
	
	success = stat(path.c_str(),&infos);
	if(success == 0 && (infos.st_mode & S_IFDIR)) // path exists and points to a directory
	{
		return true;
	}
	else if(success < 0 && errno == ENOENT) // path does not exist, create directory
	{
		success = mkdir(path.c_str(),0755);
		if(success < 0)
		{
			strerror_r(errno,errorBuffer,128);
			LOG_ERROR("couldn't create group directory ("<<errorBuffer<<")")
			return false;
		}
		else 
		{
			return true;
		}

	}
	else // some error occured, we cant create the directory
	{
		strerror_r(errno,errorBuffer,128);
		LOG_ERROR("couldn't create group directory ("<<errorBuffer<<")")
		return false;
	}

}

std::string FilePersistenceManager::constructPath(std::string group, std::string id)
{
	std::string result = baseFolder;
	result += "/";
	result += group;
	if(id != "")
	{	
		result += "/";
		result += id;
	}
	
	return result;
}

ListRecord* FilePersistenceManager::parseListRecord(std::string raw)
{
	ListRecord *result = NULL;
	
	result = new std::string(raw);
	
	return result;
}
KeyValueRecord* FilePersistenceManager::parseKeyValueRecord(std::string raw)
{
	KeyValueRecord *result = NULL;
	size_t separatorPosition = std::string::npos;
	
	separatorPosition = raw.find("=");
	if(separatorPosition != std::string::npos)
	{
		std::string keyString = raw.substr(0,separatorPosition);
		std::string valueString = raw.substr(separatorPosition+1);
		
		result = new std::pair<std::string,std::string>(Package::trimString(keyString),Package::trimString(valueString));
	}
	else
	{
		result = new std::pair<std::string,std::string>(Package::trimString(raw),std::string()); 
	}
	
	return result;
	
}
TableRecord* FilePersistenceManager::parseTableRecord(std::string raw)
{
	TableRecord *result = new std::vector<std::string>();
	std::string entry;
	size_t lastTab = std::string::npos;
	
	while((lastTab = raw.find("\t")) != std::string::npos)
	{
		entry = raw.substr(0,lastTab);
		raw.erase(0,lastTab+1);
		
		result->push_back(entry);
	}
	result->push_back(raw.substr(lastTab+1,raw.length()-lastTab-1));
	
	return result;
}

void* FilePersistenceManager::retrieveRecord(std::string group, std::string id, KeyType key)
{
	void* result = NULL;
	StorageType type = getStorageType(group, id);
	std::string rawData;
	FileInformation *file = NULL;
	std::streampos keyPosition = 0;
	
	pthread_mutex_lock(&filesMutex);
	std::map<std::pair<std::string, std::string>, FileInformation*>::const_iterator fileIter = files->find(std::make_pair(group,id));
	if(fileIter == files->end())
	{
		pthread_mutex_unlock(&filesMutex);
		return result;
	}
	file = fileIter->second;
	pthread_mutex_unlock(&filesMutex);
	
	pthread_mutex_lock(&file->keysMutex);
	std::map<KeyType, std::streampos>::const_iterator keyIter = file->keyPositions->find(key);
	if(keyIter == fileIter->second->keyPositions->end())
	{
		pthread_mutex_unlock(&file->keysMutex);
		return result;
	}
	keyPosition = keyIter->second;
	pthread_mutex_unlock(&file->keysMutex);
	
	pthread_mutex_lock(&fileIter->second->streamMutex);
	file->stream->seekg(keyPosition);
	if(! std::getline(*(file->stream),rawData))
	{
		pthread_mutex_unlock(&file->streamMutex);
		return result;
	}
	pthread_mutex_unlock(&file->streamMutex);
	
	switch(type)
	{
		case ListStorage:
			result = static_cast<void*>(parseListRecord(rawData));
			break;
		case KeyValueStorage:
			result = static_cast<void*>(parseKeyValueRecord(rawData));
			break;
		case TableStorage:
			result = static_cast<void*>(parseTableRecord(rawData));
			break;
	}
	
	return result;
}
KeyType FilePersistenceManager::nextKey(std::string group, std::string id)
{
	KeyType result = KEYTYPE_INVALID_VALUE;
	FileInformation *file = NULL;
	
	pthread_mutex_lock(&filesMutex);
	std::map<std::pair<std::string, std::string>, FileInformation*>::const_iterator iter = files->find(std::make_pair(group,id));
	if(iter != files->end())
	{
		file = iter->second;
	}
	pthread_mutex_unlock(&filesMutex);
	
	if(file)
	{
		pthread_mutex_lock(&file->keysMutex);
		result = file->nextKey++;
		pthread_mutex_unlock(&file->keysMutex);
	}
	
	return result;
}


PersistentListStorage* FilePersistenceManager::createListStorage(std::string group, std::string id)
{
	PersistentListStorage *storage = NULL;
	if(!createGroupDirectory(group)) { return storage; }
	
	std::fstream *stream = openFile(constructPath(group, id));	
	if(!stream) { return storage; }

	FileInformation *infos = new FileInformation;
		infos->path = constructPath(group,id);
		infos->stream = stream;
	
	pthread_mutex_lock(&filesMutex);
	files->insert(std::make_pair(std::make_pair(group,id), infos));
	pthread_mutex_unlock(&filesMutex);
	
	storage = new PersistentListStorage(group, id, this, ListStorage);
	
	ReaderThreadData *threadData = new ReaderThreadData;
	threadData->persistenceManager = this;
	threadData->storage = storage;
	threadData->file = infos;
	threadData->type = ListStorage;
	
	if(!server->allowedToBlock())
	{
		pthread_t *thread;
		int threadSuccess = pthread_create(thread,NULL,readRecordsIntoStorage,threadData);
		if(threadSuccess < 0)
		{
			pthread_mutex_lock(&filesMutex);
			files->erase(std::make_pair(group,id));
			pthread_mutex_unlock(&filesMutex);
		
			closeFile(infos, false);
			delete storage;
			delete threadData;
			
			return NULL;
		}
		pthread_detach(*thread);
	}
	else
	{
		readRecordsIntoStorage(threadData);
	}

	
	return storage;
}
PersistentKeyValueStorage* FilePersistenceManager::createKeyValueStorage(std::string group, std::string id)
{
	PersistentKeyValueStorage *storage = NULL;
	if(!createGroupDirectory(group)) { return storage; }
	
	std::fstream *stream = openFile(constructPath(group, id));
	if(!stream) { return storage; }
	
	FileInformation *infos = new FileInformation;
		infos->path = constructPath(group,id);
		infos->stream = stream;
	
	pthread_mutex_lock(&filesMutex);
	files->insert(std::make_pair(std::make_pair(group,id), infos));
	pthread_mutex_unlock(&filesMutex);
	
	storage = new PersistentKeyValueStorage(group, id, this, KeyValueStorage);
	
	ReaderThreadData *threadData = new ReaderThreadData;
	threadData->persistenceManager = this;
	threadData->storage = storage;
	threadData->file = infos;
	threadData->type = KeyValueStorage;
	
	if(!server->allowedToBlock())
	{
		pthread_t *thread;
		int threadSuccess = pthread_create(thread,NULL,readRecordsIntoStorage,threadData);
		if(threadSuccess < 0)
		{
			pthread_mutex_lock(&filesMutex);
			files->erase(std::make_pair(group,id));
			pthread_mutex_unlock(&filesMutex);
		
			closeFile(infos, false);
			delete storage;
			delete threadData;
		
			return NULL;
		}
		pthread_detach(*thread);
	}
	else
	{
		readRecordsIntoStorage(threadData);
	}

	
	return storage;
}
PersistentTableStorage* FilePersistenceManager::createTableStorage(std::string group, std::string id)
{
	PersistentTableStorage *storage = NULL;
	if(!createGroupDirectory(group)) { return storage; }
	
	std::fstream *stream = openFile(constructPath(group, id));
	if(!stream) { return storage; }
	
	FileInformation *infos = new FileInformation;
		infos->path = constructPath(group,id);
		infos->stream = stream;
	
	pthread_mutex_lock(&filesMutex);
	files->insert(std::make_pair(std::make_pair(group,id), infos));
	pthread_mutex_unlock(&filesMutex);
	
	storage = new PersistentTableStorage(group, id, this, TableStorage);	
	
	ReaderThreadData *threadData = new ReaderThreadData;
	threadData->persistenceManager = this;
	threadData->storage = storage;
	threadData->file = infos;
	threadData->type = TableStorage;
	
	if(!server->allowedToBlock())
	{
		pthread_t *thread;
		int threadSuccess = pthread_create(thread,NULL,readRecordsIntoStorage,threadData);
		if(threadSuccess < 0)
		{
			pthread_mutex_lock(&filesMutex);
			files->erase(std::make_pair(group,id));
			pthread_mutex_unlock(&filesMutex);
		
			closeFile(infos, false);
			delete storage;
			delete threadData;
		
			return NULL;
		}
		pthread_detach(*thread);
	}
	else
	{
		readRecordsIntoStorage(threadData);
	}
	
	return storage;
}

void FilePersistenceManager::destroyStorage(std::string group, std::string id)
{
	pthread_mutex_lock(&filesMutex);
	std::map<std::pair<std::string, std::string>, FileInformation*>::iterator iter = files->find(std::make_pair(group,id));
	if(iter != files->end())
	{
		closeFile(iter->second);
		files->erase(iter);
		
		LOG_DEBUG("destroyed storage "<<group<<"/"<<id)
	}
	pthread_mutex_unlock(&filesMutex);
}

void FilePersistenceManager::writeRecord(std::string group, std::string id, KeyType key, std::string data)
{
	std::streampos newPosition = 0;
	FileInformation *file = NULL;
	
	pthread_mutex_lock(&filesMutex);
	std::map<std::pair<std::string, std::string>, FileInformation*>::const_iterator fileIter = files->find(std::make_pair(group,id));
	if(fileIter == files->end())
	{
		LOG_DEBUG("fileIter not found")
		pthread_mutex_unlock(&filesMutex);
		return;
	}
	file = fileIter->second;
	pthread_mutex_unlock(&filesMutex);
	
	deleteRecord(group,id,key);
	
	// write the new record at the end of the file, and remember the new position for the key
	pthread_mutex_lock(&file->streamMutex);
	file->stream->seekp(0, std::ios_base::end);
	newPosition = file->stream->tellp();
	file->stream->write(data.c_str(), data.length());
	file->stream->put('\n');
	pthread_mutex_unlock(&file->streamMutex);
	
	// store the new position
	pthread_mutex_lock(&file->keysMutex);
	file->keyPositions->erase(key);
	file->keyPositions->insert(std::make_pair(key, newPosition));
	pthread_mutex_unlock(&file->keysMutex);
	LOG_DEBUG("writeRecord(): "<<group<<", "<<id<<", "<<key<<", '"<<data<<"', "<<newPosition)
}
void FilePersistenceManager::writeListRecord(std::string group, std::string id, KeyType key, ListRecord record)
{
	writeRecord(group,id,key,record);
}
void FilePersistenceManager::writeKeyValueRecord(std::string group, std::string id, KeyType key, KeyValueRecord record)
{
	std::string data = record.first;
	data += " = ";
	data += record.second;
	
	writeRecord(group,id,key,data);
}
void FilePersistenceManager::writeTableRecord(std::string group, std::string id, KeyType key, TableRecord record)
{
	std::string data;
	
	for(TableRecord::const_iterator iter = record.begin(); iter != record.end(); ++iter)
	{
		data += *iter;
		data += "\t";
	}
	data.erase(data.length()-1,1);
	
	writeRecord(group,id,key,data);
}

void FilePersistenceManager::deleteRecord(std::string group, std::string id, KeyType key)
{
	std::string oldData;
	FileInformation *file = NULL;
	std::streampos keyPosition = 0;
	
	pthread_mutex_lock(&filesMutex);
	std::map<std::pair<std::string, std::string>, FileInformation*>::const_iterator fileIter = files->find(std::make_pair(group,id));
	if(fileIter == files->end())
	{
		pthread_mutex_unlock(&filesMutex);
		return;
	}
	file = fileIter->second;
	pthread_mutex_unlock(&filesMutex);
	
	pthread_mutex_lock(&file->keysMutex);
	std::map<KeyType, std::streampos>::iterator keyIter = fileIter->second->keyPositions->find(key);
	if(keyIter == fileIter->second->keyPositions->end())
	{
		pthread_mutex_unlock(&file->keysMutex);
		return;
	}
	keyPosition = keyIter->second;
	pthread_mutex_unlock(&file->keysMutex);
	
	LOG_DEBUG(group<<","<<id<<","<<key<<","<<keyPosition)
	
	
	// extract the old data in order to determine its length
	pthread_mutex_lock(&file->streamMutex);
	file->stream->seekg(keyPosition);
	LOG_DEBUG("good bit "<<std::boolalpha<<file->stream->good())
	if(std::getline(*(file->stream),oldData))
	{
		// overwrite the old data with whitespace - this marks a line for removal upon compaction
		file->stream->seekp(keyPosition);
		LOG_DEBUG("tellp "<<file->stream->tellp())
		file->stream->write(std::string(oldData.length(),' ').c_str(),oldData.length());
		LOG_DEBUG("good bit "<<std::boolalpha<<file->stream->good())
		LOG_DEBUG("wrote "<<oldData.length()<<" whitespaces")
	}
	pthread_mutex_unlock(&file->streamMutex);
}

void FilePersistenceManager::compactFile(std::string path)
{
	std::string line;
	
	std::string outputPath = path;
	outputPath += ".compacted";
	
	std::ifstream input(path.c_str());
	std::ofstream output(outputPath.c_str());
	
	while(!input.eof())
	{
		std::getline(input,line);
		if(line.find_first_not_of(" \n") != std::string::npos)
		{
			output<<line<<std::endl;
		}
	}
	
	input.close();
	output.close();
	
	rename(outputPath.c_str(),path.c_str());
}

void* FilePersistenceManager::readRecordsIntoStorage(void *dataPointer)
{
	if(!dataPointer) { return NULL; }
	
	ReaderThreadData *data = static_cast<ReaderThreadData*>(dataPointer);
	FileInformation *file = data->file;
	
	ListRecord *listRecord = NULL;
	KeyValueRecord *keyValueRecord = NULL;
	TableRecord *tableRecord = NULL;
	
	std::streampos recordStartPosition = std::ios_base::beg;
	std::streampos nextRecordPosition = std::ios_base::beg;
	std::string rawData;
	KeyType lastKey = KEYTYPE_INVALID_VALUE;
	
	pthread_mutex_lock(&file->streamMutex);
	while(!file->stream->eof())
	{
		recordStartPosition = nextRecordPosition;
		file->stream->seekg(recordStartPosition);
		if(! std::getline(*(file->stream),rawData))
		{
			break;
		}
		nextRecordPosition = file->stream->tellg();
		pthread_mutex_unlock(&file->streamMutex);
		
		switch(data->type)
		{
			case ListStorage:
				listRecord = data->persistenceManager->parseListRecord(rawData);
				lastKey = static_cast<PersistentListStorage*>(data->storage)->store(*listRecord, false);
				LOG_INFO("retrieved record ("<<lastKey<<","<<recordStartPosition<<") "<<*listRecord)
				delete listRecord;
				break;
			case KeyValueStorage:
				keyValueRecord = data->persistenceManager->parseKeyValueRecord(rawData);
				lastKey = static_cast<PersistentKeyValueStorage*>(data->storage)->store(*keyValueRecord, false);
				LOG_INFO("retrieved record ("<<lastKey<<","<<recordStartPosition<<") "<<keyValueRecord->first<<"|"<<keyValueRecord->second)
				delete keyValueRecord;
				break;
			case TableStorage:
				tableRecord = data->persistenceManager->parseTableRecord(rawData);
				lastKey = static_cast<PersistentTableStorage*>(data->storage)->store(*tableRecord, false);
				delete tableRecord;
				break;
		}
		
		pthread_mutex_lock(&file->keysMutex);
		file->keyPositions->insert(std::make_pair(lastKey,recordStartPosition));
		pthread_mutex_unlock(&file->keysMutex);
		
		pthread_mutex_lock(&file->streamMutex);
	}
	file->stream->clear();
	pthread_mutex_unlock(&data->file->streamMutex);
	
	delete data;
	
	return NULL;
}
