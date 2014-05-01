#ifndef __CORE_UTIL_SERIALIZATION_HELPER_H_
#define __CORE_UTIL_SERIALIZATION_HELPER_H_

#include<string.h>

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace util {

template<class FixedType>
void * serializeFixedTypes(const FixedType & obj, void * buffer){
	memcpy(buffer, (char *)(&obj), sizeof(obj));
	return (char *)buffer + sizeof(obj);
}

template<class FixedType>
void * deserializeFixedTypes(void * buffer, FixedType & obj){
	obj = *((FixedType *)(buffer));
	return (char *)buffer + sizeof(obj);
}

void * serializeString(const string & msg, void * buffer){
	// first serialize size
	unsigned msgSize = msg.size();
	buffer = serializeFixedTypes(msgSize, buffer);
	// now serialize the body
	memcpy(buffer, msg.c_str() , msgSize);
	return (char *)buffer + msgSize;
}

void * deserializeString(void * buffer, string & msg){
	// first deserialize size
	unsigned msgSize = *((unsigned *)buffer);
	buffer = (char *)buffer + sizeof(unsigned);
	// now deserialize body
	char * msgBody = new char[msgSize];
	memcpy(msgBody , buffer, msgSize);
	msg = string(msgBody, msgSize);
	delete msgBody;
	return (char *)buffer + msgSize;
}

template<class FixedType>
void * serializeVectorOfFixedTypes(const vector<FixedType> & vectorObj, void * buffer){

	// first store the size
	buffer = serializeFixedTypes(vectorObj.size() , buffer);
	// and then store the elements
	for(unsigned objIndex = 0 ; objIndex != vectorObj.size() ; objIndex ++){
		buffer = serializeFixedTypes(vectorObj.at(objIndex), buffer);
	}

	return buffer;
}

template<class FixedType>
void * deserializeVectorOfFixedTypes(void * buffer, vector<FixedType> & objVector){

	// first deserialize size of vector
	unsigned sizeOfVector = 0;
	buffer = deserializeFixedTypes(buffer, sizeOfVector);
	// and deserialize objects one by one
	for(unsigned i=0; i < sizeOfVector ; ++i){
		FixedType obj;
		buffer = deserializeFixedTypes(buffer, obj);
		objVector.push_back(obj);
	}

	return buffer;
}

void * serializeVectorOfString(const vector<string> & msgVector, void * buffer){
	// first serialize size
	unsigned vectorSize = msgVector.size();
	buffer = serializeFixedTypes(vectorSize, buffer);
	//now serialize elements
	for(unsigned msgIndex = 0; msgIndex < vectorSize; msgIndex++){
		buffer = serializeString(msgVector.at(msgIndex), buffer);
	}

	return buffer;

}

void * deserializeVectorOfString(void * buffer, vector<string> & msgVector){
	// first deserialize size
	unsigned vectorSize = 0;
	buffer = deserializeFixedTypes(buffer, vectorSize);
	// now deserialize elements
	for(unsigned msgIndex = 0; msgIndex < vectorSize; msgIndex++){
		string msg;
		buffer = deserializeString(buffer, msg);
		msgVector.push_back(msg);
	}
	return buffer;
}


template<class FixedType>
unsigned getNumberOfBytesVectorOfFixedTypes(const vector<FixedType> & vectorObj){
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned); // size of vector
	numberOfBytes += sizeof(FixedType)*vectorObj.size(); // size of elements
	return numberOfBytes;
}

unsigned getNumberOfBytesVectorOfString(const vector<string> & msgVector){
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned); // size of vector
	for(unsigned msgIndex = 0 ; msgIndex < msgVector.size() ; msgIndex++){
		numberOfBytes += sizeof(unsigned) + sizeof(char)*msgVector.at(msgIndex).size();
	}
	return numberOfBytes;
}

}
}

#endif // __SHARDING_UTIL_SERIALIZATION_HELPER_H_
