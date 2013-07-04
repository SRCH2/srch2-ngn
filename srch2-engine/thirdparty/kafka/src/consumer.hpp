/**
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  Created on: 25 Jan 2012
 *  Author: Vijay Rajakumar (@rvijax)
 */

#ifndef KAFKA_CONSUMER_HPP_
#define KAFKA_CONSUMER_HPP_

#include <string>
#include <vector>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

#include <stdint.h>
#include <fstream>
#include <sstream>

#include "encoder_consumer_helper.hpp"

//namespace kafkaconnect {

//const uint32_t use_random_partition = 0xFFFFFFFF;

class consumer
{
public:
	typedef boost::function<void(const boost::system::error_code&)> error_handler_function;

	consumer(boost::asio::io_service& io_service, const std::string& hostname, const uint16_t port, const error_handler_function& error_handler = error_handler_function());
	~consumer();

	void connect(const std::string& hostname, const uint16_t port);
	void connect(const std::string& hostname, const std::string& servicename);

	void close();
	bool is_connected() const;
	
	// TODO: replace this with a sending of the buffered data so encode is called prior to send this will allow for decoupling from the encoder
	template <typename List>
	bool consume ( List& messages,
			const std::string& topic, const uint32_t partition, const uint32_t max_buffer_size,
			const uint64_t startOffset, uint64_t &newOffset )
	{
		// TODO: make this more efficient with memory allocations.
		boost::asio::streambuf* buffer_write_consumer_request_size = new boost::asio::streambuf();
		std::ostream stream_write_consumer_request_size(buffer_write_consumer_request_size);

		// send consume request size
		encoder_consumer_helper::encode_consumer_request_size(stream_write_consumer_request_size, topic);

		boost::asio::write(	_socket, *buffer_write_consumer_request_size,
			boost::asio::transfer_all());

		// send consume request
		boost::asio::streambuf* buffer_write_consumer_request = new boost::asio::streambuf();
		std::ostream stream_write_consumer_request(buffer_write_consumer_request);

		encoder_consumer_helper::encode_consumer_request(stream_write_consumer_request, topic, partition, startOffset, max_buffer_size);

		boost::asio::write( _socket, *buffer_write_consumer_request,
			boost::asio::transfer_all());

		delete buffer_write_consumer_request_size;
		delete buffer_write_consumer_request;

		// start read:
		// Read "N" - size of response
		uint32_t header;
		boost::asio::read(
			_socket,
			boost::asio::buffer( &header, sizeof(header) )
		);
		header = ntohl(header);
		//uint32_t processed_bytes = sizeof(header);
		uint32_t processed_bytes = 0;

		// Read the response
		size_t bytes_to_read = header;
		char *buffer_read = new char[bytes_to_read];
		size_t body_read = boost::asio::read(_socket, boost::asio::buffer( buffer_read, bytes_to_read) );

		processed_bytes += encoder_consumer_helper::decode_consumer(buffer_read, body_read, messages);
		newOffset = startOffset + processed_bytes;
		delete 	buffer_read;

		if ( header == 2 ) // no message received
		{
			newOffset = startOffset;
			return false; // continueIteration
		}
		else
		{
			return true; // continueIteration
		}
	}

private:
	bool _connected;
	boost::asio::ip::tcp::resolver _resolver;
	boost::asio::ip::tcp::socket _socket;
	error_handler_function _error_handler;

	void handle_resolve(const boost::system::error_code& error_code, boost::asio::ip::tcp::resolver::iterator endpoints);
	void handle_connect(const boost::system::error_code& error_code, boost::asio::ip::tcp::resolver::iterator endpoints);

	/* Fail Fast Error Handler Braindump
	 *
	 * If an error handler is not provided in the constructor then the default response is to throw
	 * back the boost error_code from asio as a boost system_error exception.
	 *
	 * Most likely this will cause whatever thread you have processing boost io to terminate unless caught.
	 * This is great on debug systems or anything where you use io polling to process any outstanding io,
	 * however if your io thread is seperate and not monitored it is recommended to pass a handler to
	 * the constructor.
	 */
	inline void fail_fast_error_handler(const boost::system::error_code& error_code)
	{
		if(_error_handler.empty()) { throw boost::system::system_error(error_code); }
		else { _error_handler(error_code); }
	}
};

//}

#endif /* KAFKA_CONSUMER_HPP_ */
