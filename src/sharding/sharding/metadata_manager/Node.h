#ifndef __SHARDING_CONFIGURATION_NODE_H__
#define __SHARDING_CONFIGURATION_NODE_H__

#include <instantsearch/Constants.h>
#include "src/wrapper/WrapperConstants.h"
#include "../../configuration/ShardingConstants.h"
#include "src/core/util/Assert.h"
#include "src/core/util/Logger.h"

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
	string toString() const;

private:
	unsigned nodeId;
	std::string ipAddress;
	unsigned portNumber;
	std::string nodeName;
	// flag to tell whether current node is master or not.
	bool nodeMaster;
	// Allow this node to be eligible as a master node (enabled by default).
	bool nodeMasterEligible;
};

}
}

#endif // __SHARDING_CONFIGURATION_NODE_H__
