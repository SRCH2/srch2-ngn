#ifndef __SHARDING_MIGRATION_SHARD_MANAGER_COMMAND_H__
#define __SHARDING_MIGRATION_SHARD_MANAGER_COMMAND_H__

#include "sharding/configuration/Cluster.h"
#include "sharding/transport/MessageAllocator.h"
#include "sharding/transport/Message.h"

#include <queue>



namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

enum ShardManagerCommandCode{
	ShardManager_NewNode_WELCOME, // class to use : ShardManagerCommand
	ShardManager_NewNode_RequestForShards, // class to use : RequestForShards
	ShardManager_NewNode_NodeArrivalOffer, // class to use : NodeArrivalOffer
	ShardManager_NewNode_DONE, // class to use : ShardManagerCommand
	ShardManager_NewNode_PREPARE, // class to use : ClusterMetadataCommand
	ShardManager_NewNode_READY, // class to use : ShardManagerCommand
	ShardManager_NewNode_ABORT, // class to use : ShardManagerCommand
	ShardManager_NewNode_COMMIT, // class to use : ShardManagerCommand
};

enum ShardManagerTransactionType{
	SHM_TRANS_TYPE_NewNode
};

class ShardMigrationDiscriptor{
	// TODO : describes what shards should move from what nodes to what nodes.
};

class RedistributionDiscriptor{
	// TODO : describes what shards should move from what nodes to what nodes.
};

/*
 * The base class of all ShardManager command classes.
 * The transactionId and commandCode are handled in this class.
 * If user wants to put more information in a command he must extend this class and
 * override virtual functions.
 */
class ShardManagerCommand{
public:
	ShardManagerCommand(unsigned transactionId, ShardManagerCommandCode commandCode);
	virtual ~ShardManagerCommand(){};

	unsigned getTransactionId() const;
	ShardManagerCommandCode getCommandCode() const;
	// serialize command into byte stream
    virtual void* serialize(MessageAllocator * allocatorObj);
	// serialize command into byte stream
    // which is allocated and passed by buffer
    // returns the pointer to the end of byte stream
    virtual void* serialize(void * buffer);
    // deserialize command from a byte stream
    virtual static ShardManagerCommand * deserialize(void* buffer);
    virtual static void * deserialize(void * buffer, ShardManagerCommand * command);
    virtual unsigned getNumberOfBytes() const;
    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messageType();

    unsigned getPriority();

    string getCommandCodeString() const;

private:
    ShardManagerCommand(){}; // for deserialization
	unsigned transactionId;
	ShardManagerCommandCode commandCode;
};


/*
 * Command used for requesting for shards, in response of WELCOME
 */
class RequestForShards : public ShardManagerCommand{
public:

	RequestForShards(unsigned transactionId,
			ShardManagerCommandCode commandCode,
			ShardManagerTransactionType transactionType);
	virtual ~RequestForShards(){};
	// serialize command into byte stream
    virtual void* serialize(MessageAllocator * allocatorObj);
	// serialize command into byte stream
    // which is allocated and passed by buffer
    // returns the pointer to the end of byte stream
    virtual void* serialize(void * buffer);
    // deserialize command from a byte stream
    virtual static RequestForShards * deserialize(void* buffer);
    virtual static void * deserialize(void * buffer, RequestForShards * command);
    virtual unsigned getNumberOfBytes() const;

private:
    RequestForShards():ShardManagerCommand(){};
};


/*
 * Command used for giving shards to a new node
 */
class NodeArrivalOffer : public ShardManagerCommand{
public:

	NodeArrivalOffer(unsigned transactionId,
			ShardManagerCommandCode commandCode,
			ShardManagerTransactionType transactionType);
	virtual ~NodeArrivalOffer(){};
	// serialize command into byte stream
    virtual void* serialize(MessageAllocator * allocatorObj);
	// serialize command into byte stream
    // which is allocated and passed by buffer
    // returns the pointer to the end of byte stream
    virtual void* serialize(void * buffer);
    // deserialize command from a byte stream
    virtual static NodeArrivalOffer * deserialize(void* buffer);
    virtual static void * deserialize(void * buffer, NodeArrivalOffer * command);
    virtual unsigned getNumberOfBytes() const;

private:
    NodeArrivalOffer():ShardManagerCommand(){};
};


/*
 * Command used for transferring the new state of cluster writeview
 */
class ClusterMetadataCommand : public ShardManagerCommand{
public:

	ClusterMetadataCommand(unsigned transactionId,
			ShardManagerCommandCode commandCode,
			ShardManagerTransactionType transactionType);
	virtual ~ClusterMetadataCommand(){};
	// serialize command into byte stream
    virtual void* serialize(MessageAllocator * allocatorObj);
	// serialize command into byte stream
    // which is allocated and passed by buffer
    // returns the pointer to the end of byte stream
    virtual void* serialize(void * buffer);
    // deserialize command from a byte stream
    virtual static ClusterMetadataCommand * deserialize(void* buffer);
    virtual static void * deserialize(void * buffer, ClusterMetadataCommand * command);
    virtual unsigned getNumberOfBytes() const;

private:
    ClusterMetadataCommand():ShardManagerCommand(){};
};


}
}


#endif // __SHARDING_MIGRATION_SHARD_MANAGER_COMMAND_H__
