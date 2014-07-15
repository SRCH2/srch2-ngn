
#include "ShardManagerCommand.h"

#include <map>
#include "core/util/Assert.h"
#include "core/util/Logger.h"
#include "core/util/SerializationHelper.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


unsigned ShardManagerCommandMetadata::getNumberOfBytes(){
	return sizeof(TransactionType);
}

void * ShardManagerCommandMetadata::serialize(void * buffer){
	return srch2::util::serializeFixedTypes(transactionType, buffer);
}
void * ShardManagerCommandMetadata::deserialize(void * buffer){
    return srch2::util::deserializeFixedTypes(buffer, transactionType);
}


SHMRequestReport::SHMRequestReport(unsigned transactionId, CommandCode code, Cluster * clusterWriteview){
	this->transactionId = transactionId;
	this->code = code;
	this->clusterWriteview = clusterWriteview;
	this->metadata = NULL;
}
SHMRequestReport::SHMRequestReport(unsigned transactionId, CommandCode code){
	this->transactionId = transactionId;
	this->code = code;
	this->clusterWriteview = NULL;
	this->metadata = NULL;
}

SHMRequestReport::SHMRequestReport(unsigned transactionId, CommandCode code, ShardManagerCommandMetadata * metadata ){
	this->transactionId = transactionId;
	this->code = code;
	this->metadata = metadata;
	this->clusterWriteview = NULL;
}

// private constructor
SHMRequestReport::SHMRequestReport(const SHMRequestReport & command){
	this->code = command.code;
	this->transactionId = command.transactionId;
	this->clusterWriteview = command.clusterWriteview;
	this->metadata = command.metadata;
};

// serialize command into byte stream
void* SHMRequestReport::serialize(MessageAllocator * allocatorObj){
	// calculate the size of byte array after serialization
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned);
	numberOfBytes += sizeof(CommandCode);
	numberOfBytes += sizeof(bool);// whether cluster writeview is available or not
	if(clusterWriteview != NULL){
		numberOfBytes += clusterWriteview->getNumberOfBytesForNetwork();
	}
	numberOfBytes += sizeof(bool);// whether cluster writeview is available or not
	if(metadata != NULL){
		numberOfBytes += metadata->getNumberOfBytes();
	}

    // allocate the space
    void * buffer = allocatorObj->allocateMessageReturnBody(numberOfBytes);
    void * bufferWritePointer = buffer;

    // serialize
    bufferWritePointer = srch2::util::serializeFixedTypes(transactionId, bufferWritePointer);
    bufferWritePointer = srch2::util::serializeFixedTypes(code, bufferWritePointer);
    bufferWritePointer = srch2::util::serializeFixedTypes((bool)(clusterWriteview != NULL), bufferWritePointer);
    if(clusterWriteview != NULL){
    	bufferWritePointer = clusterWriteview->serialize(bufferWritePointer);
    }
    bufferWritePointer = srch2::util::serializeFixedTypes((bool)(metadata != NULL), bufferWritePointer);
    if(metadata != NULL){
    	bufferWritePointer = metadata->serialize(bufferWritePointer);
    }

    // return buffer
    return buffer;

}


// deserialize command from a byte stream
SHMRequestReport * SHMRequestReport::deserialize(void* buffer){

	if(buffer == NULL){
		ASSERT(false);
		return NULL;
	}

	SHMRequestReport * shmReqRep = new SHMRequestReport();
	// deserialize command code
    buffer = srch2::util::deserializeFixedTypes(buffer, shmReqRep->transactionId);
    buffer = srch2::util::deserializeFixedTypes(buffer, shmReqRep->code);
    // deserialize the boolean flag
    bool isClusterAvailable = false;
    buffer = srch2::util::deserializeFixedTypes(buffer, isClusterAvailable);
    if(isClusterAvailable){
    	Cluster * newCluster = Cluster::deserialize(buffer);
    	buffer = (char *)buffer + newCluster->getNumberOfBytesForNetwork();
    }
    bool isMetadataAvailable = false;
    buffer = srch2::util::deserializeFixedTypes(buffer, isMetadataAvailable);
    if(isMetadataAvailable){
    	shmReqRep->metadata = new ShardManagerCommandMetadata();
    	buffer = shmReqRep->metadata->deserialize(buffer);
    }

    return shmReqRep;
}

//Returns the type of message which uses this kind of object as transport
ShardingMessageType SHMRequestReport::messageType(){
    return ShardManagerRequestReportMessageType;
    // ShardManagerClusterInfoMessageType
}

SHMRequestReport::CommandCode SHMRequestReport::getCommandCode(){
	return this->code;
}

Cluster * SHMRequestReport::getClusterWriteview(){
	return this->clusterWriteview;
}

void SHMRequestReport::setClusterWriteview(){
	this->clusterWriteview = clusterWriteview;
}

unsigned SHMRequestReport::getTransactionId(){
	return this->transactionId;
}

unsigned SHMRequestReport::getPriority(){
	switch (code) {
	case SHM_TRANS_START:
	case SHM_TRANS_START_CONFIRM:
	case SHM_TRANS_START_ERROR_ONGOING:
	case SHM_TRANS_START_ERROR_ABORTED:
	case SHM_TRANS_START_ERROR_SUCCEED:
	case SHM_TRANS_COMMIT:
	case SHM_TRANS_COMMIT_FAILED:
	case SHM_TRANS_COMMIT_CONFIRM:
	case SHM_TRANS_COMMIT_COMPLETE:
	case SHM_BUSY:
	case SHM_BOOTSTRAP_STATUS:
	case SHM_BOOTSTRAP_DONE:
			return 1; // currently all commands have the same priarity
		default:
			ASSERT(false);
			return 0;
	}
}

