#ifndef __CORE_UTIL_SERIALIZATION_HELPER_H_
#define __CORE_UTIL_SERIALIZATION_HELPER_H_

#include<string.h>
#include <map>

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace util {

template<class FixedType>
inline void * serializeFixedTypes(const FixedType & obj, void * buffer){
	memcpy(buffer, (char *)(&obj), sizeof(obj));
	return ((char *)buffer) + sizeof(obj);
}

template<class FixedType>
inline void * deserializeFixedTypes(void * buffer, FixedType & obj){
	obj = *((FixedType *)(buffer));
	return ((char *)buffer) + sizeof(FixedType);
}

#ifdef ANDROID
// This is template specialization of double. On android devices, dereferencing double pointer
// generates "bus error" ( similar to segmentation fault).
template<>
inline void * deserializeFixedTypes<double>(void * buffer, double & obj){
	memcpy((char *)(&obj), buffer, sizeof(double));
	return ((char *)buffer) + sizeof(double);
}
#endif

inline void * serializeString(const string & msg, void * buffer){
	// first serialize size
	unsigned msgSize = msg.size();
	buffer = serializeFixedTypes(msgSize, buffer);
	// now serialize the body
	memcpy(buffer, msg.c_str() , msgSize);
	return ((char *)buffer) + msgSize;
}

inline void * deserializeString(void * buffer, string & msg){
	// first deserialize size
	unsigned msgSize = *((unsigned *)buffer);
	buffer = ((char *)buffer) + sizeof(unsigned);
	// now deserialize body
	char * msgBody = new char[msgSize];
	memcpy(msgBody , buffer, msgSize);
	msg = string(msgBody, msgSize);
	delete msgBody;
	return ((char *)buffer) + msgSize;
}

template<class FixedType>
inline void * serializeVectorOfFixedTypes(const vector<FixedType> & vectorObj, void * buffer){

	// first store the size
	buffer = serializeFixedTypes(unsigned(vectorObj.size()) , buffer);
	// and then store the elements
	for(unsigned objIndex = 0 ; objIndex < vectorObj.size() ; objIndex ++){
		buffer = serializeFixedTypes(vectorObj.at(objIndex), buffer);
	}

	return buffer;
}

template<class FixedType>
inline void * deserializeVectorOfFixedTypes(void * buffer, vector<FixedType> & objVector){

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

inline void * serializeVectorOfString(const vector<string> & msgVector, void * buffer){
	// first serialize size
	unsigned vectorSize = msgVector.size();
	buffer = serializeFixedTypes(vectorSize, buffer);
	//now serialize elements
	for(unsigned msgIndex = 0; msgIndex < vectorSize; msgIndex++){
		buffer = serializeString(msgVector.at(msgIndex), buffer);
	}

	return buffer;

}

inline void * deserializeVectorOfString(void * buffer, vector<string> & msgVector){
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
inline unsigned getNumberOfBytesVectorOfFixedTypes(const vector<FixedType> & vectorObj){
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned); // size of vector
	numberOfBytes += sizeof(FixedType)*vectorObj.size(); // size of elements
	return numberOfBytes;
}

template<class DynamicType>
inline void * serializeVectorOfDynamicTypes(const vector<DynamicType> & vectorObj, void * buffer){

	// first store the size
	buffer = serializeFixedTypes(unsigned(vectorObj.size()) , buffer);
	// and then store the elements
	for(unsigned objIndex = 0 ; objIndex < vectorObj.size() ; objIndex ++){
		buffer = vectorObj.at(objIndex).serialize(buffer);
	}

	return buffer;
}

template<class DynamicType>
inline void * deserializeVectorOfDynamicTypes(void * buffer, vector<DynamicType> & objVector){

	// first deserialize size of vector
	unsigned sizeOfVector = 0;
	buffer = deserializeFixedTypes(buffer, sizeOfVector);
	// and deserialize objects one by one
	for(unsigned i=0; i < sizeOfVector ; ++i){
		DynamicType obj;
		buffer = obj.deserialize(buffer);
		objVector.push_back(obj);
	}

	return buffer;
}

template<class DynamicType>
inline unsigned getNumberOfBytesVectorOfDynamicTypes(const vector<DynamicType> & vectorObj){
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned); // size of vector
	for(unsigned i = 0 ; i < vectorObj.size() ; ++i){
		numberOfBytes += vectorObj.at(i).getNumberOfBytes();
	}
	return numberOfBytes;
}



template<class DynamicType, class FixedType>
inline void * serializeMapDynamicToFixed(const map<DynamicType, FixedType> & mapObj, void * buffer){
	buffer = serializeFixedTypes((unsigned)(mapObj.size()), buffer);
	for(typename map<DynamicType, FixedType>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		buffer = itr->first.serialize(buffer);
		buffer = serializeFixedTypes(itr->second, buffer);
	}
	return buffer;
}

template<class DynamicType, class FixedType>
inline void * deserializeMapDynamicToFixed(void * buffer, map<DynamicType, FixedType> & mapObj){
	unsigned sizeTemp;
	buffer = deserializeFixedTypes(buffer, sizeTemp);
	for(unsigned i = 0 ; i < sizeTemp ; ++i){
		DynamicType key;
		FixedType value;
		buffer = key.deserialize(buffer);
		buffer = deserializeFixedTypes(buffer, value);
		mapObj[key] = value;
	}
	return buffer;
}

template<class DynamicType, class FixedType>
inline unsigned getNumberOfBytesMapDynamicToFixed(const map<DynamicType, FixedType> & mapObj){
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned);
	for(typename map<DynamicType, FixedType>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		numberOfBytes += itr->first.getNumberOfBytes();
		numberOfBytes += sizeof(FixedType);
	}
	return numberOfBytes;
}


