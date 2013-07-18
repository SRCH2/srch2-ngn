//$Id: Srch2KafkaConsumer.cpp 3480 2013-06-19 08:00:34Z jiaying $

#include "Srch2KafkaConsumer.h"
#include "IndexWriteUtil.h"
#include "json/json.h"
#include "util/Logger.h"

using namespace srch2::instantsearch;
namespace srch2is = srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

namespace srch2http = srch2::httpwrapper;

IndexMetaData *Srch2KafkaConsumer::createIndexMetaData(const Srch2ServerConf *indexDataContainerConf)
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

void Srch2KafkaConsumer::createAndBootStrapIndexer()
{
	// create IndexMetaData
	IndexMetaData *indexMetaData = createIndexMetaData(this->indexDataContainerConf);

	switch (indexDataContainerConf->getIndexCreateOrLoad())
	{
		case srch2http::INDEXCREATE:
		{


			// This flag shows if we need to stem or not. (StemmerNormalizerType is an enum)
			StemmerNormalizerFlagType stemmerFlag;
			// gets the stem flag and set the stemType
			if (indexDataContainerConf->getStemmerFlag()) {
				stemmerFlag = srch2is::ENABLE_STEMMER_NORMALIZER;
			} else {
				stemmerFlag = srch2is::DISABLE_STEMMER_NORMALIZER;
			}
			// This flag shows if we need to keep the origin word or not.
			SynonymKeepOriginFlag synonymKeepOriginFlag;
			// gets the stem flag and set the stemType
			if (indexDataContainerConf->getSynonymKeepOrigFlag()) {
				synonymKeepOriginFlag = srch2is::SYNONYM_KEEP_ORIGIN;
			} else {
				synonymKeepOriginFlag = srch2is::SYNONYM_DONOT_KEEP_ORIGIN;
			}

			// append the stemmer file to the install direcrtory
			std::string stemmerFilterFilePath = indexDataContainerConf->getInstallDir() + indexDataContainerConf->getStemmerFile();
			// gets the path of stopFilter
			std::string stopFilterFilePath = indexDataContainerConf->getStopFilePath();
			// gets the path of stopFilter
			std::string  synonymFilterFilePath = indexDataContainerConf->getSynonymFilePath();

			// Create an analyzer
			srch2is::Analyzer *analyzer = srch2is::Analyzer::create(stemmerFlag,
					stemmerFilterFilePath,
					stopFilterFilePath,
					synonymFilterFilePath,
					synonymKeepOriginFlag,
					indexDataContainerConf->getRecordAllowedSpecialCharacters());

			// Create a schema to the data source definition in the Srch2ServerConf
			srch2is::Schema *schema = JSONRecordParser::createAndPopulateSchema(indexDataContainerConf);

			indexer = Indexer::create(indexMetaData, analyzer, schema);
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
				default:
				{
                    Logger::console("Creating new empty index");
				}
			};
			break;
		}
		case srch2http::INDEXLOAD:
		{
			// Load from index-dir directly, skip creating an index initially.
			indexer = Indexer::load(indexMetaData);

			bool isAttributeBasedSearch = (indexer->getSchema()->getPositionIndexType() == srch2::instantsearch::FIELDBITINDEX);
			if(isAttributeBasedSearch != indexDataContainerConf->getSupportAttributeBasedSearch())
			{
				cout << indexer->getSchema()->getPositionIndexType() << " " << indexDataContainerConf->getSupportAttributeBasedSearch() <<endl;
				cout << "[Warning] support-attribute-based-search changed in config file, run with index-load-or-create=0 again!"<< endl;
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
