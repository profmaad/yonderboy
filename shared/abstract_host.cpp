//      abstract_host.cpp
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
# include <sstream>
# include <map>
# include <utility>
# include <stdexcept>
# include <iostream>

# include <cerrno>
# include <cstring>
# include <climits>
# include <unistd.h>
# include <sys/socket.h>

# include "ev_cpp.h"
# include "macros.h"

# include "package.h"
# include "package_factories.h"

# include "abstract_host.h"

AbstractHost::AbstractHost(int hostSocket) : hostSocket(-1), state(Uninitialized), readWatcher(NULL), writeWatcher(NULL), receivedNewline(false)
{
	if(hostSocket < 0) { throw std::invalid_argument("Given file descriptor is invalid"); }

	memset(receiveBuffer,0,4096);
	this->hostSocket = hostSocket;
	state = Connected;
	nextPackageID = 0;

	readWatcher = new ev::io();
	readWatcher->set<AbstractHost, &AbstractHost::readCallback>(this);
	readWatcher->start(hostSocket, ev::READ);

	writeWatcher = new ev::io();
	writeWatcher->set<AbstractHost, &AbstractHost::writeCallback>(this);
}
AbstractHost::~AbstractHost()
{
	if(state == Connected || state == Disconnecting)
	{
		closeSocket(false);
	}

	delete writeWatcher;
	delete readWatcher;
}

void AbstractHost::shutdownHost()
{
	disconnect();
}
void AbstractHost::disconnect()
{
	state = Disconnecting;

	if(sendBuffer.size() == 0)
	{
		shutdownSocket();
	}
}
void AbstractHost::shutdownSocket()
{
	int result = -1;
	char errorBuffer[128] = {'\0'};

	result = shutdown(hostSocket, SHUT_WR);
	if(result < 0)
	{
		state = Connected;
		
		strerror_r(errno,errorBuffer,128);
     		throw std::runtime_error("___SHARED___Host socket shutdown failed");
	}
}
void AbstractHost::closeSocket(bool notifySubclasses)
{
	if(ev::get_default_loop().depth() > 0)
	{
		readWatcher->stop();
		writeWatcher->stop();

	}
	close(hostSocket);
	state = Disconnected;
	if(notifySubclasses) { socketClosed(); }
}

void AbstractHost::readCallback(ev::io &watcher, int revents)
{
	int bytesRead = -1;
	char errorBuffer[128] = {'\0'};
	memset(receiveBuffer, 0, 4096);

	bytesRead = recv(hostSocket, (void*)receiveBuffer, 4096, 0);
	if(bytesRead < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK))
	{
//		strerror_r(errno,errorBuffer,128);
//		throw std::runtime_error("___SHARED___Host socket read failed: "+std::string(errorBuffer));
		throw std::runtime_error("___SHARED___Host socket read failed: "+std::string(strerror_r(errno,errorBuffer,128)));
	}
	else if (bytesRead == 0)
	{
		closeSocket();
	}

	parseReceivedData();
}
void AbstractHost::parseReceivedData()
{
	size_t lastPosition = std::string::npos;

	size_t receiveBufferLength = strlen(receiveBuffer);
	if(receiveBufferLength > 4096)
	{
		receiveBufferLength = 4096;
	}
	std::string buffer(receiveBuffer,receiveBufferLength);

	if(receivedNewline && buffer[0] == '\n') // we received the end of a package, lets process it
	{
		buffer.erase(0,1);
		processPackage();
		receivedNewline = false;
	}

	while((lastPosition = buffer.find("\n\n")) != std::string::npos) // check whether we have received a complete package
	{
		parseBuffer += buffer.substr(0,lastPosition+1); // we include the first newline, because package parsing needs a newline after each line
		buffer.erase(0,lastPosition+2);

		processPackage();
	}

	if(*(buffer.rbegin()) == '\n') { receivedNewline = true; }
	else { receivedNewline = false; }
	parseBuffer += buffer;
}
void AbstractHost::processPackage()
{
	Package *constructedPackage = new Package(parseBuffer);
	if(!constructedPackage->isValid())
	{
		sendPackage(constructAcknowledgementPackage(constructedPackage, "invalid"));
		delete constructedPackage;
	}
	else
	{
		updateNextPackageID(constructedPackage->getID());
		handlePackage(constructedPackage);
	}

	parseBuffer.clear();
}

void AbstractHost::sendPackage(Package *thePackage, bool withID)
{
	if(state != Connected) { return; }

	if( !thePackage || !(thePackage->isValid()) )
	{
		return;
	}

	std::string serializedData = thePackage->serialize();
	if(withID && !thePackage->hasID() && !thePackage->getType() == Acknowledgement && !thePackage->getType() == StatusChange)
	{
		serializedData += "id = ";
		serializedData += getNextPackageID();
		serializedData += '\n';
	}
	serializedData += '\n';

	sendBuffer += serializedData;

	if(! writeWatcher->is_active()) { writeWatcher->start(hostSocket, ev::WRITE); }
}
void AbstractHost::sendPackageAndDelete(Package *thePackage, bool withID)
{
	sendPackage(thePackage, withID);
	delete thePackage;
}

std::string AbstractHost::getNextPackageID()
{
	std::ostringstream conversionStream("");
	conversionStream<<nextPackageID;
	nextPackageID++;
	
	return conversionStream.str();
}
void AbstractHost::updateNextPackageID(unsigned long long lastReceivedID)
{
	if(lastReceivedID >= nextPackageID || (nextPackageID > ULLONG_MAX-100  && lastReceivedID < 100) ) // second case hopefully handles wrap-arounds
	{
		nextPackageID = lastReceivedID+1;
	}
}
void AbstractHost::writeCallback(ev::io &watcher, int revents)
{
	int bytesWritten = -1;
	char errorBuffer[128] = {'\0'};
	std::string buffer;
	int bufferSize = 0;
	int sendBufferSize = sendBuffer.size();

	while(sendBufferSize > 0)
	{
		if(sendBufferSize > 4096)
		{
			buffer = sendBuffer.substr(0,4096);
			bufferSize = 4096;
		}
		else
		{
			buffer = sendBuffer;
			bufferSize = sendBufferSize;
		}

		bytesWritten = send(hostSocket, buffer.c_str(), bufferSize, 0);
		if(bytesWritten < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK) )
		{
//			strerror_r(errno, errorBuffer, 128);
//			throw std::runtime_error("___SHARED___Host socket send failed: "+std::string(errorBuffer));
			throw std::runtime_error("___SHARED___Host socket send failed: "+std::string(strerror_r(errno, errorBuffer, 128)));
		}
		else if(errno == EAGAIN || errno == EWOULDBLOCK) { break; }
		
		sendBuffer.erase(0,bytesWritten);
		sendBufferSize -= bytesWritten;
	}

	if(state == Disconnecting && sendBuffer.size() == 0)
	{
		shutdownSocket();
	}
	else if (sendBuffer.size() == 0)
	{
		writeWatcher->stop();
	}
}
