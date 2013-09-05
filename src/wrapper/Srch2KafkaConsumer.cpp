//$Id: Srch2KafkaConsumer.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

#include "Srch2KafkaConsumer.h"
#include "IndexWriteUtil.h"
#include "json/json.h"
#include "util/Logger.h"
#include "util/FileOps.h"
#include "index/IndexUtil.h"
#include "MongodbAdapter.h"

using namespace srch2::instantsearch;
namespace srch2is = srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

namespace srch2http = srch2::httpwrapper;

IndexMetaData *Srch2KafkaConsumer::createIndexMetaData(const ConfigManager *indexDataContainerConf)
{
	//Create a cache
	srch2is::GlobalCache *cache = srch2is::GlobalCache::create(indexDataContainerConf->getCacheSizeInBytes(), 200000);

	// Create an IndexMetaData
	srch2is::IndexMetaData *indexMetaData = new srch2is::IndexMetaData( cache,
							   indexDataContainerConf->getMergeEveryNSeconds(),
							   indexDataContainerConf->getMergeEveryMWrites(),
							   indexDataContainerConf->getIndexPath(),
							   indexDataContainerConf->getTrieBootstrapDictFileName());

	return indexMetaData;
}

// Check if index files already exist.
bool checkIndexExistence(const ConfigManager *indexDataContainerConf)
{
    const string &directoryName = indexDataContainerConf->getIndexPath();
    if(!checkDirExistence((directoryName + "/" + IndexConfig::analyzerFileName).c_str()))
        return false;
    if(!checkDirExistence((directoryName + "/" + IndexConfig::trieFileName).c_str()))
        return false;
    if(!checkDirExistence((directoryName + "/" + IndexConfig::forwardIndexFileName).c_str()))
        return false;
    if(!checkDirExistence((directoryName + "/" + IndexConfig::schemaFileName).c_str()))
        return false;
    if (indexDataContainerConf->getIndexType() == srch2::instantsearch::DefaultIndex){
        // Check existence of the inverted index file for basic keyword search ("A1")
        if(!checkDirExistence((directoryName + "/" + IndexConfig::invertedIndexFileName).c_str()))
            return false;
    }else{
        // Check existence of the quadtree index file for geo keyword search ("M1")
        if(!checkDirExistence((directoryName + "/" + IndexConfig::quadTreeFileName).c_str()))
            return false;
    }
    return true;
}

void Srch2KafkaConsumer::createAndBootStrapIndexer()
{
	// create IndexMetaData
	IndexMetaData *indexMetaData = createIndexMetaData(this->indexDataContainerConf);
	IndexCreateOrLoad indexCreateOrLoad;
	if(checkIndexExistence(indexDataContainerConf))
	    indexCreateOrLoad = srch2http::INDEXLOAD;
	else
	    indexCreateOrLoad = srch2http::INDEXCREATE;

	switch (indexCreateOrLoad)
	{
		case srch2http::INDEXCREATE:
		{
			AnalyzerHelper::initializeAnalyzerResource(this->indexDataContainerConf);
			// Create a schema to the data source definition in the Srch2ServerConf
			srch2is::Schema *schema = JSONRecordParser::createAndPopulateSchema(indexDataContainerConf);
			Analyzer *analyzer = AnalyzerFactory::createAnalyzer(this->indexDataContainerConf);
			indexer = Indexer::create(indexMetaData, analyzer, schema);
			delete analyzer;
			switch(indexDataContainerConf->getDataSourceType())
			{
				case srch2http::FILEBOOTSTRAP_TRUE:
				{
					// Create from JSON and save to index-dir
                    Logger::console("Creating an index from JSON file...");
					DaemonDataSource::createNewIndexFromFile(indexer, indexDataContainerConf);
					this->offset = this->indexer->getKafkaOffsetFromIndexSnapShot();
					break;
				}
				case srch2http::MONGOBOOTSTRAP:
				{
					Logger::console("Creating an index from MongoDb instance...");
					MongoDataSource::createNewIndexes(indexer, indexDataContainerConf);
					break;
				}
				default:
				{
                    Logger::console("Creating new empty index");
				}
			};
			AnalyzerHelper::saveAnalyzerResource(this->indexDataContainerConf);
			break;
		}
		case srch2http::INDEXLOAD:
		{
			// Load from index-dir directly, skip creating an index initially.
			indexer = Indexer::load(indexMetaData);
			// Load Analayzer data from disk
			AnalyzerHelper::loadAnalyzerResource(this->indexDataContainerConf);
			bool isAttributeBasedSearch = (indexer->getSchema()->getPositionIndexType() == srch2::instantsearch::FIELDBITINDEX);
			if(isAttributeBasedSearch != indexDataContainerConf->getSupportAttributeBasedSearch())
			{
				cout << indexer->getSchema()->getPositionIndexType() << " " << indexDataContainerConf->getSupportAttributeBasedSearch() <<endl;
				cout << "[Warning] support-attribute-based-search changed in config file, remove all index files and run it again!"<< endl;
			}
			break;
		}
	}
}

