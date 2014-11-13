#ifndef __SHARDING_SHARDING_BULK_LOAD_COMMAND_H__
#define __SHARDING_SHARDING_BULK_LOAD_COMMAND_H__

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
#include <sys/stat.h>
#ifndef ANDROID
#   include <sys/statvfs.h>
#else
#   include <sys/vfs.h>
#   define statvfs statfs
#endif


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

#define BULK_LOAD_BATCH_SIZE 1000

enum BulkLoadType {
	RecordBulkLoad,
	AclRecordBulkLoad,
	AclAttributeBulkLoad,
};

class BulkLoadCommand: public ProducerInterface {
public:
	BulkLoadCommand(ConsumerInterface * consumer,
			const string & filePath, BulkLoadType bulkLoadType,
			const CoreInfo_t * coreInfo):ProducerInterface(consumer),
			coreInfo(coreInfo),
			filePath(filePath){
		ASSERT(coreInfo != NULL);
		ASSERT(this->getConsumer() != NULL);
		ASSERT(this->getConsumer()->getTransaction());
		clusterReadview = ((ReadviewTransaction *)(this->getTransaction().get()))->getReadview();
		this->bulkLoadType = bulkLoadType;
		this->isArrayOfJsonRecords = false;
		this->lineProcessed = 0;
		bulkLoadWriteCommand = NULL;
		compactRecSerializer = NULL;
	}
	~BulkLoadCommand() {
		bulkdLoadFileStream.close();
    	delete compactRecSerializer;
	}

	SP(Transaction) getTransaction(){
		if(this->getConsumer() == NULL){
			ASSERT(false);
			return SP(Transaction)();
		}
		return this->getConsumer()->getTransaction();
	}

	void init() {
    	struct stat filestat;
    	if (::stat(filePath.c_str(), &filestat) == -1) {
    		messageCodes.push_back(HTTP_Json_Data_File_Does_Not_Exist);
    		finalize();
    		return;
    	}
        //check file size in KB
        unsigned fileSize = getFileSize(filePath.c_str());
        //Logger::console("The size of the data file is %lu KB", fileSize/(1024));

        struct statvfs *buff;
        if (!(buff = (struct statvfs *) malloc(sizeof(struct statvfs)))) {
            Logger::error("Failed to allocate memory to buffer.");
            finalize();
            return;
        } else {
            //We check the space available for the disk where srch2Home is set
            if (statvfs(coreInfo->getSrch2Home().c_str(), buff) < 0) {
                Logger::warn("Failed to calculate free disk space, statvfs() failed.");
            } else {
                //calculate free disk space. (No. of free blocks * block size) KB
                unsigned long freeDiskSpace = (buff->f_bfree * buff->f_bsize) / (1024);

                //Display warning if free disk space is less than twice the size of file
                if (freeDiskSpace < (2 * fileSize)) {
                    Logger::warn("The system may not have enough disk space to serialize the indexes for the given json file.");
                }
            }
            free(buff);
        }

		// Create from JSON and save to index-dir
		Logger::console("Creating indexes from JSON file...");

		bulkdLoadFileStream.open(filePath.c_str());
		if (bulkdLoadFileStream.fail())
		{
			Logger::error("DataSource file not found at: %s", filePath.c_str());
            finalize();
			return;
		}

		Schema * storedAttrSchema = Schema::create();
		RecordSerializerUtil::populateStoredSchema(storedAttrSchema,
				coreInfo->getSchema());
		compactRecSerializer = new RecordSerializer(*storedAttrSchema);

	}

	void produce(){


		bulkLoadWriteCommand = parseRecordsFromFile();

		if (bulkLoadWriteCommand)
			bulkLoadWriteCommand->produce();

		performCleanup();
	}

	void finalize(){
		//TODO : call consume
	}

	string getName() const {return "bulk-loader";};

	bool isBulkLoadDone() const { return bulkdLoadFileStream.eof(); }

private:

    ifstream::pos_type getFileSize(const char* filename) {
        std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
        ifstream::pos_type size = in.tellg();
        in.close();
        return size;
    }

    void performCleanup() {
    	delete bulkLoadWriteCommand;
    	for (unsigned i = 0; i < recordsToIndex.size(); ++i) {
    		delete recordsToIndex[i];
    	}
    	recordsToIndex.clear();
    	aclDataForWriteCommand.clear();
    }