template<class DynamicType, class DynamicType2>
inline void * serializeMapDynamicToDynamic(const map<DynamicType, DynamicType2> & mapObj, void * buffer){
	buffer = serializeFixedTypes((unsigned)(mapObj.size()), buffer);
	for(typename map<DynamicType, DynamicType2>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		buffer = itr->first.serialize(buffer);
		buffer = itr->second.serialize(buffer);
	}
	return buffer;
}

template<class DynamicType, class DynamicType2>
inline void * deserializeMapDynamicToDynamic(void * buffer, map<DynamicType, DynamicType2> & mapObj){
	unsigned sizeTemp;
	buffer = deserializeFixedTypes(buffer, sizeTemp);
	for(unsigned i = 0 ; i < sizeTemp ; ++i){
		DynamicType key;
		DynamicType2 value;
		buffer = key.deserialize(buffer);
		buffer = value.deserialize(buffer);
		mapObj[key] = value;
	}
	return buffer;
}

template<class DynamicType, class DynamicType2>
inline unsigned getNumberOfBytesMapDynamicToDynamic(const map<DynamicType, DynamicType2> & mapObj){
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned);
	for(typename map<DynamicType, DynamicType2>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		numberOfBytes += itr->first.getNumberOfBytes();
		numberOfBytes += itr->second.getNumberOfBytes();
	}
	return numberOfBytes;
}



template<class FixedType, class DynamicType>
inline void * serializeMapFixedToDynamic(const map<FixedType, DynamicType> & mapObj, void * buffer){
	buffer = serializeFixedTypes((unsigned)(mapObj.size()), buffer);
	for(typename map<FixedType, DynamicType>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		buffer = serializeFixedTypes(itr->first, buffer);
		buffer = itr->second.serialize(buffer);
	}
	return buffer;
}

template<class FixedType, class DynamicType>
inline void * deserializeMapFixedToDynamic(void * buffer, map<FixedType, DynamicType> & mapObj){
	unsigned sizeTemp;
	buffer = deserializeFixedTypes(buffer, sizeTemp);
	for(unsigned i = 0 ; i < sizeTemp ; ++i){
		FixedType key;
		DynamicType value;
		buffer = deserializeFixedTypes(buffer, key);
		buffer = value.deserialize(buffer);
		mapObj[key] = value;
	}
	return buffer;
}

template<class FixedType, class DynamicType>
inline unsigned getNumberOfBytesMapFixedToDynamic(const map<FixedType, DynamicType> & mapObj){
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned);
	for(typename map<FixedType, DynamicType>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		numberOfBytes += sizeof(FixedType);
		numberOfBytes += itr->second.getNumberOfBytes();
	}
	return numberOfBytes;
}



template<class FixedType, class FixedType2>
inline void * serializeMapFixedToFixed(const map<FixedType, FixedType2> & mapObj, void * buffer){
	buffer = serializeFixedTypes((unsigned)(mapObj.size()), buffer);
	for(typename map<FixedType, FixedType2>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		buffer = serializeFixedTypes(itr->first, buffer);
		buffer = serializeFixedTypes(itr->second, buffer);
	}
	return buffer;
}

template<class FixedType, class FixedType2>
inline void * deserializeMapFixedToFixed(void * buffer, map<FixedType, FixedType2> & mapObj){
	unsigned sizeTemp;
	buffer = deserializeFixedTypes(buffer, sizeTemp);
	for(unsigned i = 0 ; i < sizeTemp ; ++i){
		FixedType key;
		FixedType2 value;
		buffer = deserializeFixedTypes(key, buffer);
		buffer = deserializeFixedTypes(value, buffer);
		mapObj[key] = value;
	}
	return buffer;
}

template<class FixedType, class FixedType2>
inline unsigned getNumberOfBytesMapFixedToFixed(const map<FixedType, FixedType2> & mapObj){
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned);
	for(typename map<FixedType, FixedType2>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		numberOfBytes += sizeof(FixedType);
		numberOfBytes += sizeof(FixedType2);
	}
	return numberOfBytes;
}

template<class DynamicType>
inline void * serializeVectorOfDynamicTypePointers(const vector<DynamicType *> & vectorObj, void * buffer){

	// first store the size
	buffer = serializeFixedTypes(unsigned(vectorObj.size()) , buffer);
	// and then store the elements
	for(unsigned objIndex = 0 ; objIndex < vectorObj.size() ; objIndex ++){
		buffer = vectorObj.at(objIndex)->serialize(buffer);
	}

	return buffer;
}

template<class DynamicType>
inline void * deserializeVectorOfDynamicTypePointers(void * buffer, vector<DynamicType *> & objVector){

	// first deserialize size of vector
	unsigned sizeOfVector = 0;
	buffer = deserializeFixedTypes(buffer, sizeOfVector);
	// and deserialize objects one by one
	for(unsigned i=0; i < sizeOfVector ; ++i){
		DynamicType * obj = new DynamicType();
		buffer = obj->deserialize(buffer);
		objVector.push_back(obj);
	}

	return buffer;
}

template<class DynamicType>
inline unsigned getNumberOfBytesVectorOfDynamicTypePointers(const vector<DynamicType *> & vectorObj){
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned); // size of vector
	for(unsigned i = 0 ; i < vectorObj.size() ; ++i){
		numberOfBytes += vectorObj.at(i)->getNumberOfBytes();
	}
	return numberOfBytes;
}


inline unsigned getNumberOfBytesVectorOfString(const vector<string> & msgVector){
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
