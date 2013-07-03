//$Id: BimapleKafkaConsumer.cpp 2647 2012-07-03 17:02:15Z oliverax $

#include "BimapleKafkaConsumer.h"
#include "IndexWriteUtil.h"
#include "json/json.h"

using namespace bimaple::instantsearch;
namespace bmis = bimaple::instantsearch;

namespace bimaple
{
namespace httpwrapper
{

namespace bmhttp = bimaple::httpwrapper;

IndexMetaData *BimapleKafkaConsumer::createIndexMetaData(const BimapleServerConf *indexDataContainerConf)
{
	//Create a cache
	bmis::GlobalCache *cache = bmis::GlobalCache::create(indexDataContainerConf->getCacheSizeInBytes(), 200000);

	// Create an IndexMetaData
	bmis::IndexMetaData *indexMetaData = new bmis::IndexMetaData( cache,
							   indexDataContainerConf->getMergeEveryNSeconds(),
							   indexDataContainerConf->getMergeEveryMWrites(),
							   indexDataContainerConf->getIndexPath(),
							   indexDataContainerConf->getTrieBootstrapDictFileName(),
							   indexDataContainerConf->getLicenseKeyFileName()
							  );

	return indexMetaData;
}

void BimapleKafkaConsumer::createAndBootStrapIndexer(const BimapleServerLogger *bimapleServerLogger)
{
	// create IndexMetaData
	IndexMetaData *indexMetaData = createIndexMetaData(this->indexDataContainerConf);

	switch (indexDataContainerConf->getIndexCreateOrLoad())
	{
		case bmhttp::INDEXCREATE:
		{
			// Create an analyzer
			bmis::Analyzer *analyzer = bmis::Analyzer::create(bimaple::instantsearch::NO_STEMMER_NORMALIZER, indexDataContainerConf->getRecordAllowedSpecialCharacters());

			// Create a schema to the data source definition in the BimapleServerConf
			bmis::Schema *schema = JSONRecordParser::createAndPopulateSchema(indexDataContainerConf);

			indexer = Indexer::create(indexMetaData, analyzer, schema);
			switch(indexDataContainerConf->getDataSourceType())
			{
				case bmhttp::FILEBOOTSTRAP_TRUE:
				{
					// Create from JSON and save to index-dir
				  cout << "Creating an index from JSON file..." << endl;
                    bimapleServerLogger->BMLog(1, "%s", "Creating an index from JSON file...");
					DaemonDataSource::createNewIndexFromFile(indexer, indexDataContainerConf, bimapleServerLogger);
					this->offset = this->indexer->getKafkaOffsetFromIndexSnapShot();
					break;
				}
				default:
				{
					cout << "Creating new empty index" << endl;
                    bimapleServerLogger->BMLog(1, "%s", "Creating new empty index");
				}
			};
			break;
		}
		case bmhttp::INDEXLOAD:
		{
			// Load from index-dir directly, skip creating an index initially.
			indexer = Indexer::load(indexMetaData);
			break;
		}
	}
}

void BimapleKafkaConsumer::_init_consumer()
{
	if ( this->bimapleConsumer != NULL)
	{
		delete this->bimapleConsumer;
	}

	this->work_consumer.reset(new boost::asio::io_service::work(this->io_service));
	this->bimapleConsumer = new kafkaconnect::consumer(this->io_service, this->kafkaBrokerHostName, this->kafkaBrokerPort);
	
	/*while(!this->bimapleConsumer->is_connected())
	{
		boost::this_thread::sleep(boost::posix_time::seconds(1));
	}*/
}

void BimapleKafkaConsumer::_init_producer()
{
	if ( this->bimapleProducer != NULL)
	{
		delete this->bimapleProducer;
	}

	this->work_producer.reset(new boost::asio::io_service::work(this->io_service));
	this->bimapleProducer = new kafkaconnect::producer(this->io_service, this->kafkaBrokerHostName, this->kafkaBrokerPort);
	
	/*while(!this->bimapleProducer->is_connected())
	{
		boost::this_thread::sleep(boost::posix_time::seconds(1));
	}*/
}


bool BimapleKafkaConsumer::produceHealthMessage()
{
	if (this->bimapleProducer->is_connected())
	{
		std::vector<std::string> messages;
		messages.push_back(this->indexer->getIndexHealth());
		return this->bimapleProducer->send(messages, kafkaConsumerTopicName+"_health", kafkaConsumerPartitionId);
	}
	else
	{
		this->_init_producer();
		return false;
	}	
}

//TODO add logging to kafka
bool BimapleKafkaConsumer::consumeMessages()
{
	bool continueIteration = true;
	do
	{
		if (this->bimapleConsumer->is_connected())
		{
			std::vector<std::string> messages;
			uint64_t newOffset = this->offset;

			continueIteration =
					this->bimapleConsumer->consume(messages,
										kafkaConsumerTopicName,	kafkaConsumerPartitionId, kafkaConsumerReadBufferInBytes,
										this->offset, newOffset);

			std::cout << kafkaConsumerTopicName << ":" << kafkaConsumerPartitionId  << ":" << kafkaConsumerReadBufferInBytes 
					<< "|num.messages:[" << messages.size() << "]oldoffset:[" << this->offset << "]newoffset[" << newOffset << "]" << std::endl;

			this->offset = newOffset;
			this->_executeMessagesOnIndex(messages, this->offset);
		}
		else
		{
			this->_init_consumer();
		}
	} while(continueIteration);

	return true;
}

void BimapleKafkaConsumer::_executeMessagesOnIndex(const std::vector<std::string> &messages, const uint64_t offset)
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
