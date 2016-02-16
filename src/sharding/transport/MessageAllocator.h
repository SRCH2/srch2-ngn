/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __Message_Allocator_H__
#define __Message_Allocator_H__

#include <memory>
#include "Message.h"

namespace srch2 {
namespace httpwrapper {

class MessageAllocator : public std::allocator<char> {
public:

	/*
	 * This function allocates a message including the header and the body.
	 * The size of header is always sizeof(Message) and bodySize is the size of body
	 * to be used.
	 * This function returns the pointer to the beginning of body.
	 */
	void* allocateMessageReturnBody(size_type bodySize) {
		// allocate full message
		char *msgCharBuffer = (char*) allocator<char>::allocate(bodySize + sizeof(Message));
		// initalize all bits of message to zero
		memset(msgCharBuffer, 0, sizeof(Message) + bodySize);
		// set the body size
		Message * newMsg = new (msgCharBuffer) Message;
		newMsg->setBodySize(bodySize);
		// return the pointer to the beginning of body
		return Message::getBodyPointerFromMessagePointer(newMsg);
	}

	void * allocateByteArray(size_type bodySize){
		 return allocator<char>::allocate(bodySize);
	}

	void deallocateByreArray(void * buffer, size_type bufferSize){
		 allocator<char>::deallocate((char *)(buffer), bufferSize );
	}

	/*
	 * this function receives the pointer to the body of a message and deallocates
	 * this message completely.
	 */
	void deallocateByBodyPointer(pointer bodyPointer, size_type bodySize) {
		allocator<char>::deallocate((char *)(Message::getMessagePointerFromBodyPointer(bodyPointer)), bodySize + sizeof(Message));
	}

	void deallocateBody(pointer bodyPointer, size_type bodySize){
		allocator<char>::deallocate((char *)(bodyPointer), bodySize);
	}

	/*
	 * This function allocates a message with bodySize bytes for body.
	 */
	Message* allocateMessage(size_type bodySize) {
		void * bodyPointer = allocateMessageReturnBody(bodySize);
		return Message::getMessagePointerFromBodyPointer(bodyPointer);
	}
	/*
	 * This function deallocates the message by receiving the message pointer
	 */
	void deallocateByMessagePointer(Message *msg) {
		deallocateByBodyPointer(Message::getBodyPointerFromMessagePointer(msg), msg->getBodySize());
	}
};

}}

#endif /* __MESSAGE_ALLOCATOR_H__ */
