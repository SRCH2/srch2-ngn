#include <iostream>
#include <stdlib.h>

#include "thirdparty/kafka/consumer/consumer.hpp"
#include "thirdparty/kafka/producer/producer.hpp"

using namespace std;

int main(int argc, char** argv)
{
	kafkaconnect::consumer *srch2Consumer;
	boost::asio::io_service io_service;
    std::auto_ptr<boost::asio::io_service::work> work_consumer;
        
	work_consumer.reset(new boost::asio::io_service::work(io_service));
	srch2Consumer = new kafkaconnect::consumer(io_service, "127.0.0.1", 9092);
	boost::thread bt(boost::bind(&boost::asio::io_service::run, &io_service));
    
    uint64_t offset = 0;
    bool continueIteration = true;
    do
	{
		sleep(1);
		if (srch2Consumer->is_connected())
		{
			std::vector<std::string> messages;
			uint64_t newOffset = offset;		

			continueIteration =
					srch2Consumer->consume(messages,
										"pinkf",	0, 50000000,
										offset, newOffset);

			//std::cout << kafkaConsumerTopicName << ":" << kafkaConsumerPartitionId  << ":" << kafkaConsumerReadBufferInBytes 
			std::cout		<< "|num.messages:[" << messages.size() << "]oldoffset:[" << offset << "]newoffset[" << newOffset << "]" << std::endl;

			offset = newOffset;
			
			for (std::vector<std::string>::const_iterator iter = messages.begin();
						iter != messages.end();
						++iter)
			{
				std::cout << "[" << *iter<< "]" << std::endl;
			}
			//this->_executeMessagesOnIndex(messages, this->offset);
		}
		else
		{
			std::cout << "conn lost" << std::endl;
		}
	} while(continueIteration);
	
	delete srch2Consumer;
	
	return 0;	
}
