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

#include "Node.h"
#include "core/util/SerializationHelper.h"
#include <sstream>

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {


Node::Node()
{
	this->nodeId = 0;
	this->nodeName = "";
	this->ipAddress = "";
	this->portNumber = 0;
	this->nodeMaster = false;
	this->nodeMasterEligible = true;
	this->thisIsMe = false;
}

Node::Node(const std::string& nodeName, const std::string& ipAddress,
		unsigned portNumber, bool thisIsMe, unsigned numberOfPShards, bool nodeMasterEligible){
	this->nodeId = 0;
	this->nodeName = nodeName;
	this->ipAddress = ipAddress;
	this->portNumber = portNumber;
	this->thisIsMe = thisIsMe;
	this->nodeMaster = false;
	this->nodeMasterEligible = nodeMasterEligible;
}

bool Node::isMasterEligible() const {
	return nodeMasterEligible;
}
bool Node::isMaster() const {
	return nodeMaster;
}

void Node::setMaster(bool isMaster) {
	nodeMaster = isMaster;
}
std::string Node::getName() const {
	return this->nodeName;
}

std::string Node::getIpAddress() const {
	return this->ipAddress;
}

unsigned int Node::getId() const {
	return this->nodeId;
}

void Node::setId(unsigned nodeId){
	this->nodeId = nodeId;
}

unsigned int Node::getPortNumber() const{
	return this->portNumber;
}

//serializes the object to a byte array and places array into the region
//allocated by given allocator
void* Node::serializeForNetwork(void * buffer){
	buffer = srch2::util::serializeFixedTypes(nodeId, buffer);
	buffer = srch2::util::serializeString(ipAddress, buffer);
	buffer = srch2::util::serializeFixedTypes(portNumber, buffer);
	buffer = srch2::util::serializeString(nodeName, buffer);
	buffer = srch2::util::serializeFixedTypes(nodeMaster, buffer);
	buffer = srch2::util::serializeFixedTypes(nodeMasterEligible, buffer);
	buffer = srch2::util::serializeFixedTypes(thisIsMe, buffer);
	return buffer;
}

//given a byte stream recreate the original object
Node * Node::deserializeForNetwork(void* buffer){
	Node * newNode = new Node();
	buffer = srch2::util::deserializeFixedTypes(buffer, newNode->nodeId);
	buffer = srch2::util::deserializeString(buffer, newNode->ipAddress);
	buffer = srch2::util::deserializeFixedTypes(buffer, newNode->portNumber);
	buffer = srch2::util::deserializeString(buffer, newNode->nodeName);
	buffer = srch2::util::deserializeFixedTypes(buffer, newNode->nodeMaster);
	buffer = srch2::util::deserializeFixedTypes(buffer, newNode->nodeMasterEligible);
	buffer = srch2::util::deserializeFixedTypes(buffer, newNode->thisIsMe);
	return newNode;
}

unsigned Node::getNumberOfBytesForNetwork(){
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(NodeId);
	numberOfBytes += srch2::util::getNumberOfBytesString(ipAddress);
	numberOfBytes += sizeof(uint32_t);
	numberOfBytes += srch2::util::getNumberOfBytesString(nodeName);
	numberOfBytes += sizeof(bool);
	numberOfBytes += sizeof(bool);
	numberOfBytes += sizeof(bool); // this is master
	return numberOfBytes;
}

string Node::serialize() {
	stringstream ss;
	ss.write((const char *)&nodeId, sizeof(nodeId));
	unsigned size = nodeName.size();
	ss.write((const char *)&size, sizeof(unsigned));
	ss.write(nodeName.c_str(), nodeName.size());
	size = ipAddress.size();
	ss.write((const char *)&size, sizeof(unsigned));
	ss.write(ipAddress.c_str(), ipAddress.size());
	ss.write((const char *)&portNumber, sizeof(portNumber));
	ss.write((const char *)&thisIsMe, sizeof(thisIsMe));
	ss.write((const char *)&nodeMaster, sizeof(nodeMaster));
	ss.write((const char *)&nodeMasterEligible, sizeof(nodeMasterEligible));
	return ss.str();
}

void Node::deserialize(char *serlializedNode) {

	char *buffer = serlializedNode;
	nodeId = *(NodeId *)buffer;
	buffer += sizeof(NodeId);

	uint32_t size = *(uint32_t *)buffer;
	buffer += sizeof(uint32_t);
	nodeName.assign(buffer, buffer + size);
	buffer += size;

	size = *(uint32_t *)buffer;
	buffer += sizeof(uint32_t);
	ipAddress.assign(buffer, buffer + size);
	buffer += size;

	portNumber = *(uint32_t *)buffer;
	buffer += sizeof(uint32_t);

	thisIsMe = *(bool *)buffer;
	buffer += sizeof(thisIsMe);

	nodeMaster = *(bool *)buffer;
	buffer += sizeof(nodeMaster);

	nodeMasterEligible = *(bool *)buffer;
	buffer += sizeof(nodeMasterEligible);
}

string Node::toStringShort() const{
    stringstream ss;
    ss << "ID(" << nodeId << "),";
    ss << "N(" << nodeName << "),";
    ss << "IP(" << ipAddress << "),";
    ss << "P(" << portNumber << "),";
    if(thisIsMe){
        ss << "ME,";
    }
    if(nodeMaster){
        ss << "MATR,";
    }else{
        ss << "CLNT,";
    }

    if(nodeMasterEligible){
        ss << "MELGBL,";
    }else{
        ss << "MNONELGBL";
    }
    return ss.str();
}


string Node::toString() const{
	stringstream ss;
	ss << "ID:" << nodeId << "%";
	ss << "Name:" << nodeName << "%";
	ss << "IP:" << ipAddress << "%";
	ss << "Port:" << portNumber << "%";
	if(thisIsMe){
		ss << "THIS NODE." << "%";
	}
	if(nodeMaster){
		ss << "MASTER" << "%";
	}else{
		ss << "CLIENT" << "%";
	}

	if(nodeMasterEligible){
		ss << "MASTER-ELIGIBLE" << "%";
	}else{
		ss << "MASTER-NON-ELIGIBLE" << "%";
	}
	return ss.str();
}

}
}
