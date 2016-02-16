/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "ConnectionMap.h"
#include <netdb.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <stdlib.h>

using namespace srch2::httpwrapper;

namespace srch2 {
namespace httpwrapper {

 int recieveGreeting(int fd, NodeInfo& nodeInfo, bool noTimeout = false) {
	char greetings[sizeof(GREETING_MESSAGE)+sizeof(NodeInfo)+1];
	memset(greetings, 0, sizeof(greetings));
	char *currentPos = greetings;
	int remaining = sizeof(GREETING_MESSAGE) + sizeof(NodeInfo);
	fd_set checkConnect;
	timeval timeout;
	while(remaining) {
		//prevent infinite hanging in except so listening socket closes
		FD_ZERO(&checkConnect);
		FD_SET(fd, &checkConnect);
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		if(select(fd+1, &checkConnect, NULL, NULL, (noTimeout) ? NULL : &timeout)
				!=1) {
			perror("select failed : ");
			break;
		}

		int readSize = read(fd, currentPos, remaining);
		if(readSize <= 0) {
			if(readSize == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
				continue;
			close(fd);
			return -1;
		}
		remaining -= readSize;
		currentPos += readSize;
	}
	if(remaining ||
			memcmp(greetings, GREETING_MESSAGE, sizeof(GREETING_MESSAGE))) {
		//incorrect greeting
		close(fd);
		return -1;
	}
	nodeInfo = *((NodeInfo *)(greetings + sizeof(GREETING_MESSAGE)));;
	return nodeInfo.nodeId;
}

bool sendGreeting(int fd, bool greeted, const NodeInfo& node) {

	char greetings[sizeof(GREETING_MESSAGE) + sizeof(NodeInfo) + 1];
	memset(greetings, 0, sizeof(greetings));

	char *currentPos = greetings;
	int remaining = sizeof(GREETING_MESSAGE) + sizeof(NodeInfo);
	memcpy(greetings, (greeted) ? GREETING_MESSAGE : FAILED_GREETING_MESSAGE,
			sizeof(GREETING_MESSAGE));

	memcpy(greetings + sizeof(GREETING_MESSAGE), (char *)&node, sizeof(NodeInfo));

	while(true) {
#ifdef __MACH__
		int flag = SO_NOSIGPIPE;
#else
		int flag = MSG_NOSIGNAL;
#endif
		int writeSize = send(fd, currentPos, remaining, flag);
		if(writeSize == -1) {
			if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
			close(fd);
			return false;
		}
		remaining -= writeSize;
		if(remaining == 0) {
			return true;
		}
		currentPos += writeSize;
	}
	return false;
}

bool ConnectionMap::isConnectionExist(NodeId nodeId) {
	boost::unique_lock< boost::shared_mutex > lock(_access);

	return nodeConnectionMap.count(nodeId);
}

void ConnectionMap::acceptConnection(int fd, NodeId distinationNodeId) {

	fcntl(fd, F_SETFL, O_NONBLOCK);
	Logger::console("Adding node = %d to known connection", distinationNodeId);
	addNodeConnection(distinationNodeId, fd);
}

void ConnectionMap::addNodeConnection(NodeId nodeId, int fd) {
	//look into routeMap thread safety
	boost::unique_lock< boost::shared_mutex > lock(_access);
	nodeConnectionMap[nodeId] = Connection(fd, nodeId);
}

Connection& ConnectionMap::getConnection(NodeId nodeId) {
	return nodeConnectionMap[nodeId];
}

ConnectionMap::iterator ConnectionMap::begin() { return nodeConnectionMap.begin(); }
ConnectionMap::iterator ConnectionMap::end() { return nodeConnectionMap.end(); }

}}