    WriteCommand * parseRecordsFromFile() {

		unsigned successCounter = 0;
		string line;
    	while(getline(bulkdLoadFileStream, line))
    	{
        	if (lineProcessed == 0 &&  line == "[") {
        		// Solr style data source - array of JSON records
        		isArrayOfJsonRecords = true;
        		++lineProcessed;
        		continue;
        	}
        	if (isArrayOfJsonRecords == true && line == "]") {
        		// end of JSON array in Solr style data source
        		break; // assume nothing follows array (will ignore more records or another array)
        	}
    		++lineProcessed;
        	std::stringstream error;
        	switch(bulkLoadType) {
        	case RecordBulkLoad: {
        		srch2is::Record*  record = parseRecordsFromString(line, *compactRecSerializer, error);
        		if (record) {
        			++successCounter;
        			recordsToIndex.push_back(record);
        		} else {
        			Logger::error("BulkLoad : Cannot process line = %d,  error =  %s", lineProcessed,
        					error.str().c_str());
        		}
        		break;
        	}
        	case AclAttributeBulkLoad:{
        		vector<string> attributeList;
        		vector<string> roleIdList;
        		Json::Value doc(line);
        		Json::Value aclAttributeResponse;
        		bool success = AttributeAccessControl::processSingleJSONAttributeAcl(
        				doc, "http bulkload", aclAttributeResponse, roleIdList,
        				attributeList, *coreInfo->getSchema());
        		if (success) {
        			++successCounter;
        			AclCommandHttpHandler::prepareAclDataForApiLayer(aclDataForWriteCommand,
        			    	        						roleIdList, attributeList);
        		} else {
        			Logger::error("BulkLoad : Cannot process line = %d,  error =  %s", lineProcessed,
        					error.str().c_str());
        		}
        		break;
        	}
        	case AclRecordBulkLoad:{
        		vector<string> roleIdList;
        		string primaryKeyID;
        		Json::Value doc(line);
        		Json::Value aclAttributeResponse;
        		stringstream log_str;
        		bool success = JSONRecordParser::_extractResourceAndRoleIds(roleIdList,
        				primaryKeyID, doc, coreInfo, log_str);
        		if (success) {
        			++successCounter;
        			AclRecordCommandHttpHandler::prepareAclDataForApiLayer(aclDataForWriteCommand,
        					primaryKeyID, roleIdList);
        		} else {
        			Logger::error("BulkLoad : Cannot process line = %d,  error =  %s", lineProcessed,
        					error.str().c_str());
        		}
        		break;
        	}
        	default:
        		ASSERT(false);
        	}

    		if (successCounter > BULK_LOAD_BATCH_SIZE) {
    			break;
    		}
    	}

    	switch(bulkLoadType) {
    	case RecordBulkLoad:
    	{
    		return new WriteCommand(this->getConsumer(),recordsToIndex, Insert_ClusterRecordOperation_Type, coreInfo);
    		break;
    	}
    	case AclRecordBulkLoad:
    	{
    		return new WriteCommand(this->getConsumer(), aclDataForWriteCommand, Acl_Record_Add, coreInfo);
    		break;
    	}
    	case AclAttributeBulkLoad:
    	{
    		unsigned aclCoreId =  coreInfo->getAttributeAclCoreId();
    		const CoreInfo_t * aclCoreInfo = clusterReadview->getCore(aclCoreId);
    		ASSERT(aclCoreInfo == NULL);
    		if (aclCoreInfo)
    			return new WriteCommand(this->getConsumer(), aclDataForWriteCommand, ACL_APPEND, aclCoreInfo);
    		else
    			return NULL;
    		break;
    	}
    	default:
    		ASSERT(false);
    		return NULL;
    	}
    }

    srch2is::Record* parseRecordsFromString(std::string& line,
    		RecordSerializer& compactRecSerializer,  std::stringstream& error){

    	bool parseSuccess = false;

    	// remove the trailing space or "," characters
    	while (!line.empty() &&
    			(line.at(line.length() - 1) == ' ' || line.at(line.length() - 1) == ',')) {
    		line.erase(line.length() - 1);
    	}
    	boost::trim(line);

    	if (line.empty()) {
    		error << "Line is either empty or does not have JSON record.";
    		return NULL;
    	}

    	srch2is::Record* record = new srch2is::Record(coreInfo->getSchema());

    	parseSuccess = JSONRecordParser::populateRecordFromJSON(line, coreInfo, record,
    			error, compactRecSerializer);

    	if(parseSuccess)
    	{
    		return record;
    	} else {
    		delete record;
    		return NULL;
    	}
    }

private:  // data

	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	vector<JsonMessageCode> messageCodes;
	const string & filePath;
	const CoreInfo_t * coreInfo;

    BulkLoadType bulkLoadType;
    ifstream bulkdLoadFileStream;
    RecordSerializer *compactRecSerializer;
    unsigned lineProcessed;
    bool isArrayOfJsonRecords;
    WriteCommand * bulkLoadWriteCommand;

    vector<Record *> recordsToIndex;  // for record bulk load
    std::map< string, vector<string> > aclDataForWriteCommand;  // for acl bulk load
};


}
}


#endif // __SHARDING_SHARDING_BULK_LOAD_COMMAND_H__
