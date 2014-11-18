
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
	numberOfBytes += sizeof(unsigned);
	numberOfBytes += sizeof(unsigned) + ipAddress.size();
	numberOfBytes += sizeof(unsigned);
	numberOfBytes += sizeof(unsigned) + nodeName.size();
	numberOfBytes += sizeof(bool);
	numberOfBytes += sizeof(bool);
	numberOfBytes += sizeof(unsigned);
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
	nodeId = *(unsigned *)buffer;
	buffer += sizeof(nodeId);

	unsigned size = *(unsigned *)buffer;
	buffer += sizeof(size);
	nodeName.assign(buffer, buffer + size);
	buffer += size;

	size = *(unsigned *)buffer;
	buffer += sizeof(size);
	ipAddress.assign(buffer, buffer + size);
	buffer += size;

	portNumber = *(unsigned *)buffer;
	buffer += sizeof(portNumber);

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
