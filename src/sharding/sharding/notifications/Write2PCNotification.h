#ifndef __SHARDING_SHARDING_WRITE_2PC_NOTIFICATION_H__
#define __SHARDING_SHARDING_WRITE_2PC_NOTIFICATION_H__

#include "Notification.h"
#include "../metadata_manager/Shard.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/Message.h"
#include "include/instantsearch/Record.h"
#include "../metadata_manager/Cluster.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

enum WriteNotificationMode{
	WriteNotificationModeAsk2PC,
	WriteNotificationModePerformWrite
};

/*
 * Contains the state of write operation per record
 */
struct RecordWriteOpHandle{
	RecordWriteOpHandle(Record * record);
	RecordWriteOpHandle(const string & primaryKey);
	RecordWriteOpHandle(const string & primaryKey, const vector<string> & roleIds);
	RecordWriteOpHandle();
	string getPrimaryKey() const;
	Record * getRecordObj();
	vector<string> & getRoleIds();
	void finalize(bool success);
	bool isSuccessful() const;
	bool isWorkDone() const;
	void addMessage(ShardId * shardId, const JsonMessageCode messageCode);
	map<ShardId * ,vector<JsonMessageCode>, ShardPtrComparator > getMessageCodes();
	void * serialize(void * buffer, const WriteNotificationMode mode) const;
	void * deserialize(void * buffer, const WriteNotificationMode mode, const Schema *schema);
	unsigned getNumberOfBytes(const WriteNotificationMode mode) const;


private:
	bool success;
	bool workDone;
	Record * record;
	string primaryKey;
	// in case it's a record acl operation, primaryKey holds the pk and this
	// vector holds the roleids
	vector<string> roleIds;
	bool recordAclCommandFlag;
	// null pointer means general messages
	map<ShardId * ,vector<JsonMessageCode>, ShardPtrComparator >messageCodes;
};
struct RecordComparator
{
    inline bool operator() (const RecordWriteOpHandle * lhs, const RecordWriteOpHandle * rhs)
    {
    	if(rhs == NULL || lhs == NULL){
    		ASSERT(false);
    		return false;
    	}
        return (rhs->getPrimaryKey().compare(rhs->getPrimaryKey()) < 0);
    }
};

class Write2PCNotification : public ShardingNotification {
public:
	ShardingMessageType messageType() const;
	Write2PCNotification(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			ClusterRecordOperation_Type commandType,
			NodeTargetShardInfo targets,
			WriteNotificationMode mode,
			vector<RecordWriteOpHandle *> recordHandles);
	Write2PCNotification();


	bool resolveNotification(SP(ShardingNotification) _notif);

	void * serializeBody(void * buffer) const;
    unsigned getNumberOfBytesBody() const;
    void * deserializeBody(void * buffer);

    vector<RecordWriteOpHandle *> & getRecordHandles();

    ClusterRecordOperation_Type getCommandType() const;

    NodeTargetShardInfo getTargets() const;

    bool shouldPerformWrite() const{
    	return (mode == WriteNotificationModePerformWrite);
    }

    bool hasResponse() const {
    		return true;
	}

    boost::shared_ptr<const ClusterResourceMetadata_Readview> getClusterReadview();

private:
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	ClusterRecordOperation_Type commandType;
	NodeTargetShardInfo targets;
	WriteNotificationMode mode;
	vector<RecordWriteOpHandle *> recordHandles;


public:
	class ACK : public ShardingNotification {
	public:

		struct ShardResult{
			ShardResult(ShardId * shardId, bool statusValue, const vector<JsonMessageCode> & messageCodes);
			~ShardResult();
			ShardResult();
		    ShardId * shardId;
			bool statusValue;
			vector<JsonMessageCode> messageCodes;
			void * serialize(void * buffer) const;
		    unsigned getNumberOfBytes() const;
		    void * deserialize(void * buffer);
		};

		ShardingMessageType messageType() const;
		ACK(){};

		~ACK();

		bool resolveNotification(SP(ShardingNotification) _notif);

		void * serializeBody(void * buffer) const;
	    unsigned getNumberOfBytesBody() const;
	    void * deserializeBody(void * buffer);

	    void addPrimaryKeyShardResult(const string & primaryKey, ShardResult * result);

	    map<string, vector<ShardResult *> > & getResults();

	private:
	    map<string, vector<ShardResult *> > primaryKeyShardResults;
	};
};

}
}

#endif // __SHARDING_SHARDING_WRITE_2PC_NOTIFICATION_H__

