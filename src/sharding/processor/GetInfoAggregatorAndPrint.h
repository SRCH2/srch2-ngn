#ifndef __SHARDING_PROCESSOR_GET_INFO_AGGREGATOR_AND_PRINT_H_
#define __SHARDING_PROCESSOR_GET_INFO_AGGREGATOR_AND_PRINT_H_

#include "ResultsAggregatorAndPrint.h"

#include "serializables/SerializableGetInfoCommandInput.h"
#include "serializables/SerializableGetInfoResults.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {


class GetInfoAggregatorAndPrint : public ResultAggregatorAndPrint<SerializableGetInfoCommandInput,SerializableGetInfoResults> {
public:
	GetInfoAggregatorAndPrint(ConfigManager * configurationManager, evhttp_request *req);

	/*
	 * This function is always called by RoutingManager as the first call back function
	 */
	void preProcessing(ResultsAggregatorAndPrintMetadata metadata);
	/*
	 * This function is called by RoutingManager if a timeout happens, The call to
	 * this function must be between preProcessing(...) and callBack()
	 */
	void timeoutProcessing(PendingMessage<SerializableGetInfoCommandInput, SerializableGetInfoResults> * message,
			ResultsAggregatorAndPrintMetadata metadata);


	/*
	 * The main function responsible of aggregating status (success or failure) results
	 */
	void callBack(vector<PendingMessage<SerializableGetInfoCommandInput, SerializableGetInfoResults> * > messages);

	/*
	 * The last call back function called by RoutingManager in all cases.
	 * Example of call back call order for search :
	 * 1. preProcessing()
	 * 2. timeoutProcessing() [only if some shard times out]
	 * 3. aggregateSearchResults()
	 * 4. finalize()
	 */
	void finalize(ResultsAggregatorAndPrintMetadata metadata);

private:
	ConfigManager * configurationManager;
	evhttp_request *req;

	mutable boost::shared_mutex _access;
	std::stringstream messages;
	unsigned readCount;
	unsigned writeCount;
	unsigned numberOfDocumentsInIndex;
	vector<string> lastMergeTimeStrings;
	vector<string> versionInfoStrings;
	unsigned docCount;
};

}
}

#endif // __SHARDING_PROCESSOR_GET_INFO_AGGREGATOR_AND_PRINT_H_
