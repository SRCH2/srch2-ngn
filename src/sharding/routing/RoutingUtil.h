#ifndef __SHARDING_RM_UTIL_H__
#define __SHARDING_RM_UTIL_H__

#include <instantsearch/Schema.h>
#include <core/util/Assert.h>
#include <sharding/processor/serializables/SerializableInsertUpdateCommandInput.h>
#include <sharding/transport/Message.h>

namespace srch2 {
namespace httpwrapper {

template<typename RequestType > inline
RequestType * decodeInternalMessage(Message * message){
	ASSERT(message->getBodySize() == sizeof(RequestType *));
	void * body = message->getBody();
	RequestType * objectPointer = NULL;
	memcpy(&objectPointer, body, message->getBodySize());
	return objectPointer;
}
template<typename RequestType > inline
RequestType * decodeExternalMessage(Message * message){
	return RequestType::deserialize((void*) message->getBody());
}

inline SerializableInsertUpdateCommandInput * decodeExternalInsertUpdateMessage(Message * message, const srch2::instantsearch::Schema * schema){
	return SerializableInsertUpdateCommandInput::deserialize((void*) message->getBody(),schema);
}

}
}

#endif //__SHARDING_RM_UTIL_H__
