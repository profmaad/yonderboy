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

# endif /*MACROS_H*/