void Srch2KafkaConsumer::_init_consumer() {
	if (this->srch2Consumer != NULL) {
		delete this->srch2Consumer;
	}

	this->work_consumer.reset(new boost::asio::io_service::work(this->io_service));
	this->srch2Consumer = new kafkaconnect::consumer(this->io_service, this->kafkaBrokerHostName, this->kafkaBrokerPort);
	
	/*while(!this->srch2Consumer->is_connected())
	{
		boost::this_thread::sleep(boost::posix_time::seconds(1));
	}*/
}

void Srch2KafkaConsumer::_init_producer() {
	if (this->srch2Producer != NULL) {
		delete this->srch2Producer;
	}

	this->work_producer.reset(new boost::asio::io_service::work(this->io_service));
	this->srch2Producer = new kafkaconnect::producer(this->io_service, this->kafkaBrokerHostName, this->kafkaBrokerPort);
	
	/*while(!this->srch2Producer->is_connected())
	{
		boost::this_thread::sleep(boost::posix_time::seconds(1));
	}*/
}


bool Srch2KafkaConsumer::produceHealthMessage() {
	if (this->srch2Producer->is_connected()) {
		std::vector<std::string> messages;
		messages.push_back(this->indexer->getIndexHealth());
		return this->srch2Producer->send(messages, kafkaConsumerTopicName + "_health", kafkaConsumerPartitionId);
	} else {
		this->_init_producer();
		return false;
	}
}

//TODO add logging to kafka
bool Srch2KafkaConsumer::consumeMessages() {
	bool continueIteration = true;
	do {
		if (this->srch2Consumer->is_connected()) {
			std::vector<std::string> messages;
			uint64_t newOffset = this->offset;

			continueIteration = this->srch2Consumer->consume(messages,
					kafkaConsumerTopicName, kafkaConsumerPartitionId,
					kafkaConsumerReadBufferInBytes, this->offset, newOffset);

			std::cout << kafkaConsumerTopicName << ":"
					<< kafkaConsumerPartitionId << ":"
					<< kafkaConsumerReadBufferInBytes
					<< "|num.messages:["
					<< messages.size() << "]oldoffset:[" << this->offset
					<< "]newoffset[" << newOffset << "]" << std::endl;

			this->offset = newOffset;
			this->_executeMessagesOnIndex(messages, this->offset);
		} else {
			this->_init_consumer();
		}
	} while (continueIteration);

	return true;
}

void Srch2KafkaConsumer::_executeMessagesOnIndex(const std::vector<std::string> &messages, const uint64_t offset)
{
	Record *record = new Record(this->indexer->getSchema());
	for (std::vector<std::string>::const_iterator iter = messages.begin();
				iter != messages.end();
				iter++)
	{
		std::stringstream log_str;

		const std::string inputLine = *iter;

		// Parse example data
		Json::Value root;
		Json::Value payload;
		Json::Reader reader;
		bool parseSuccess = reader.parse(inputLine, root, false);

		if(parseSuccess == false)
		{
			log_str << "[FAILED: " << inputLine << "]";
		}
		else
		{
			//JSON message command type
			const string commandTypeJSONVariableName = "command";
			
			//std::cout << "command[" << root.get(commandTypeJSONVariableName, "4" ).asString() << "]" << std::endl;
			
			int command = root.get(commandTypeJSONVariableName, "4" ).asInt();
			
			switch(command)
			{
				case 1: //insert
					payload = root["payload"];
					IndexWriteUtil::_insertCommand(this->indexer, this->indexDataContainerConf, payload, offset, record, log_str);
					record->clear();
					break;
				case 2: //delete
					payload = root["payload"];
					IndexWriteUtil::_deleteCommand(this->indexer, this->indexDataContainerConf, payload, offset, log_str);
					break;
				case 3: //commit
					std::cout << "[COMMIT][start]" << std::endl;
					IndexWriteUtil::_commitCommand(this->indexer, this->indexDataContainerConf, offset, log_str);
					break;
				default: //unrecognized. No op. Log.
					break;
			};
		}
		
		std::cout << log_str.str() << std::endl;
	}
	delete record;
}

}}
