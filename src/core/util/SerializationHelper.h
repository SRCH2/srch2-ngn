#ifndef __CORE_UTIL_SERIALIZATION_HELPER_H_
#define __CORE_UTIL_SERIALIZATION_HELPER_H_

#include<string.h>
#include <map>

#include <inttypes.h>


namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace util {
#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

inline uint64_t pack754(long double f, unsigned bits, unsigned expbits)
{
	long double fnorm;
	int shift;
	long long sign, exp, significand;
	unsigned significandbits = bits - expbits - 1; // -1 for sign bit

	if (f == 0.0) return 0; // get this special case out of the way

	// check sign and begin normalization
	if (f < 0) { sign = 1; fnorm = -f; }
	else { sign = 0; fnorm = f; }

	// get the normalized form of f and track the exponent
	shift = 0;
	while(fnorm >= 2.0) { fnorm /= 2.0; shift++; }
	while(fnorm < 1.0) { fnorm *= 2.0; shift--; }
	fnorm = fnorm - 1.0;

	// calculate the binary form (non-float) of the significand data
	significand = fnorm * ((1LL<<significandbits) + 0.5f);

	// get the biased exponent
	exp = shift + ((1<<(expbits-1)) - 1); // shift + bias

	// return the final answer
	return (sign<<(bits-1)) | (exp<<(bits-expbits-1)) | significand;
}

inline long double unpack754(uint64_t i, unsigned bits, unsigned expbits)
{
	long double result;
	long long shift;
	unsigned bias;
	unsigned significandbits = bits - expbits - 1; // -1 for sign bit

	if (i == 0) return 0.0;

	// pull the significand
	result = (i&((1LL<<significandbits)-1)); // mask
	result /= (1LL<<significandbits); // convert back to float
	result += 1.0f; // add the one back on

	// deal with the exponent
	bias = (1<<(expbits-1)) - 1;
	shift = ((i>>significandbits)&((1LL<<expbits)-1)) - bias;
	while(shift > 0) { result *= 2.0; shift--; }
	while(shift < 0) { result /= 2.0; shift++; }

	// sign it
	result *= (i>>(bits-1))&1? -1.0: 1.0;

	return result;
}

template<class FixedType>
inline void * serializeFixedTypes(const FixedType & obj, void * buffer){
	memcpy(buffer, (char *)(&obj), sizeof(obj));
	return ((char *)buffer) + sizeof(obj);
}

inline void * serializeFixedTypes(const double & obj, void * buffer){
	uint64_t packed;
	packed = pack754_64(obj);
	memcpy(buffer, (char *)(&obj), sizeof(packed));
	return ((char *)buffer) + sizeof(uint64_t);
}

inline void * serializeFixedTypes(const float & obj, void * buffer){
	uint32_t packed;
	packed = pack754_32(obj);
	memcpy(buffer, (char *)(&obj), sizeof(packed));
	return ((char *)buffer) + sizeof(uint32_t);
}

template<class FixedType>
inline void * deserializeFixedTypes(void * buffer, FixedType & obj){
	obj = *((FixedType *)(buffer));
	return ((char *)buffer) + sizeof(FixedType);
}

inline void * derializeFixedTypes(void * buffer, double & obj){
	uint64_t unpacked;
	unpacked = *((uint64_t *)(buffer));
	obj = pack754_64(unpacked);
	return ((char *)buffer) + sizeof(uint64_t);
}

inline void * derializeFixedTypes(void * buffer, float & obj){
	uint32_t unpacked;
	unpacked = *((uint32_t *)(buffer));
	obj = pack754_32(unpacked);
	return ((char *)buffer) + sizeof(uint32_t);
}

template<class FixedType>
inline size_t getNumberOfBytesFixedTypes(const FixedType & obj){
	return sizeof(FixedType);
}

inline size_t getNumberOfBytesFixedTypes(const double & obj){
	return sizeof(uint64_t);
}

inline size_t getNumberOfBytesFixedTypes(const float & obj){
	return sizeof(uint32_t);
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
	uint32_t msgSize = msg.size();
	buffer = serializeFixedTypes(msgSize, buffer);
	// now serialize the body
	memcpy(buffer, msg.c_str() , msgSize);
	return ((char *)buffer) + msgSize;
}

inline void * deserializeString(void * buffer, string & msg){
	// first deserialize size
	uint32_t msgSize = *((uint32_t *)buffer);
	uint32_t intVar;
	buffer = ((char *)buffer) + getNumberOfBytesFixedTypes(intVar);
	// now deserialize body
	char * msgBody = new char[msgSize];
	memcpy(msgBody , buffer, msgSize);
	msg = string(msgBody, msgSize);
	delete [] msgBody;
	return ((char *)buffer) + msgSize;
}

inline size_t getNumberOfBytesString(const string & obj){
	return sizeof(uint32_t) + obj.size();
}


template<class FixedType>
inline void * serializeVectorOfFixedTypes(const vector<FixedType> & vectorObj, void * buffer){

	// first store the size
	buffer = serializeFixedTypes(uint32_t(vectorObj.size()) , buffer);
	// and then store the elements
	for(uint32_t objIndex = 0 ; objIndex < vectorObj.size() ; objIndex ++){
		buffer = serializeFixedTypes(vectorObj.at(objIndex), buffer);
	}

	return buffer;
}

template<class FixedType>
inline void * deserializeVectorOfFixedTypes(void * buffer, vector<FixedType> & objVector){

	// first deserialize size of vector
	uint32_t sizeOfVector = 0;
	buffer = deserializeFixedTypes(buffer, sizeOfVector);
	// and deserialize objects one by one
	for(uint32_t i=0; i < sizeOfVector ; ++i){
		FixedType obj;
		buffer = deserializeFixedTypes(buffer, obj);
		objVector.push_back(obj);
	}

	return buffer;
}

inline void * serializeVectorOfString(const vector<string> & msgVector, void * buffer){
	// first serialize size
	uint32_t vectorSize = msgVector.size();
	buffer = serializeFixedTypes(vectorSize, buffer);
	//now serialize elements
	for(uint32_t msgIndex = 0; msgIndex < vectorSize; msgIndex++){
		buffer = serializeString(msgVector.at(msgIndex), buffer);
	}

	return buffer;

}

inline void * deserializeVectorOfString(void * buffer, vector<string> & msgVector){
	// first deserialize size
	uint32_t vectorSize = 0;
	buffer = deserializeFixedTypes(buffer, vectorSize);
	// now deserialize elements
	for(uint32_t msgIndex = 0; msgIndex < vectorSize; msgIndex++){
		string msg;
		buffer = deserializeString(buffer, msg);
		msgVector.push_back(msg);
	}
	return buffer;
}

template<class FixedType>
inline uint32_t getNumberOfBytesVectorOfFixedTypes(const vector<FixedType> & vectorObj){
	uint32_t numberOfBytes = 0;
	numberOfBytes += getNumberOfBytesFixedTypes(numberOfBytes); // size of vector
	FixedType a;
	numberOfBytes += getNumberOfBytesFixedTypes(a)*vectorObj.size(); // size of elements
	return numberOfBytes;
}

template<class DynamicType>
inline void * serializeVectorOfDynamicTypes(const vector<DynamicType> & vectorObj, void * buffer){

	// first store the size
	buffer = serializeFixedTypes(uint32_t(vectorObj.size()) , buffer);
	// and then store the elements
	for(uint32_t objIndex = 0 ; objIndex < vectorObj.size() ; objIndex ++){
		buffer = vectorObj.at(objIndex).serialize(buffer);
	}

	return buffer;
}

template<class DynamicType>
inline void * deserializeVectorOfDynamicTypes(void * buffer, vector<DynamicType> & objVector){

	// first deserialize size of vector
	uint32_t sizeOfVector = 0;
	buffer = deserializeFixedTypes(buffer, sizeOfVector);
	// and deserialize objects one by one
	for(uint32_t i=0; i < sizeOfVector ; ++i){
		DynamicType obj;
		buffer = obj.deserialize(buffer);
		objVector.push_back(obj);
	}

	return buffer;
}

template<class DynamicType>
inline uint32_t getNumberOfBytesVectorOfDynamicTypes(const vector<DynamicType> & vectorObj){
	uint32_t numberOfBytes = 0;
	numberOfBytes += getNumberOfBytesFixedTypes(numberOfBytes); // size of vector
	for(uint32_t i = 0 ; i < vectorObj.size() ; ++i){
		numberOfBytes += vectorObj.at(i).getNumberOfBytes();
	}
	return numberOfBytes;
}



template<class DynamicType, class FixedType>
inline void * serializeMapDynamicToFixed(const map<DynamicType, FixedType> & mapObj, void * buffer){
	buffer = serializeFixedTypes((uint32_t)(mapObj.size()), buffer);
	for(typename map<DynamicType, FixedType>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		buffer = itr->first.serialize(buffer);
		buffer = serializeFixedTypes(itr->second, buffer);
	}
	return buffer;
}

template<class DynamicType, class FixedType>
inline void * deserializeMapDynamicToFixed(void * buffer, map<DynamicType, FixedType> & mapObj){
	uint32_t sizeTemp;
	buffer = deserializeFixedTypes(buffer, sizeTemp);
	for(uint32_t i = 0 ; i < sizeTemp ; ++i){
		DynamicType key;
		FixedType value;
		buffer = key.deserialize(buffer);
		buffer = deserializeFixedTypes(buffer, value);
		mapObj[key] = value;
	}
	return buffer;
}

template<class DynamicType, class FixedType>
inline uint32_t getNumberOfBytesMapDynamicToFixed(const map<DynamicType, FixedType> & mapObj){
	uint32_t numberOfBytes = 0;
	numberOfBytes += getNumberOfBytesFixedTypes(numberOfBytes);
	for(typename map<DynamicType, FixedType>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		numberOfBytes += itr->first.getNumberOfBytes();
		FixedType a;
		numberOfBytes += getNumberOfBytesFixedTypes(a);
	}
	return numberOfBytes;
}


template<class DynamicType, class DynamicType2>
inline void * serializeMapDynamicToDynamic(const map<DynamicType, DynamicType2> & mapObj, void * buffer){
	buffer = serializeFixedTypes((uint32_t)(mapObj.size()), buffer);
	for(typename map<DynamicType, DynamicType2>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		buffer = itr->first.serialize(buffer);
		buffer = itr->second.serialize(buffer);
	}
	return buffer;
}

template<class DynamicType, class DynamicType2>
inline void * deserializeMapDynamicToDynamic(void * buffer, map<DynamicType, DynamicType2> & mapObj){
	uint32_t sizeTemp;
	buffer = deserializeFixedTypes(buffer, sizeTemp);
	for(uint32_t i = 0 ; i < sizeTemp ; ++i){
		DynamicType key;
		DynamicType2 value;
		buffer = key.deserialize(buffer);
		buffer = value.deserialize(buffer);
		mapObj[key] = value;
	}
	return buffer;
}

template<class DynamicType, class DynamicType2>
inline uint32_t getNumberOfBytesMapDynamicToDynamic(const map<DynamicType, DynamicType2> & mapObj){
	uint32_t numberOfBytes = 0;
	numberOfBytes += getNumberOfBytesFixedTypes(numberOfBytes);
	for(typename map<DynamicType, DynamicType2>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		numberOfBytes += itr->first.getNumberOfBytes();
		numberOfBytes += itr->second.getNumberOfBytes();
	}
	return numberOfBytes;
}



template<class FixedType, class DynamicType>
inline void * serializeMapFixedToDynamic(const map<FixedType, DynamicType> & mapObj, void * buffer){
	buffer = serializeFixedTypes((uint32_t)(mapObj.size()), buffer);
	for(typename map<FixedType, DynamicType>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		buffer = serializeFixedTypes(itr->first, buffer);
		buffer = itr->second.serialize(buffer);
	}
	return buffer;
}

template<class FixedType, class DynamicType>
inline void * deserializeMapFixedToDynamic(void * buffer, map<FixedType, DynamicType> & mapObj){
	uint32_t sizeTemp;
	buffer = deserializeFixedTypes(buffer, sizeTemp);
	for(uint32_t i = 0 ; i < sizeTemp ; ++i){
		FixedType key;
		DynamicType value;
		buffer = deserializeFixedTypes(buffer, key);
		buffer = value.deserialize(buffer);
		mapObj[key] = value;
	}
	return buffer;
}

template<class FixedType, class DynamicType>
inline uint32_t getNumberOfBytesMapFixedToDynamic(const map<FixedType, DynamicType> & mapObj){
	uint32_t numberOfBytes = 0;
	numberOfBytes += getNumberOfBytesFixedTypes(numberOfBytes);
	for(typename map<FixedType, DynamicType>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		FixedType a;
		numberOfBytes += getNumberOfBytesFixedTypes(a);
		numberOfBytes += itr->second.getNumberOfBytes();
	}
	return numberOfBytes;
}



template<class FixedType, class FixedType2>
inline void * serializeMapFixedToFixed(const map<FixedType, FixedType2> & mapObj, void * buffer){
	buffer = serializeFixedTypes((uint32_t)(mapObj.size()), buffer);
	for(typename map<FixedType, FixedType2>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		buffer = serializeFixedTypes(itr->first, buffer);
		buffer = serializeFixedTypes(itr->second, buffer);
	}
	return buffer;
}

template<class FixedType, class FixedType2>
inline void * deserializeMapFixedToFixed(void * buffer, map<FixedType, FixedType2> & mapObj){
	uint32_t sizeTemp;
	buffer = deserializeFixedTypes(buffer, sizeTemp);
	for(uint32_t i = 0 ; i < sizeTemp ; ++i){
		FixedType key;
		FixedType2 value;
		buffer = deserializeFixedTypes(key, buffer);
		buffer = deserializeFixedTypes(value, buffer);
		mapObj[key] = value;
	}
	return buffer;
}

template<class FixedType, class FixedType2>
inline uint32_t getNumberOfBytesMapFixedToFixed(const map<FixedType, FixedType2> & mapObj){
	uint32_t numberOfBytes = 0;
	numberOfBytes += getNumberOfBytesFixedTypes(numberOfBytes);
	for(typename map<FixedType, FixedType2>::const_iterator itr = mapObj.begin();
			itr != mapObj.end(); ++itr){
		FixedType a;
		FixedType2 b;
		numberOfBytes += getNumberOfBytesFixedTypes(a);
		numberOfBytes += getNumberOfBytesFixedTypes(b);
	}
	return numberOfBytes;
}

template<class DynamicType>
inline void * serializeVectorOfDynamicTypePointers(const vector<DynamicType *> & vectorObj, void * buffer){

	// first store the size
	buffer = serializeFixedTypes(uint32_t(vectorObj.size()) , buffer);
	// and then store the elements
	for(uint32_t objIndex = 0 ; objIndex < vectorObj.size() ; objIndex ++){
		buffer = vectorObj.at(objIndex)->serialize(buffer);
	}

	return buffer;
}

template<class DynamicType>
inline void * deserializeVectorOfDynamicTypePointers(void * buffer, vector<DynamicType *> & objVector){

	// first deserialize size of vector
	uint32_t sizeOfVector = 0;
	buffer = deserializeFixedTypes(buffer, sizeOfVector);
	// and deserialize objects one by one
	for(uint32_t i=0; i < sizeOfVector ; ++i){
		DynamicType * obj = new DynamicType();
		buffer = obj->deserialize(buffer);
		objVector.push_back(obj);
	}

	return buffer;
}

template<class DynamicType>
inline uint32_t getNumberOfBytesVectorOfDynamicTypePointers(const vector<DynamicType *> & vectorObj){
	uint32_t numberOfBytes = 0;
	numberOfBytes += getNumberOfBytesFixedTypes(numberOfBytes); // size of vector
	for(uint32_t i = 0 ; i < vectorObj.size() ; ++i){
		numberOfBytes += vectorObj.at(i)->getNumberOfBytes();
	}
	return numberOfBytes;
}


inline uint32_t getNumberOfBytesVectorOfString(const vector<string> & msgVector){
	uint32_t numberOfBytes = 0;
	numberOfBytes += getNumberOfBytesFixedTypes(numberOfBytes); // size of vector
	for(uint32_t msgIndex = 0 ; msgIndex < msgVector.size() ; msgIndex++){
		char a;
		numberOfBytes += getNumberOfBytesString(msgVector.at(msgIndex));
	}
	return numberOfBytes;
}

}
}

#endif // __SHARDING_UTIL_SERIALIZATION_HELPER_H_
