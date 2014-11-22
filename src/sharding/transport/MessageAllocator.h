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
