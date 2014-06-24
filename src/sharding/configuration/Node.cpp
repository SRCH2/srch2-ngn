
#include "Node.h"

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
	this->nodeMaster = true;
	this->nodeData = true;
	this->dataDir = "";
	this->homeDir = "";
	this->numberOfThreads = 1;
	this->thisIsMe = false;
	//coreToShardsMap has to be initialized

	this->numberOfPrimaryShards = 1;

}

Node::Node(const std::string& nodeName, const std::string& ipAddress,
		unsigned portNumber, bool thisIsMe, unsigned numberOfPShards){
	this->nodeId = 0;
	this->nodeName = nodeName;
	this->ipAddress = ipAddress;
	this->portNumber = portNumber;
	this->thisIsMe = thisIsMe;
	this->nodeMaster = true;
	this->nodeData = true;
	this->dataDir = "";
	this->homeDir = "";
	this->numberOfThreads = 1;
	this->numberOfPrimaryShards = 0; // this constructor is used for other nodes
}

Node::Node(std::string& nodeName, std::string& ipAddress,
		unsigned portNumber,bool thisIsMe, bool nodeMaster,
		bool nodeData,std::string& dataDir, std::string& homeDir, unsigned numberOfPShards) {
	this->nodeId = 0;
	this->nodeName = nodeName;
	this->ipAddress = ipAddress;
	this->portNumber = portNumber;
	this->thisIsMe = thisIsMe;
	this->nodeMaster = nodeMaster;
	this->nodeData = nodeData;
	this->dataDir = dataDir;
	this->homeDir = homeDir;
	this->numberOfThreads = 1; // default value is 1
	this->numberOfPrimaryShards = numberOfPShards;
}

std::string Node::getHomeDir() const {
	return this->homeDir;
}
std::string Node::getDataDir() const {
	return this->dataDir;
}

bool Node::isMaster() const {
	return nodeMaster;
}

bool Node::isData() const {
	return nodeData;
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

unsigned Node::getDefaultNumberOfPrimaryShards() const{
	return this->numberOfPrimaryShards;
}

unsigned short Node::getPort(PortType_t portType) const
{
      if (static_cast<unsigned int> (portType) >= ports.size()) {
          return 0;
      }

      unsigned short portNumber = ports[portType];
      return portNumber;
}

void Node::setPort(PortType_t portType, unsigned short portNumber)
{
      if (static_cast<unsigned int> (portType) >= ports.size()) {
          ports.resize(static_cast<unsigned int> (EndOfPortType), 0);
      }

      switch (portType) {
      case SearchPort:
      case SuggestPort:
      case InfoPort:
      case DocsPort:
      case UpdatePort:
      case SavePort:
      case ExportPort:
      case ResetLoggerPort:
          ports[portType] = portNumber;
          break;

      default:
          Logger::error("Unrecognized HTTP listening port type: %d", static_cast<int> (portType));
          break;
      }
}

string Node::serialize() { // TODO : should be reviewed for merge
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
	ss.write((const char *)&nodeData, sizeof(nodeData));
	ss.write((const char *)&nodeMaster, sizeof(nodeMaster));
	return ss.str();
}

void Node::deserialize(char *serlializedNode) { // TODO : should be reviewd for merge

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

	nodeData = *(bool *)buffer;
	buffer += sizeof(nodeData);

	nodeMaster = *(bool *)buffer;
	buffer += sizeof(nodeMaster);
}

}
}
