
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




ShardManagerCommand::ShardManagerCommand(unsigned transactionId, ShardManagerCommandCode commandCode){
	this->transactionId = transactionId;
	this->commandCode = commandCode;
}

unsigned ShardManagerCommand::getTransactionId() const{
	return this->transactionId;
}
ShardManagerCommandCode ShardManagerCommand::getCommandCode() const{
	return this->commandCode;
}

// serialize command into byte stream
virtual void* ShardManagerCommand::serialize(MessageAllocator * allocatorObj){
	unsigned numberOfBytes = getNumberOfBytes();
	void * buffer = allocatorObj->allocateMessageReturnBody(numberOfBytes);
    serialize(buffer);
    return buffer;
}
// serialize command into byte stream
// which is allocated and passed by buffer
// returns the pointer to the end of byte stream
virtual void* ShardManagerCommand::serialize(void * buffer){
	buffer = srch2::util::serializeFixedTypes(transactionId, buffer);
	buffer = srch2::util::serializeFixedTypes(commandCode, buffer);
	return buffer;
}


// deserialize command from a byte stream
virtual static ShardManagerCommand * ShardManagerCommand::deserialize(void* buffer){
	ShardManagerCommand * newCommand = new ShardManagerCommand();
	deserialize(buffer, newCommand);
	return newCommand;
}

virtual static void * ShardManagerCommand::deserialize(void * buffer, ShardManagerCommand * command){
	ASSERT(command != NULL);
	buffer = srch2::util::deserializeFixedTypes(buffer, command->transactionId);
	buffer = srch2::util::deserializeFixedTypes(buffer, command->commandCode);
	return buffer;
}

virtual unsigned ShardManagerCommand::getNumberOfBytes() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned);
	numberOfBytes += sizeof(ShardManagerCommandCode);
	return numberOfBytes;
}

//Returns the type of message which uses this kind of object as transport
static ShardingMessageType ShardManagerCommand::messageType(){
	return ShardManagerRequestReportMessageType;
}


unsigned ShardManagerCommand::getPriority(){
	switch (commandCode) {
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

string ShardManagerCommand::getCommandCodeString() const{
	switch (commandCode) {
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

RequestForShards::RequestForShards(unsigned transactionId,
		ShardManagerCommandCode commandCode,
		ShardManagerTransactionType transactionType): ShardManagerCommand(transactionId, commandCode){
}

// serialize command into byte stream
virtual void* RequestForShards::serialize(MessageAllocator * allocatorObj){
	//TODO
}
// serialize command into byte stream
// which is allocated and passed by buffer
// returns the pointer to the end of byte stream
virtual void* RequestForShards::serialize(void * buffer){
	//TODO
}


// deserialize command from a byte stream
virtual static RequestForShards * RequestForShards::deserialize(void* buffer){
	//TODO
}

virtual static void * RequestForShards::deserialize(void * buffer, RequestForShards * command){
	//TODO
}

virtual unsigned RequestForShards::getNumberOfBytes() const{
	//TODO
}


// serialize command into byte stream
virtual void* NodeArrivalOffer::serialize(MessageAllocator * allocatorObj){
	//TODO
}
// serialize command into byte stream
// which is allocated and passed by buffer
// returns the pointer to the end of byte stream
virtual void* NodeArrivalOffer::serialize(void * buffer){
	//TODO
}
// deserialize command from a byte stream
virtual static NodeArrivalOffer * NodeArrivalOffer::deserialize(void* buffer){
	//TODO
}
virtual static void * NodeArrivalOffer::deserialize(void * buffer, NodeArrivalOffer * command){
	//TODO
}
virtual unsigned NodeArrivalOffer::getNumberOfBytes() const{
	//TODO
}

// serialize command into byte stream
virtual void* ClusterMetadataCommand::serialize(MessageAllocator * allocatorObj){
    //TODO
}
// serialize command into byte stream
// which is allocated and passed by buffer
// returns the pointer to the end of byte stream
virtual void* ClusterMetadataCommand::serialize(void * buffer){
	//TODO
}
// deserialize command from a byte stream
virtual static ClusterMetadataCommand * ClusterMetadataCommand::deserialize(void* buffer){
	//TODO
}
virtual static void * ClusterMetadataCommand::deserialize(void * buffer, ClusterMetadataCommand * command){
	//TODO
}
virtual unsigned ClusterMetadataCommand::getNumberOfBytes() const{
	//TODO
}

}
}
