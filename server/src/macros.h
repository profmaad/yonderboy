//      macros.h
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

# ifndef MACROS_H
# define MACROS_H

# include <map>
# include <string>

// Typedefs
typedef std::map<std::string, std::string> KeyValueMap;
typedef long long KeyType;
# define KEYTYPE_INVALID_VALUE -1

enum ConnectionState
{
	Uninitialized,
	Disconnected,
	Connecting,
	Connected,
	Established,
	Disconnecting,
	Bound,
	Listening
};

enum PackageType
{
	Unknown,
	Command,
	StatusChange,
	Signal,
	Request,
	Response,
	Acknowledgement,
	ConnectionManagement
};

enum Decision
{
	Invalid = -1,
	Negative = 0,
	Positive = 1,
	CantDecide = -2
};

enum Entity
{
	Cookie,
	SSLCertificate,
	Download,
	Login,
	FormData,
	Website,
	HistoryEntry,
	Bookmark
};

enum StorageType
{
	InvalidStorage = -1,
	ListStorage,
	KeyValueStorage,
	TableStorage,
};

enum ServerState
{
	ServerStateUninitialized,		// binary was just executed, no work done yet - blocking not allowed
	ServerStateInitializing,		// static initialization taking place - blocking allowed
	ServerStateInitialized,			// all static init work done - blocking not allowed
	ServerStateStarting,			// server was asked to start running, is about to do so  - blocking allowed
	ServerStateRunning,				// server is fully up and running - during this state, no blocking and no crashing is alowed
	ServerStateShuttingDown			// server is going down, doing deinitialization - blocking is allowed, crashing is not
};

enum LogLevel
{
	LogLevelDebug = 0,
	LogLevelInfo = 10,
	LogLevelWarning = 20,
	LogLevelError = 30,
	LogLevelFatal = 40
};


# endif /*MACROS_H*/
