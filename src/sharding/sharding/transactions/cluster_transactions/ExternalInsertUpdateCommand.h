#ifndef __SHARDING_SHARDING_EXT_INSERT_UPDATE_COMMAND_OPERATION_H__
#define __SHARDING_SHARDING_EXT_INSERT_UPDATE_COMMAND_OPERATION_H__

#include "../../state_machine/State.h"
#include "../../state_machine/operations/ConcurrentNotifOperation.h"
#include "../../state_machine/notifications/Notification.h"
#include "../../state_machine/notifications/CommandStatusNotification.h"
#include "../../state_machine/notifications/InsertUpdateNotification.h"
#include "../../metadata_manager/Shard.h"

#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

// 	ExternalInsertUpdateCommand::insert(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
//	evhttp_request *req, unsigned coreId);
class ExternalInsertUpdateCommand : public AggregatorCallbackInterface{
public:
	ExternalInsertUpdateCommand(CommandStatusAggregationCallbackInterface * consumer,
			vector<srch2is::Record *> records, InsertUpdateNotification::OperationCode insertOrUpdate, unsigned coreId){
    	ASSERT(consumer != NULL);
    	this->consumer = consumer;
		this->records = records;
		this->insertOrUpdate = insertOrUpdate;
		this->coreId = coreId;
	}
//
//	ExternalInsertUpdateCommand(CommandStatusAggregationCallbackInterface * consumer,
//			const string & deletePrimaryKey, unsigned coreId){
//    	ASSERT(consumer != NULL);
//    	this->consumer = consumer;
//		this->deletePrimaryKey = deletePrimaryKey;
//		this->coreId = coreId;
//		this->commandCode = RecordWriteCommand_Delete;
//	}


//    InsertUpdateCommand(CommandStatusAggregationCallbackInterface * consumer,
//    		const vector<string>&  fieldTokens,
//    		const vector<string>& roleValueTokens, AclActionType action){
//    	ASSERT(consumer != NULL);
//    	this->consumer = consumer;
//    	this->fieldTokens = fieldTokens;
//    	this->roleValueTokens = roleValueTokens;
//    	this->aclAttrAction = action;
//    }


	void start(){

	}


	void setMessageChannel(boost::shared_ptr<HTTPJsonRecordOperationResponse > brokerSideInformationJson){
		this->brokerSideInformationJson = brokerSideInformationJson;
	}
private:
    CommandStatusAggregationCallbackInterface * consumer;
	unsigned coreId;


	// INSERT/UPDATE
	vector<srch2is::Record *> records;
	InsertUpdateNotification::OperationCode insertOrUpdate;
	vector<WriteCommandNotification *> insertUpdateNotifications;
//	// DELETE
//	string deletePrimaryKey;
//	WriteCommandNotification * deleteNotification;
//	// ACL ATTR
//	vector<string>  fieldTokens;
//	vector<string> roleValueTokens;
//	AclActionType aclAttrAction;
//	WriteCommandNotification * aclAttrNotification;


	boost::shared_ptr<HTTPJsonRecordOperationResponse > brokerSideInformationJson ;
//			boost::shared_ptr<HTTPJsonRecordOperationResponse > (new HTTPJsonRecordOperationResponse(req));

	void initInsertUpdateNotifications(){
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
		ShardManager::getReadview(clusterReadview);

		map<unsigned, vector<Record *> > partitionRecords;
		map<unsigned, vector<NodeTargetShardInfo> > partitionTargets;

		CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(coreId));
		for(unsigned i = 0 ; i < records.size(); i++){
			unsigned hashKey = partitioner->hashDJB2(records.at(i)->getPrimaryKey().c_str());
			vector<NodeTargetShardInfo> targets;
			partitioner->getAllWriteTargets(hashKey,
					clusterReadview->getCurrentNodeId(), targets);
			if(partitionRecords.find(hashKey) == partitionRecords.end()){
				partitionRecords[hashKey] = vector<Record * >();
				partitionTargets[hashKey] = targets;
			}
			partitionRecords[hashKey].push_back(records.at(i));
		}

		for(map<unsigned, vector<Record *> >::iterator partItr = partitionRecords.begin();
				partItr != partitionRecords.end(); ++partItr){
			for(unsigned i = 0 ; i < partitionTargets.find(partItr->first)->second.size() ; ++i){
				insertUpdateNotifications.push_back(
						new InsertUpdateNotification(partitionTargets.find(partItr->first)->second.at(i),
								partItr->second, insertOrUpdate));
			}
		}

	}

	void initDeleteNotification(){
		//TODO
	}

	void initAclAttrNotifications(){
		//TODO
	}
};
}
}
#endif // __SHARDING_SHARDING_EXT_INSERT_UPDATE_COMMAND_OPERATION_H__
