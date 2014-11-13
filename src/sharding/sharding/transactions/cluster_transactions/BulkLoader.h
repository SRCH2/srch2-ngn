#ifndef __SHARDING_SHARDING_BULK_LOADER_COMMAND_H__
#define __SHARDING_SHARDING_BULK_LOADER_COMMAND_H__

#include "../../state_machine/node_iterators/ConcurrentNotifOperation.h"
#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../notifications/CommandStatusNotification.h"
#include "../../metadata_manager/Shard.h"
#include "../Transaction.h"
#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"

#include <iostream>
#include <ctime>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class BulkLoader: public ProducerInterface, public NodeIteratorListenerInterface {
public:
	// calls consume(const vector<string> & attributeIds, const vector<JsonMessageCode> & messages); from the consumer
	BulkLoader(ConsumerInterface * consumer,
			const string & filePath,
			const CoreInfo_t * coreInfo):ProducerInterface(consumer),
			coreInfo(coreInfo),
			filePath(filePath){
		ASSERT(coreInfo != NULL);
		ASSERT(this->getConsumer() != NULL);
		ASSERT(this->getConsumer()->getTransaction());
		clusterReadview = ((ReadviewTransaction *)(this->getTransaction().get()))->getReadview();
	}
	~AclAttributeReadCommand();

	SP(Transaction) getTransaction(){
		if(this->getConsumer() == NULL){
			ASSERT(false);
			return SP(Transaction)();
		}
		return this->getConsumer()->getTransaction();
	}

	void produce(){
//		/*
//		 * 0. check the presence of the filePath
//		 */
//
//    	unsigned indexedCounter = 0;
//    	if(filePath.compare("") == 0){
//    		messageCodes.push_back(HTTP_Json_Data_File_Does_Not_Exist);
//    		finalize();
//    		return;
//    	}
//
//
//
//
//        //check file size in KB
//        unsigned fileSize = getFileSize(filePath.c_str());
//        //Logger::console("The size of the data file is %lu KB", fileSize/(1024));
//
//        struct statvfs *buff;
//        if (!(buff = (struct statvfs *) malloc(sizeof(struct statvfs)))) {
//            Logger::error("Failed to allocate memory to buffer.");
//            finalize();
//            return;
//        } else {
//            //We check the space available for the disk where srch2Home is set
//            if (statvfs(coreInfo->getSrch2Home().c_str(), buff) < 0) {
//                Logger::warn("Failed to calculate free disk space, statvfs() failed.");
//            } else {
//                //Logger::console("The number of free blocks on disk is %lu", buff->f_bfree);
//                //Logger::console("The size of each block is %lu bytes", buff->f_bsize);
//                //Logger::console("The total size of free disk space is %lu KB", (buff->f_bfree * buff->f_bsize) / (1024));
//
//                //calculate free disk space. (No. of free blocks * block size) KB
//                unsigned long freeDiskSpace = (buff->f_bfree * buff->f_bsize) / (1024);
//
//                //Display warning if free disk space is less than twice the size of file
//                if (freeDiskSpace < (2 * fileSize)) {
//                    Logger::warn("The system may not have enough disk space to serialize the indexes for the given json file.");
//                }
//            }
//            free(buff);
//        }
//
//		// Create from JSON and save to index-dir
//		Logger::console("Creating indexes from JSON file...");
//		indexedCounter = DaemonDataSource::createNewIndexFromFile(getIndexer(),
//				storedAttrSchema, this->getCoreInfo(),getDataFilePath());
//
//		/*
//		 *  commit the indexes once bulk load is done and then save it to the disk only
//		 *  if number of indexed record is > 0.
//		 */
//		getIndexer()->commit();
//        // Load ACL list from disk
//        indexer->getAttributeAcl().bulkLoadAttributeAclJSON(indexDataConfig->getAttibutesAclFile());
//
//        if(indexDataConfig->getHasRecordAcl()){
//        	DaemonDataSource::addRecordAclFile(indexer, indexDataConfig);
//        }
//
//        /*
//         *  if the roleCore is null it means that this core doesn't have any access control
//         *  so we can save it now.
//         *  otherwise first we should read the data for acl and we will save this core after that.
//         */
//        if (indexedCounter > 0) {
//            indexer->save();
//            Logger::console("Indexes saved.");
//        }
//		break;

	}



	void finalize(){
		//TODO : call consume
	}

	string getName() const {return "bulk-loader";};
private:
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	vector<JsonMessageCode> messageCodes;
	const string & filePath;
	const CoreInfo_t * coreInfo;
    ifstream::pos_type getFileSize(const char* filename) {
        std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
        ifstream::pos_type size = in.tellg();
        in.close();
        return size;
    }


    unsigned parseRecordsFromFile(){
//        ifstream in(filePath.c_str());
//        if (in.fail())
//        {
//            Logger::error("DataSource file not found at: %s", filePath.c_str());
//            return 0;
//        }
//
//        string line;
//        srch2is::Record *record = new srch2is::Record(coreInfo->getSchema());
//
//        unsigned lineCounter = 0;
//        unsigned indexedRecordsCount = 0;
//        // use same analyzer object for all the records
//        srch2is::Analyzer *analyzer = AnalyzerFactory::createAnalyzer(coreInfo);
//
//        RecordSerializer compactRecSerializer = RecordSerializer(*storedAttrSchema);
//
//        if(in.good()){
//            bool isArrayOfJsonRecords = false;
//            while(getline(in, line))
//            {
//                bool parseSuccess = false;
//
//            // remove the trailing space or "," characters
//            while (!line.empty() && (
//                        line.at(line.length() - 1) == ' ' ||
//                                        line.at(line.length() - 1) == ','
//                                        )
//                      ) {
//                    line.erase(line.length() - 1);
//            }
//
//                boost::trim(line);
//                if (indexedRecordsCount == 0 &&  line == "[") {
//                    // Solr style data source - array of JSON records
//                    isArrayOfJsonRecords = true;
//                    continue;
//                }
//                if (isArrayOfJsonRecords == true && line == "]") {
//                    // end of JSON array in Solr style data source
//                    break; // assume nothing follows array (will ignore more records or another array)
//                }
//
//                std::stringstream error;
//                parseSuccess = JSONRecordParser::populateRecordFromJSON(line, indexDataContainerConf, record, error, compactRecSerializer);
//
//                if(parseSuccess)
//                {
//                    // Add the record to the index
//                    //indexer->addRecordBeforeCommit(record, 0);
//                    indexer->addRecord(record, analyzer);
//                    indexedRecordsCount++;
//                }
//                else
//                {
//                    Logger::error("at line: %d" , lineCounter);
//                    Logger::error("%s", error.str().c_str());
//                }
//                record->clear();
//                int reportFreq = 10000;
//                ++lineCounter;
//                if (indexedRecordsCount % reportFreq == 0) {
//                    Logger::console("Indexing first %d records.\r", indexedRecordsCount);
//                }
//            }
//        }
//        Logger::console("Indexed %d / %d records.", indexedRecordsCount, lineCounter);
//        Logger::console("Finalizing ...");
//        in.close();
//
//        delete analyzer;
//        delete record;
//        return indexedRecordsCount;
    }

};


}
}


#endif // __SHARDING_SHARDING_BULK_LOADER_COMMAND_H__
