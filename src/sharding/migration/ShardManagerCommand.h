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

class ShardManagerCommandMetadata{
public:
	enum TransactionType{
		// TODO : for future, when we start a transaction, master must tell
		//                    clients what type of transaction it is ....
	};
	TransactionType transactionType;

	unsigned getNumberOfBytes();
	void * serialize(void * buffer);
	void * deserialize(void * buffer);

};

class SHMRequestReport {
public:

	enum CommandCode{
		SHM_TRANS_START, // always from master, with the new TID
		SHM_TRANS_START_CONFIRM, // Always from clients, contains no data.
		SHM_TRANS_START_ERROR_ONGOING, // also contains the old transaction id in transactionId of command object
		SHM_TRANS_START_ERROR_ABORTED, // also contains the old transaction id in transactionId of command object
		SHM_TRANS_START_ERROR_SUCCEED, // also contains the old transaction id in transactionId of command object

		SHM_TRANS_COMMIT,
		SHM_TRANS_COMMIT_FAILED, // also contains an image of the source node's cluster writeview so that master can find the problem
		SHM_TRANS_COMMIT_CONFIRM,
		SHM_TRANS_COMMIT_COMPLETE,

		SHM_BUSY,
		SHM_BOOTSTRAP_STATUS,   // no cluster metadata with this code
		SHM_BOOTSTRAP_DONE,     // partial cluster metadata with this code
	};

	SHMRequestReport(unsigned transactionId, CommandCode code, Cluster * clusterWriteview);
	SHMRequestReport(unsigned transactionId, CommandCode code);
	SHMRequestReport(unsigned transactionId, CommandCode code, ShardManagerCommandMetadata * metadata );
	// serialize command into byte stream
    void* serialize(MessageAllocator * allocatorObj);


    // deserialize command from a byte stream
    static SHMRequestReport * deserialize(void* buffer);

    //Returns the type of message which uses this kind of object as transport
    static ShardingMessageType messageType();

    CommandCode getCommandCode();

    Cluster * getClusterWriteview();

    void setClusterWriteview();

    unsigned getTransactionId();

    unsigned getPriority();

    ShardManagerCommandMetadata * getMetadata();
    string getCommandCodeString() const;

private:

    SHMRequestReport(){}; // only used for deserialization
    SHMRequestReport(const SHMRequestReport & command);

    CommandCode code;
    unsigned transactionId;
    // should not be deleted in this class,
    // it's always allocated/deleted in ShardManager
    Cluster * clusterWriteview;

    ShardManagerCommandMetadata * metadata;

};


// Command buffer
// This buffer stores the command messages coming to this node
// so that internal threads can be freed as soon as possible
struct CommandHandle{
	CommandHandle(SHMRequestReport * command,unsigned messageId){
		this->command = command;
		this->messageId = messageId;
	}
	SHMRequestReport * command;
	unsigned messageId;
};
class CommandBuffer{
public:
	// adds the command to the buffer
	void saveCommand(NodeId nodeId, SHMRequestReport * command,unsigned messageId);

	// pop the oldest command of this node and return true
	// returns false if there is no command in the mailbox of this nodeId
	bool getNextNodeCommand(NodeId nodeId, SHMRequestReport *& command, unsigned & messageId);

	// pop the oldest command of any node that has a command in it's mailbox along with its NodeId
	bool getNextCommand(NodeId & nodeId, SHMRequestReport *& command, unsigned & messageId);

private:
	boost::mutex commandBufferLock;
	std::map<NodeId, std::queue<CommandHandle> > commands;
};


}
}


#endif // __SHARDING_MIGRATION_SHARD_MANAGER_COMMAND_H__
