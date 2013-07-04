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

#ifndef _KAFKA_ENCODER_CONSUMER_HELPER_HPP_
#define _KAFKA_ENCODER_CONSUMER_HELPER_HPP_

#include <ostream>
#include <iostream>
#include <istream>
#include <string>

#include <arpa/inet.h>
#include <boost/crc.hpp>

#include <stdint.h>

//namespace kafkaconnect {
//namespace test { class encoder_consumer_helper; }

const uint16_t kafka_format_version = 1; // FETCh

const uint8_t message_format_magic_number = 0;
const uint8_t message_format_extra_data_size = 1 + 4;
const uint8_t message_format_header_size = message_format_extra_data_size + 4;

//const uint32_t max_size = 1024 * 1024;

class encoder_consumer_helper
{
	public:

		static int is_big_endian(void)
		{
			union {
				uint32_t i;
				char c[4];
			} bint = {0x01020304};

			return bint.c[0] == 1;
		}

		//soruce: http://stackoverflow.com/questions/3022552/is-there-any-standard-htonl-like-function-for-64-bits-integers-in-c
		static uint64_t htonll(uint64_t value)
		{
			// The answer is 42
			static const int num = 42;

			// Check the endianness
			if (is_big_endian() == 0)
			{
				// little endian
				const uint32_t high_part = htonl(static_cast<uint32_t>(value >> 32));
				const uint32_t low_part = htonl(static_cast<uint32_t>(value & 0xFFFFFFFFLL));

				return (static_cast<uint64_t>(low_part) << 32) | high_part;
			} else
			{
				// big endian
				return value;
			}
		}

		static void encode_consumer_request_size(std::ostream& stream, const std::string& topic)
		{

		/*
			source: https://github.com/dsully/pykafka/blob/master/kafka/consumer.py
			# REQUEST TYPE ID + TOPIC LENGTH + TOPIC + PARTITION + OFFSET + MAX SIZE
			def request_size(self):
				return 2 + 2 + len(self.topic) + 4 + 8 + 4

			def encode_request_size(self):
				return struct.pack('>i', self.request_size())

		*/
			// Packet format is ... packet size (4 bytes)
			encoder_consumer_helper::raw(stream, htonl(2 + 2 + topic.size() + 4 + 8 + 4));
		}


		static void encode_consumer_request(std::ostream& stream, const std::string& topic, const uint32_t partition, const uint64_t offset, const uint32_t max_size)
		{
		/*
			source: https://github.com/dsully/pykafka/blob/master/kafka/consumer.py
			def encode_request(self):
				length = len(self.topic)
				return struct.pack('>HH%dsiQi' % length, self.request_type, length, self.topic, self.partition, self.offset, self.max_size)*/

			// ... topic string size (2 bytes) & topic string
			//encoder_consumer_helper::raw(stream, htonl(2 + 2 + topic.size() + 4 + 8 + 4));

			// ... kafka format number (2 bytes)
			encoder_consumer_helper::raw(stream, htons(kafka_format_version));

			encoder_consumer_helper::raw(stream, htons(topic.size()));

			stream << topic;

			// ... partition (4 bytes)
			encoder_consumer_helper::raw(stream, htonl(partition));

			// ... offet (8 bytes)
			//int64_t __builtin_bswap64 (int64_t x)
			//uint64_t offset = 0;
			//encoder_consumer_helper::raw(stream, kafkaconnect::htonll(offset));
			encoder_consumer_helper::raw(stream, htonll(offset));

			// ... max_size (4 bytes)
			encoder_consumer_helper::raw(stream, htonl(max_size));
		}

		template <typename List>
		static uint32_t decode_consumer(char* buffer_read, const uint32_t total_bytes_to_process, List& messages)
		{
			messages.clear();
			uint32_t processed_bytes_cursor = 0;

			// source: https://cwiki.apache.org/confluence/display/KAFKA/Writing+a+Driver+for+Kafka
			uint16_t error_code;
			std::memcpy(&error_code, buffer_read + processed_bytes_cursor, sizeof(error_code));
			processed_bytes_cursor += sizeof(error_code);

			while (processed_bytes_cursor < total_bytes_to_process)
			{
				/*
				A message. The format of an N byte message is the following:
				 4 message size <-- already parsed in caller.
				 1 byte "magic" identifier to allow format changes
				 4 byte CRC32 of the payload
				 N - 5 byte payload
				*/

				uint32_t message_size;
				std::memcpy(&message_size, buffer_read + processed_bytes_cursor, sizeof(message_size));
				processed_bytes_cursor += sizeof(message_size);
				message_size =  ntohl(message_size);

				if ( (processed_bytes_cursor + message_size) > total_bytes_to_process)
				{
					processed_bytes_cursor -= 4;
					break;
				}

				// ... magic number (1 byte)
				uint8_t message_format_magic_number;
				std::memcpy(&message_format_magic_number, buffer_read + processed_bytes_cursor, sizeof(message_format_magic_number));
				processed_bytes_cursor += sizeof(message_format_magic_number);
				message_format_magic_number =  ntohl(message_format_magic_number);

				// ... string crc32 (4 bytes)
				uint32_t checksum;
				std::memcpy(&checksum, buffer_read + processed_bytes_cursor, sizeof(checksum));
				processed_bytes_cursor += sizeof(checksum);
				checksum =  ntohl(checksum);

				// source: https://cwiki.apache.org/confluence/display/KAFKA/Writing+a+Driver+for+Kafka
				//Kafka Edge case behavior:
				/*If you ask the broker for up to 300K worth of messages from a given topic and partition,
				 it will send you the appropriate headers followed by a 300K chunk worth of the message log.
				 If 300K ends in the middle of a message, you get half a message at the end. If it ends halfway
				 through a message header, you get a broken header. This is not an error, this is Kafka pushing
				 complexity outward to the driver to make the broker simple and fast.
				 Kafka stores its messages in log files of a configurable size (512MB by default) called segments.
				 A fetch of messages will not cross the segment boundary to read from multiple files. So if you ask
				 for a fetch of 300K's worth of messages and the offset you give is such that there's only one message
				 at the end of that segment file, then you will get just one message back. The next time you call fetch
				 with the following offset, you'll get a full set of messages from the next segment file. Basically,
				 don't make any assumptions about how many messages are remaining from how many you got in the last fetch.*/

				// ... message string bytes
				uint32_t length = message_size - 1 - 4 + 1; // +1 for null terminal character
				char *msg = new char[length];
				std::memcpy(msg, buffer_read + processed_bytes_cursor, length - 1);
				processed_bytes_cursor += length - 1;
				msg[length - 1] = '\0';

				std::string message(msg);

				delete msg;

				// check checksum
				boost::crc_32_type result;
				result.process_bytes(message.c_str(), message.length());
				if (result.checksum() == checksum)
					messages.push_back(message);
			}
			return processed_bytes_cursor - 2;
		}

		template <typename Data>
		static std::ostream& raw(std::ostream& stream, const Data& data)
		{
			stream.write(reinterpret_cast<const char*>(&data), sizeof(Data));
			return stream;
		}
};

//}

#endif /* _KAFKA_ENCODER_CONSUMER_HELPER_HPP_ */
