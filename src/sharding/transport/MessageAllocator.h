#ifndef __Message_Allocator_H__
#define __Message_Allocator_H__

#include <memory>
#include "Message.h"

namespace srch2 {
namespace httpwrapper {

class MessageAllocator : public std::allocator<char> {
public:

	/*
	 * This function allocates a message including the header and the body
	 * the size of header is always sizeof(Message) and bodySize is the size of body
	 * to be used.
	 * This function returns the pointer to the beginning of body.
	 */
	void* allocateMessageReturnBody(size_type bodySize) {
		// allocate full message
		Message *msg = (Message*) allocator<char>::allocate(bodySize + sizeof(Message));
		// initalize all bits of message to zero
		memset(msg, 0, bodySize + sizeof(Message));
		// set the body size
		msg->setBodySize(bodySize);
		// return the pointer to the beginning of body
		return msg->getBody();
	}

	/*
	 * this function receives the pointer to the body of a message and deallocates
	 * this message completely.
	 */
	void deallocateByBodyPointer(pointer bodyPointer, size_type bodySize) {
		allocator<char>::deallocate(bodyPointer - sizeof(Message), bodySize + sizeof(Message));
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
		deallocateByBodyPointer(msg->getBody(), msg->getBodySize());
	}
};

}}

#endif /* __MESSAGE_ALLOCATOR_H__ */
;
