//      log.h
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

# ifndef LOG_H
# define LOG_H

# include <iostream>

# include "macros.h"
# include "server_controller.h"

extern ServerController *server;

# define DEFAULT_LOG_LEVEL LogLevelDebug

# define LOG_DEBUG(s) if( (server && server->getLogLevel() <= LogLevelDebug) || DEFAULT_LOG_LEVEL <= LogLevelDebug) { std::clog<<"DEBUG: "<<"["<<__FILE__<<":"<<__LINE__<<"] ("<<__FUNCTION__<<") "<<s<<std::endl; }
# define LOG_INFO(s) if( (server && server->getLogLevel() <= LogLevelInfo) || DEFAULT_LOG_LEVEL <= LogLevelInfo) { std::clog<<"INFO: "<<"["<<__FILE__<<":"<<__LINE__<<"] ("<<__FUNCTION__<<") "<<s<<std::endl; }
# define LOG_WARNING(s) if( (server && server->getLogLevel() <= LogLevelWarning) || DEFAULT_LOG_LEVEL <= LogLevelWarning) { std::clog<<"WARNING: "<<"["<<__FILE__<<":"<<__LINE__<<"] ("<<__FUNCTION__<<") "<<s<<std::endl; }
# define LOG_ERROR(s) if( (server && server->getLogLevel() <= LogLevelError) || DEFAULT_LOG_LEVEL <= LogLevelError) { std::clog<<"ERROR: "<<"["<<__FILE__<<":"<<__LINE__<<"] ("<<__FUNCTION__<<") "<<s<<std::endl; }
# define LOG_FATAL(s) if( (server && server->getLogLevel() <= LogLevelFatal) || DEFAULT_LOG_LEVEL <= LogLevelFatal) { std::clog<<"FATAL: "<<"["<<__FILE__<<":"<<__LINE__<<"] ("<<__FUNCTION__<<") "<<s<<std::endl; }

# define LOG(s) std::clog<<s<<std::endl;

# endif /*LOG_H*/