ShardManagerCommandMetadata * SHMRequestReport::getMetadata(){
	return metadata;
}

string SHMRequestReport::getCommandCodeString() const{
	switch (code) {
	case SHM_TRANS_START:
		return "SHM_TRANS_START";
	case SHM_TRANS_START_CONFIRM:
		return "SHM_TRANS_START_CONFIRM";
	case SHM_TRANS_START_ERROR_ONGOING:
		return "SHM_TRANS_START_ERROR_ONGOING";
	case SHM_TRANS_START_ERROR_ABORTED:
		return "SHM_TRANS_START_ERROR_ABORTED";
	case SHM_TRANS_START_ERROR_SUCCEED:
		return "SHM_TRANS_START_ERROR_SUCCEED";
	case SHM_TRANS_COMMIT:
		return "SHM_TRANS_COMMIT";
	case SHM_TRANS_COMMIT_FAILED:
		return "SHM_TRANS_COMMIT_FAILED";
	case SHM_TRANS_COMMIT_CONFIRM:
		return "SHM_TRANS_COMMIT_CONFIRM";
	case SHM_TRANS_COMMIT_COMPLETE:
		return "SHM_TRANS_COMMIT_COMPLETE";
	case SHM_BUSY:
		return "SHM_BUSY";
	case SHM_BOOTSTRAP_STATUS:
		return "SHM_BOOTSTRAP_STATUS";
	case SHM_BOOTSTRAP_DONE:
		return "SHM_BOOTSTRAP_DONE";
	default:
		ASSERT(false);
		return "UNKNOWN";
	}
}

/////////////////////////////// Command Buffer Implementation /////////////////////////////////////

// adds the command to the buffer
void CommandBuffer::saveCommand(NodeId nodeId, SHMRequestReport * command,unsigned messageId){
	ASSERT(command != NULL);
	// lock the buffer
	commandBufferLock.lock();

	// check if the map has any mailbox for this node
	std::map<NodeId, std::queue<CommandHandle> >::iterator nodeEntry = commands.find(nodeId);
	if(nodeEntry == commands.end()){
		std::pair<std::map<NodeId, std::queue<CommandHandle> >::iterator, bool> insertResult =
				commands.insert(std::make_pair(nodeId, std::queue<CommandHandle>()));
		if(! insertResult.second){
			ASSERT(false);
			// unlock the buffer
			commandBufferLock.unlock();
			return;
		}
		nodeEntry = insertResult.first;
	}

	srch2::util::Logger::console("Command %s received from node %d and saved in buffer (msg id : %d .)"
			,command->getCommandCodeString().c_str(),nodeId, messageId);
	nodeEntry->second.push(CommandHandle(command, messageId));
	// unlock the buffer
	commandBufferLock.unlock();
}

// pop the oldest command of this node and return true
// returns false if there is no command in the mailbox of this nodeId
bool CommandBuffer::getNextNodeCommand(NodeId nodeId, SHMRequestReport *& command, unsigned & messageId){
	// lock the buffer
	commandBufferLock.lock();
	// check if the map has any mailbox for this node
	std::map<NodeId, std::queue<CommandHandle> >::iterator nodeEntry = commands.find(nodeId);
	if(nodeEntry == commands.end()){
		// unlock the buffer
		commandBufferLock.unlock();
		return false;
	}

	if(nodeEntry->second.size() == 0){
		// unlock the buffer
		commandBufferLock.unlock();
		return false; // no unread command from this nodeId
	}

	command = nodeEntry->second.front().command;
	messageId = nodeEntry->second.front().messageId;

	srch2::util::Logger::console("Command %s from node %d is fetched from buffer (msg id : %d .)"
			,command->getCommandCodeString().c_str(),nodeId, messageId);
	nodeEntry->second.pop();
	// unlock the buffer
	commandBufferLock.unlock();

	return true;
}


// pop the oldest command of any node that has a command in it's mailbox along with its NodeId
bool CommandBuffer::getNextCommand(NodeId & nodeId, SHMRequestReport *& command, unsigned & messageId){
	// lock the buffer
	commandBufferLock.lock();
	// iterate on all entries in the map and return the first one which has something
	if(commands.size() == 0){
		// unlock the buffer
		commandBufferLock.unlock();
		return false;
	}
	for(std::map<NodeId, std::queue<CommandHandle> >::iterator commandItr = commands.begin();
			commandItr != commands.end(); ++commandItr){
		if(commandItr->second.size() != 0){
			// return the first element in queue (which is the oldest one)
			nodeId = commandItr->first;
			command = commandItr->second.front().command;
			messageId = commandItr->second.front().messageId;
			srch2::util::Logger::console("Command %s from node %d is fetched from buffer (msg id : %d .)"
					,command->getCommandCodeString().c_str(),nodeId, messageId);
			commandItr->second.pop();
			// unlock the buffer
			commandBufferLock.unlock();
			return true;
		}
	}
	// unlock the buffer
	commandBufferLock.unlock();
	return false;
}


}
}
