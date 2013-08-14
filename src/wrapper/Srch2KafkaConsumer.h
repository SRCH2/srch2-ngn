//$Id: Srch2KafkaConsumer.h 3456 2013-06-14 02:11:13Z jiaying $

/**
 *  Created on: 25 Jan 2012
 *  Author: Vijay Rajakumar (@rvijax)
 */

#ifndef __SRCH2_KAFKA_CONSUMER_WRAPPER_H_
#define __SRCH2_KAFKA_CONSUMER_WRAPPER_H_

#include <string>
#include <vector>

#include <iostream>

#include <stdint.h>
#include <fstream>
#include <sstream>

#include <instantsearch/Indexer.h>
#include <instantsearch/Record.h>
#include <instantsearch/Schema.h>
#include "thirdparty/kafka/consumer/consumer.hpp"
#include "thirdparty/kafka/producer/producer.hpp"
//#include "JSONRecordParser.h"
#include "ConfigManager.h"

namespace srch2is = srch2::instantsearch;
namespace srch2http = srch2::httpwrapper;

using namespace srch2is;

namespace srch2
{
namespace httpwrapper
{

class Srch2KafkaConsumer
{
public:

	Srch2KafkaConsumer(const ConfigManager *indexDataContainerConf)
	{
		this->indexDataContainerConf = indexDataContainerConf;

		//pthread_mutex_init(&mutex_writer, 0);
		this->offset = 0;

	    this->createAndBootStrapIndexer();

	    if ( this->indexDataContainerConf->getWriteApiType() == srch2http::KAFKAWRITEAPI)
	    {
	    	this->initKafkaConsumerAndThreadLoop();
		    std::cout << "Kafka Init-Commit Offset:[" << this->offset << "]" << std::endl;
	    }
	    else
	    {
	    	// nothing to do. Return the empty indexer.
	    }
	}

	void initKafkaConsumerAndThreadLoop()
	{
		this->kafkaBrokerHostName = this->indexDataContainerConf->getKafkaBrokerHostName();
		this->kafkaBrokerPort = this->indexDataContainerConf->getKafkaBrokerPort();
		this->kafkaConsumerTopicName = this->indexDataContainerConf->getCustomerName();
		this->kafkaConsumerPartitionId = this->indexDataContainerConf->getKafkaConsumerPartitionId();
		this->kafkaConsumerReadBufferInBytes = this->indexDataContainerConf->getWriteReadBufferInBytes();
		this->pingKafkaBrokerEveryNSeconds = this->indexDataContainerConf->getPingKafkaBrokerEveryNSeconds();
		this->produce_info_to_kafka_every_n_seconds = 2;
		this->srch2Consumer = NULL;
		this->srch2Producer = NULL;

		// Run boost::asio io_server thread.run(). Same io_service for multiple workers.
		boost::thread bt(boost::bind(&boost::asio::io_service::run, &this->io_service));

		//Init boost::asio workers
		this->_init_consumer();
		this->_init_producer();

		void *(*time_loop_consumer) (void*);
		void *(*time_loop_producer) (void*);
		
		time_loop_consumer = &Srch2KafkaConsumer::start_time_loop_consumer_thread;
		time_loop_producer = &Srch2KafkaConsumer::start_time_loop_producer_thread;

		std::cout << "Kafka Start Offset:[" << this->offset << "]" << std::endl;

		time_loop_consumer_thread_id = pthread_create(&thread_consumer,
														NULL,
														//&attr,
														time_loop_consumer,
														this);
														
		time_loop_producer_thread_id = pthread_create(&thread_producer,
														NULL,
														//&attr,
														time_loop_producer,
														this);
	}

	Indexer *getIndexer()
	{
		return this->indexer;
	}

	virtual ~Srch2KafkaConsumer()
	{
		if ( this->indexDataContainerConf->getWriteApiType() == srch2http::KAFKAWRITEAPI)
		{
			work_consumer.reset();
			work_producer.reset();
			delete this->srch2Consumer;
			delete this->srch2Producer;
			io_service.stop();
		}
		//pthread_mutex_destroy(&mutex_writer);

		//TODO: Gracefully shutdown the thread
		//pthread_exit(NULL);
	}

	static IndexMetaData *createIndexMetaData(const ConfigManager *indexDataContainerConf);

private:
	kafkaconnect::consumer *srch2Consumer;
	kafkaconnect::producer *srch2Producer;
	
    boost::asio::io_service io_service;
    std::auto_ptr<boost::asio::io_service::work> work_consumer;
    std::auto_ptr<boost::asio::io_service::work> work_producer;

	Indexer *indexer;
	const ConfigManager *indexDataContainerConf;

	std::string kafkaBrokerHostName;
    uint16_t kafkaBrokerPort;
    std::string kafkaConsumerTopicName;
    uint32_t kafkaConsumerPartitionId;
    unsigned pingKafkaBrokerEveryNSeconds;
    unsigned kafkaConsumerReadBufferInBytes;
    unsigned produce_info_to_kafka_every_n_seconds;

    // Offset can be changed only with a lock
    uint64_t offset;

    mutable pthread_mutex_t mutex_writer;
    pthread_t thread_consumer;
    pthread_t thread_producer;
    
    int time_loop_consumer_thread_id;
    int time_loop_producer_thread_id;

    void consumeMessagesBeforeCommit();

   	void _init_consumer();
   	void _init_producer();
   	bool consumeMessages();
   	bool produceHealthMessage();

   	void createAndBootStrapIndexer();

    void consumer_watch_time()
   	{
   		while (1) {
			//pthread_mutex_lock(&mutex_writer);

			this->consumeMessages();

			//pthread_mutex_unlock(&mutex_writer);
   			sleep(this->pingKafkaBrokerEveryNSeconds);
   		}
   		pthread_exit(NULL);
   	}

	void producer_watch_time()
   	{
   		while (1) {
		
			this->produceHealthMessage();

   			sleep(this->produce_info_to_kafka_every_n_seconds);
   		}
   		pthread_exit(NULL);
   	}

    static void *start_time_loop_consumer_thread(void *obj)
	{
		//All we do here is call the watch_time() function
		reinterpret_cast<Srch2KafkaConsumer *>(obj)->consumer_watch_time();
		return NULL;
	}
	
	static void *start_time_loop_producer_thread(void *obj)
	{
		//All we do here is call the watch_time() function
		reinterpret_cast<Srch2KafkaConsumer *>(obj)->producer_watch_time();
		return NULL;
	}
	
	void _executeMessagesOnIndex(const std::vector<std::string> &messages, const uint64_t offset);
	
	
};


}
}


#endif /* __SRCH2_KAFKA_CONSUMER_WRAPPER_H_ */
