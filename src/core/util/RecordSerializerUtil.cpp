/*
 * RecordSerializerUtil.cpp
 *
 *  Created on: Feb 28, 2014
 *      Author: srch2
 */

#include "RecordSerializerUtil.h"
#include <set>
#include <sstream>
#include <algorithm>
#include "thirdparty/snappy-1.0.4/snappy.h"
#include "util/DateAndTimeHandler.h"

using namespace srch2::instantsearch;

namespace srch2 {
namespace util {

void RecordSerializerUtil::populateStoredSchema(Schema* storedSchema, const Schema *schema) {

	const string* primaryKey = schema->getPrimaryKey();
	storedSchema->setPrimaryKey(*primaryKey);
	bool pk_found = false;
	std::set<string> visitedAttr;
	std::map<std::string, unsigned>::const_iterator searchableAttributeIter =
			schema->getSearchableAttribute().begin();
	for ( ; searchableAttributeIter != schema->getSearchableAttribute().end();
			searchableAttributeIter++)
	{
		bool isMultiValued = schema->isSearchableAttributeMultiValued(searchableAttributeIter->second);
		bool isHighLight = schema->isHighlightEnabled(searchableAttributeIter->second);
		storedSchema->setSearchableAttribute(searchableAttributeIter->first, 1, isMultiValued, isHighLight);
		visitedAttr.insert(searchableAttributeIter->first);
	}

	map<string, unsigned>::const_iterator refiningAttributeIter =
			schema->getRefiningAttributes()->begin();
	for ( ; refiningAttributeIter != schema->getRefiningAttributes()->end();
			refiningAttributeIter++)
	{
		if (visitedAttr.count(refiningAttributeIter->first) > 0){
			continue;
		}
		bool isMultiValued = schema->isRefiningAttributeMultiValued(refiningAttributeIter->second);
		if (isMultiValued) {
			storedSchema->setSearchableAttribute(refiningAttributeIter->first,
								1, true, false);
			continue;
		}
		srch2is::FilterType type = schema->getTypeOfRefiningAttribute(refiningAttributeIter->second);
		switch (type) {
		case srch2is::ATTRIBUTE_TYPE_UNSIGNED:
		case srch2is::ATTRIBUTE_TYPE_FLOAT:
			storedSchema->setRefiningAttribute(refiningAttributeIter->first,
					type, *schema->getDefaultValueOfRefiningAttribute(refiningAttributeIter->second),
					false);
			break;
		default:
			storedSchema->setSearchableAttribute(refiningAttributeIter->first,
					1, false, false);
		}
	}
}

void RecordSerializerUtil::convertCompactToJSONString(Schema * storedAttrSchema,
		StoredRecordBuffer buffer, const string& externalRecordId, string& jsonBuffer) {
	convertCompactToJSONString(storedAttrSchema, buffer, externalRecordId, jsonBuffer, NULL);
}
void RecordSerializerUtil::convertCompactToJSONString(Schema * storedAttrSchema, StoredRecordBuffer buffer,
		const string& externalRecordId, string& jsonBuffer, const vector<string>* attrToReturn) {

		RecordSerializer compactRecDeserializer = RecordSerializer(*storedAttrSchema);
		std::map<std::string, unsigned>::const_iterator iter =
				storedAttrSchema->getSearchableAttribute().begin();
		jsonBuffer.reserve(1.25 * buffer.length);
		jsonBuffer.append("{") ;
		jsonBuffer+='"'; jsonBuffer+=*(storedAttrSchema->getPrimaryKey()); jsonBuffer+='"';
		jsonBuffer+=":\""; jsonBuffer+=externalRecordId; jsonBuffer+="\",";
		for ( ; iter != storedAttrSchema->getSearchableAttribute().end(); iter++)
		{
			if (attrToReturn &&
			    std::find(attrToReturn->begin(), attrToReturn->end(), iter->first) == attrToReturn->end()) {
				continue;
			}
			// check if it is also a primary key, skip if true because PK is emitted already.
			if (*(storedAttrSchema->getPrimaryKey()) == iter->first) {
				continue;
			}
			unsigned id = storedAttrSchema->getSearchableAttributeId(iter->first);
			unsigned lenOffset = compactRecDeserializer.getSearchableOffset(id);
			const char *attrdata = buffer.start.get() + *((unsigned *)(buffer.start.get() + lenOffset));
			unsigned len = *(((unsigned *)(buffer.start.get() + lenOffset)) + 1) -
					*((unsigned *)(buffer.start.get() + lenOffset));

			std::string uncompressedInMemoryRecordString;
			snappy::Uncompress(attrdata,len, &uncompressedInMemoryRecordString);
			jsonBuffer+='"'; jsonBuffer+=iter->first; jsonBuffer+='"';
			jsonBuffer+=':';
			if (storedAttrSchema->isSearchableAttributeMultiValued(id)) {
				jsonBuffer+='[';
				size_t lastpos = 0;
				while(1) {
					size_t pos = uncompressedInMemoryRecordString.find(" $$ ", lastpos) ;
					if (pos == string::npos)
						break;
					string result =  uncompressedInMemoryRecordString.substr(lastpos, pos - lastpos);
					jsonBuffer+='"';
					cleanAndAppendToBuffer(result, jsonBuffer);
					jsonBuffer+='"';
					jsonBuffer+=',';
					lastpos = pos + 4;
				}
				string result =  uncompressedInMemoryRecordString.substr(lastpos,
															uncompressedInMemoryRecordString.length());
				jsonBuffer+='"';
				cleanAndAppendToBuffer(result, jsonBuffer);
				jsonBuffer+='"';
				jsonBuffer+=']';
				jsonBuffer+=',';
			} else {
				jsonBuffer+='"';
				cleanAndAppendToBuffer(uncompressedInMemoryRecordString, jsonBuffer);
				jsonBuffer+='"';
				jsonBuffer+=',';
			}
		}
		iter = storedAttrSchema->getRefiningAttributes()->begin();
		for ( ; iter != storedAttrSchema->getRefiningAttributes()->end(); iter++)
		{
			if (attrToReturn &&
					std::find(attrToReturn->begin(), attrToReturn->end(), iter->first) == attrToReturn->end()) {
				continue;
			}
			// check if it is also a primary key, skip if true because PK is emitted already.
			if (*(storedAttrSchema->getPrimaryKey()) == iter->first) {
				continue;
			}
			unsigned id = storedAttrSchema->getRefiningAttributeId(iter->first);
			unsigned lenOffset = compactRecDeserializer.getRefiningOffset(id);
			jsonBuffer+='"'; jsonBuffer+=iter->first; jsonBuffer+='"';
			jsonBuffer+=':';
			jsonBuffer+='"';
			switch(storedAttrSchema->getTypeOfRefiningAttribute(id)){
			case srch2is::ATTRIBUTE_TYPE_FLOAT:
			{
				float attrdata = *((float *)(buffer.start.get() + lenOffset));
				stringstream ss;
				ss << attrdata;
				jsonBuffer += ss.str();
				break;
			}
			case srch2is::ATTRIBUTE_TYPE_UNSIGNED:
			{
				unsigned attrdata = *((unsigned *)(buffer.start.get() + lenOffset));
				stringstream ss;
				ss << attrdata;
				jsonBuffer += ss.str();
				break;
			}
			default: break;
				// should not come here.
			}
			jsonBuffer+='"';
			jsonBuffer+=',';
		}
		if (storedAttrSchema->getRefiningAttributes()->size() > 0 ||
				storedAttrSchema->getSearchableAttribute().size() > 0)
			jsonBuffer.erase(jsonBuffer.length()-1);
		jsonBuffer.append("}");
}

void RecordSerializerUtil::cleanAndAppendToBuffer(const string& in, string& out) {
	unsigned inLen = in.length();
	unsigned inIdx = 0;
	while (inIdx < inLen) {
		// remove non printable characters
		// A byte of a non-ASCII character can be >= 128. 
		// For example, the utf8 byte array of "李" is e6-9d-8e. 
		// Thus "in[inIdx]" will be negative, and we have to treat it as an unsigned value.
		if ( static_cast<unsigned char> (in[inIdx]) < 32) {
			++inIdx; continue;
		}
		switch(in[inIdx]) {
		case '"':
		{
			// because we have reached here, there was no '\' before this '"'
			out +='\\'; out +='"';
			break;
		}
		case '\\':
		{
			if (inIdx != inLen - 1 and in[inIdx + 1] == '"') {  // looking for '\"'
				out += in[inIdx++];      // push them in one go...
				out += in[inIdx];
			} else {
				out +='\\'; out +='\\';  // escape the lonesome '\'
			}
			break;
		}
		default:
			out += in[inIdx];
		}
		++inIdx;
	}
}

/*
 *  The function returns the list of values for the list of refining attributes from
 *  the in-memory representation in forward Index.
 */
void RecordSerializerUtil::getBatchOfAttributes(
        const std::vector<string> & refiningAttributes, const Schema * schema, const Byte* data,
        std::vector<TypedValue> * typedValuesArg)  {

    std::vector<TypedValue>& typedValues = (*typedValuesArg);
    Schema *storedSchema = Schema::create();
    RecordSerializerUtil::populateStoredSchema(storedSchema, schema);
    RecordSerializer recSerializer(*storedSchema);
    // now extract the scores
    unsigned startOffset = 0;
    for(unsigned i = 0 ; i < refiningAttributes.size(); ++i){
    	const string& name = refiningAttributes[i];
    	FilterType type = getAttributeType(name, schema);
    	bool multiVal = schema->isRefiningAttributeMultiValued(schema->getRefiningAttributeId(name));
        TypedValue attributeValue;
        convertByteArrayToTypedValue(name , multiVal, type, recSerializer, data , &attributeValue);
        typedValues.push_back(attributeValue);
    }
    delete storedSchema;
}

FilterType RecordSerializerUtil::getAttributeType(const string& name,
        const Schema * schema) {
	unsigned id = schema->getRefiningAttributeId(name);
	return schema->getTypeOfRefiningAttribute(id);
}

/*
 *  read an unsigned int value at an offset (= startOffset) from the data pointer.
 */
unsigned RecordSerializerUtil::convertByteArrayToUnsigned(
        unsigned startOffset , const Byte * data) {

    const Byte * bytePointer = data + startOffset;
    unsigned * unsignedPointer = (unsigned *) bytePointer;
    return *unsignedPointer;
}

/*
 *  read a float value at an offset (= startOffset) from the data pointer.
 */
float RecordSerializerUtil::convertByteArrayToFloat(unsigned startOffset, const Byte * data) {
    const Byte * bytePointer = data + startOffset;
    float * floatPointer = (float *) bytePointer;
    return *floatPointer;
}

/*
 *  read a long value at an offset (= startOffset) from the data pointer.
 */
long RecordSerializerUtil::convertByteArrayToLong(
        unsigned startOffset, const Byte * data) {
    const Byte * bytePointer = data + startOffset;
    long * longPointer = (long *) bytePointer;
    return *longPointer;
}

/*
 *   Given a refining attribute name and type, fetch its value from in-memory compact representation.
 *   - Single value refining attribute of type int , float and long are stored as it is in the byte
 *     array.
 *   - Multivalue refining attributes are stored as single compressed string where each values is
 *     separated by a delimiter (MULTI_VAL_ATTR_DELIMITER defined in Constants.h).
 *   - Single Value refining attributes of type text and time are stored as compressed string.
 *     Note: time string is converted to long format.
 */
void RecordSerializerUtil::convertByteArrayToTypedValue(const string& name,
		bool isMultiValued, const FilterType& type, RecordSerializer& recSerializer, const Byte * data,
		TypedValue * result) {

	if(isMultiValued == false){ // case of single value
		unsigned intValue = 0;
		float floatValue = 0;
		long longValue = 0;
		unsigned sizeOfString = 0;
		string stringValue = "";
		switch (type) {
		case ATTRIBUTE_TYPE_UNSIGNED:
		{
			unsigned startOffset = 0;
			if (recSerializer.getStorageSchema().getRefiningAttributeId(name) != -1) {
				startOffset = recSerializer.getRefiningOffset(name);
				intValue = convertByteArrayToUnsigned(startOffset,data);
			} else {
				ASSERT(false);  // for Debug mode
			}
			result->setTypedValue(intValue);
			break;
		}
		case ATTRIBUTE_TYPE_FLOAT:
		{
			unsigned startOffset = 0;
			if (recSerializer.getStorageSchema().getRefiningAttributeId(name) != -1) {
				startOffset = recSerializer.getRefiningOffset(name);
				floatValue = convertByteArrayToFloat(startOffset,data);
			} else {
				ASSERT(false);  // for Debug mode
			}
			result->setTypedValue(floatValue);
			break;
		}
		case ATTRIBUTE_TYPE_TEXT:
		{
			unsigned lenOffset = 0;
			if (recSerializer.getStorageSchema().getSearchableAttributeId(name) != -1) {
				lenOffset = recSerializer.getSearchableOffset(name);
				const char *attrdata = data + *((unsigned *)(data + lenOffset));
				unsigned len = *(((unsigned *)(data + lenOffset)) + 1) -
						*((unsigned *)(data + lenOffset));
				snappy::Uncompress(attrdata,len, &stringValue);
				std::transform(stringValue.begin(), stringValue.end(), stringValue.begin(), ::tolower);
			} else {
				ASSERT(false);  // for Debug mode
			}
			result->setTypedValue(stringValue);
			break;
		}
		case ATTRIBUTE_TYPE_TIME:
		{
			unsigned lenOffset = 0;
			if (recSerializer.getStorageSchema().getSearchableAttributeId(name) != -1) {
				lenOffset = recSerializer.getSearchableOffset(name);
				const char *attrdata = data + *((unsigned *)(data + lenOffset));
				unsigned len = *(((unsigned *)(data + lenOffset)) + 1) -
						*((unsigned *)(data + lenOffset));
				snappy::Uncompress(attrdata,len, &stringValue);
				longValue = DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(stringValue);
			} else {
				ASSERT(false);  // for Debug mode
			}
			result->setTypedValue(longValue);
			break;
		}

		default:
			ASSERT(false);
			break;
		}
	}else{ // case of multi value

		unsigned lenOffset = 0;
		string stringValue = "";
		vector<string> multiValues;
		if (recSerializer.getStorageSchema().getSearchableAttributeId(name) != -1) {
			lenOffset = recSerializer.getSearchableOffset(name);
			const char *attrdata = data + *((unsigned *)(data + lenOffset));
			unsigned len = *(((unsigned *)(data + lenOffset)) + 1) -
					*((unsigned *)(data + lenOffset));
			snappy::Uncompress(attrdata,len, &stringValue);
			size_t lastpos = 0;
			while(1) {
				size_t pos = stringValue.find(MULTI_VAL_ATTR_DELIMITER, lastpos) ;
				if (pos == string::npos)
					break;
				string result =  stringValue.substr(lastpos, pos - lastpos);
				std::transform(result.begin(), result.end(), result.begin(), ::tolower);
				multiValues.push_back(result);
				lastpos = pos + 4;
			}
			string lastVal = stringValue.substr(lastpos, stringValue.size());
			std::transform(lastVal.begin(), lastVal.end(), lastVal.begin(), ::tolower);
			multiValues.push_back(lastVal);
		} else {
			ASSERT(false);  // for debug ..else we return empty values
		}

		vector<unsigned> intValues;
		vector<float> floatValues;
		vector<long> timeValues;

		switch (type) {
		case ATTRIBUTE_TYPE_UNSIGNED:
			for(int i=0;i<multiValues.size(); i++){
				intValues.push_back(atol(multiValues[i].c_str()));
			}
			result->setTypedValue(intValues);
			break;
		case ATTRIBUTE_TYPE_FLOAT:
			for(int i=0;i<multiValues.size(); i++){
				floatValues.push_back(atof(multiValues[i].c_str()));
			}
			result->setTypedValue(floatValues);
			break;
		case ATTRIBUTE_TYPE_TEXT:
			result->setTypedValue(multiValues);
			break;
		case ATTRIBUTE_TYPE_TIME:
			for(int i=0;i<multiValues.size(); i++){
				long lTime = DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(multiValues[i]);
				timeValues.push_back(lTime);
			}
			result->setTypedValue(timeValues);
			break;
		case ATTRIBUTE_TYPE_DURATION:
			ASSERT(false);
			break;
		default:
			ASSERT(false);
			break;
		}
	}
}

RecordSerializerUtil::RecordSerializerUtil() {
}

RecordSerializerUtil::~RecordSerializerUtil() {
}

} /* namespace util */
} /* namespace srch2 */
