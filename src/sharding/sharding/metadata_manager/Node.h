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
#ifndef __SHARDING_CONFIGURATION_NODE_H__
#define __SHARDING_CONFIGURATION_NODE_H__

#include "../../configuration/ShardingConstants.h"
#include "src/core/util/Assert.h"
#include "src/core/util/Logger.h"
#include "src/wrapper/WrapperConstants.h"
#include <instantsearch/Constants.h>

#include <vector>
#include <string>

using namespace std;
using namespace srch2::util;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

// static Node information read from config file.
class NodeConfig {
public:
	NodeConfig() {
		isNodeMasterEligible = true;
	}
	unsigned short getPort(PortType_t portType) const;
	void setPort(PortType_t portType, unsigned short portNumber);
	void setMasterEligible(bool masterEligible) { isNodeMasterEligible = masterEligible; }
	bool isMasterEligible() { return isNodeMasterEligible; }
	void setName(string nodeName) { this->nodeName = nodeName; }
	string getName() { return nodeName; }
private:
	vector<unsigned short> ports;
	string nodeName;
	bool isNodeMasterEligible;
};

// Dynamic Node metadata
class Node {
public:

	Node();

	Node(const std::string& nodeName, const std::string& ipAddress,
			unsigned portNumber, bool thisIsMe, unsigned numberOfPShards = 1, bool nodeMasterEligible = true);

	Node(const Node & node){
		this->nodeId = node.nodeId;
		this->ipAddress = node.ipAddress;
		this->portNumber = node.portNumber;
		this->nodeName = node.nodeName;
		this->nodeMaster = node.nodeMaster;
		this->nodeMasterEligible = node.nodeMasterEligible;
		this->thisIsMe = node.thisIsMe;
	}

	Node & operator=(const Node & node){
		if(this == &node){
			return *this;
		}
		this->nodeId = node.nodeId;
		this->ipAddress = node.ipAddress;
		this->portNumber = node.portNumber;
		this->nodeName = node.nodeName;
		this->nodeMaster = node.nodeMaster;
		this->nodeMasterEligible = node.nodeMasterEligible;
		this->thisIsMe = node.thisIsMe;
		return *this;
	}
	bool isMasterEligible() const;
	bool isMaster() const;
	std::string getName() const;
	std::string getIpAddress() const;
	unsigned int getId() const;
	void setId(unsigned nodeId);
	unsigned int getPortNumber() const;

	void setMaster(bool isMaster);
	bool thisIsMe; // temporary for V0

	unsigned short getPort(PortType_t portType) const;
	void setPort(PortType_t portType, unsigned short portNumber);

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serializeForNetwork(void * buffer);

    //given a byte stream recreate the original object
    static Node * deserializeForNetwork(void* buffer);

    unsigned getNumberOfBytesForNetwork();

    string serialize();
	void deserialize(char *serlializedNode) ;
	string toStringShort() const;
	string toString() const;



private:
	NodeId nodeId;
	std::string ipAddress;
	uint32_t portNumber;
	std::string nodeName;
	// flag to tell whether current node is master or not.
	bool nodeMaster;
	// Allow this node to be eligible as a master node (enabled by default).
	bool nodeMasterEligible;
};

}
}

#endif // __SHARDING_CONFIGURATION_NODE_H__
