#ifndef __SHARDING_CONFIGURATION_NODE_H__
#define __SHARDING_CONFIGURATION_NODE_H__

#include <instantsearch/Constants.h>
#include "src/wrapper/WrapperConstants.h"
#include "ShardingConstants.h"
#include "src/core/util/Assert.h"
#include "src/core/util/Logger.h"

#include <vector>
#include <string>

using namespace std;
using namespace srch2::util;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

class Node {
public:

	Node();

	Node(const std::string& nodeName, const std::string& ipAddress,
			unsigned portNumber, bool thisIsMe, unsigned numberOfPShards = 1);
	Node(std::string& nodeName, std::string& ipAddress, unsigned portNumber,
			bool thisIsMe, bool nodeMaster, bool nodeData,std::string& dataDir, std::string& homeDir, unsigned numberOfPShards);

	Node(const Node & node){
		this->nodeId = node.nodeId;
		this->ipAddress = node.ipAddress;
		this->portNumber = node.portNumber;
		this->nodeName = node.nodeName;
		this->ports = node.ports;
		this->nodeMaster = node.nodeMaster;
		this->nodeData = node.nodeData;
		this->homeDir = node.homeDir;
		this->dataDir = node.dataDir;
		this->numberOfThreads = node.numberOfThreads;
		this->thisIsMe = node.thisIsMe;
		this->numberOfPrimaryShards = node.numberOfPrimaryShards;
	}

	std::string getHomeDir() const;
	std::string getDataDir() const;
	bool isMaster() const;
	bool isData() const;
	std::string getName() const;
	std::string getIpAddress() const;
	unsigned int getId() const;
	void setId(unsigned nodeId);
	unsigned int getPortNumber() const;

	unsigned getDefaultNumberOfPrimaryShards() const;


	bool thisIsMe; // temporary for V0

	unsigned short getPort(PortType_t portType) const;
	void setPort(PortType_t portType, unsigned short portNumber);

	// TODO (for Surendra): refine this iterator
	// const Node& operator = (const Node& node);

	// an iterator to go through the shards in this node
	//class ShardIterator {
	//public:
	//unsigned first; // TODO: Ask Surendra
	//Shard second;
	//bool operator == (NodeIterator* rhs);
	//};

	//typedef NodeIterator * Iterator;
	//Iterator begin();
	//Iterator next();
	//Iterator end();

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serializeForNetwork(void * buffer);

    //given a byte stream recreate the original object
    static Node * deserializeForNetwork(void* buffer);

    unsigned getNumberOfBytesForNetwork();

	string serialize();

	void deserialize(char *serlializedNode) ;
private:
	unsigned nodeId;
	std::string ipAddress;
	unsigned portNumber;
	std::string nodeName;
	vector<unsigned short> ports;

	// Allow this node to be eligible as a master node (enabled by default).
	bool nodeMaster;

	// Allow this node to store data (enabled by default). If enabled, the node is eligible to store data shards.
	bool nodeData;

	// Home directory for all the index files of shards on this node.
	string homeDir;

	string dataDir;
	unsigned int numberOfThreads;
	// other node-related info

	// temporary for phase 1 of V1
	unsigned numberOfPrimaryShards;
};

}
}

#endif // __SHARDING_CONFIGURATION_NODE_H__
