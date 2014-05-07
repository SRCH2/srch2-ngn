#ifndef __SHARDING_PROCESSOR_COMMAND_STATUS_AGGREGATOR_AND_PRINT_H_
#define __SHARDING_PROCESSOR_COMMAND_STATUS_AGGREGATOR_AND_PRINT_H_

#include "ResultsAggregatorAndPrint.h"
#include "serializables/SerializableCommandStatus.h"
#include "serializables/SerializableInsertUpdateCommandInput.h"
#include "serializables/SerializableDeleteCommandInput.h"
#include "serializables/SerializableSerializeCommandInput.h"
#include "serializables/SerializableResetLogCommandInput.h"
#include "serializables/SerializableCommitCommandInput.h"

#include <string>
#include <sstream>

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

template <class RequestWithStatusResponse>
class CommandStatusAggregatorAndPrint : public ResultAggregatorAndPrint<RequestWithStatusResponse,SerializableCommandStatus> {
public:


	CommandStatusAggregatorAndPrint(ConfigManager * configurationManager, evhttp_request *req){
		this->configurationManager = configurationManager;
		this->req = req;
	}

	void setMessages(std::stringstream & log_str){
		this->messages << log_str.str();
	}

	std::stringstream & getMessages(){
		return this->messages;
	}

	/*
	 * This function is always called by RoutingManager as the first call back function
	 */
	void preProcessing(ResultsAggregatorAndPrintMetadata metadata){

	}

	/*
	 * This function is called by RoutingManager if a timeout happens, The call to
	 * this function must be between preProcessing(...) and callBack()
	 */
	/*
	 * This function is called by RoutingManager if a timeout happens, The call to
	 * this function must be between preProcessing(...) and callBack()
	 */
	void timeoutProcessing(ShardId * shardInfo, RequestWithStatusResponse * sentRequest, ResultsAggregatorAndPrintMetadata metadata){

		if(((string)"SerializableInsertUpdateCommandInput").compare(typeid(sentRequest).name()) == 0){// timeout in insert and update

			boost::unique_lock< boost::shared_mutex > lock(_access);
			SerializableInsertUpdateCommandInput * sentInsetUpdateRequest = (SerializableInsertUpdateCommandInput *)(sentRequest);
			messages << "{\"rid\":\"" << sentInsetUpdateRequest->getRecord()->getPrimaryKey()
								<< "\",\"" << (sentInsetUpdateRequest->getInsertOrUpdate()?"insert":"update") << "\":\"failed\",\"reason\":\"Corresponging shard ("<<
								shardInfo->toString()<<") timedout.\"}";

		}else if (((string)"SerializableDeleteCommandInput").compare(typeid(sentRequest).name()) == 0){

			boost::unique_lock< boost::shared_mutex > lock(_access);
			SerializableDeleteCommandInput * sentDeleteRequest = (SerializableDeleteCommandInput *)(sentRequest);
			messages << "{\"rid\":\"" << sentDeleteRequest->getPrimaryKey()
					<< "\",\"delete\":\"failed\",\"reason\":\"Corresponging ("<<
					shardInfo->toString() << ") shard timedout.\"}";

		}else if(((string)"SerializableSerializeCommandInput").compare(typeid(sentRequest).name()) == 0){

			boost::unique_lock< boost::shared_mutex > lock(_access);
			SerializableSerializeCommandInput * serializeRequest = (SerializableSerializeCommandInput *)(sentRequest);
			messages << "{\""<< (serializeRequest->getIndexOrRecord()?"save":"export") << "\":\"failed\",\"reason\":\"Corresponging (" <<
					shardInfo->toString() << ") shard timedout.\"}";

		}else if(((string)"SerializableResetLogCommandInput").compare(typeid(sentRequest).name()) == 0){

			boost::unique_lock< boost::shared_mutex > lock(_access);
			SerializableResetLogCommandInput * resetRequest = (SerializableResetLogCommandInput *)(sentRequest);
			messages << "{\"reset_log\":\"failed\",\"reason\":\"Corresponging (" << shardInfo->toString()<<") shard timedout.\"}";

		}else if(((string)"SerializableCommitCommandInput").compare(typeid(sentRequest).name()) == 0){

			boost::unique_lock< boost::shared_mutex > lock(_access);
			SerializableCommitCommandInput * resetRequest = (SerializableCommitCommandInput *)(sentRequest);
			messages << "{\"commit\":\"failed\",\"reason\":\"Corresponging (" << shardInfo->toString()<<") shard timedout.\"}";

		}else{
			//TODO : what should we do here?
			ASSERT(false);
			return;
		}
	}

	/*
	 * The main function responsible of aggregating status (success or failure) results
	 */
	void callBack(const SerializableCommandStatus * responseObject){

		boost::unique_lock< boost::shared_mutex > lock(_access);
		messages << responseObject->getMessage();

	}

	void callBack(vector<const SerializableCommandStatus *> responseObjects){

		boost::unique_lock< boost::shared_mutex > lock(_access);
		for(vector<const SerializableCommandStatus *>::iterator responseItr = responseObjects.begin(); responseItr != responseObjects.end(); ++responseItr){
			messages << (*responseItr)->getMessage();
		}
	}
	/*
	 * The last call back function called by RoutingManager in all cases.
	 * Example of call back call order for search :
	 * 1. preProcessing()
	 * 2. timeoutProcessing() [only if some shard times out]
	 * 3. aggregateSearchResults()
	 * 4. finalize()
	 */
	void finalize(ResultsAggregatorAndPrintMetadata metadata){
		Logger::info("%s", messages.str().c_str());

		bmhelper_evhttp_send_reply2(req, HTTP_OK, "OK",
				"{\"message\":\"The batch was processed successfully\",\"log\":["
				+ messages.str() + "]}\n");
	}


private:



	ConfigManager * configurationManager;
	evhttp_request *req;

	mutable boost::shared_mutex _access;
	std::stringstream messages;
};

}
}

#endif // __SHARDING_PROCESSOR_COMMAND_STATUS_AGGREGATOR_AND_PRINT_H_
