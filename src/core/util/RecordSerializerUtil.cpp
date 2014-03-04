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
			unsigned id = storedAttrSchema->getSearchableAttributeId(iter->first);
			unsigned lenOffset = compactRecDeserializer.getSearchableOffset(id);
			const char *attrdata = buffer.start + *((unsigned *)(buffer.start + lenOffset));
			unsigned len = *(((unsigned *)(buffer.start + lenOffset)) + 1) -
					*((unsigned *)(buffer.start + lenOffset));

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
			unsigned id = storedAttrSchema->getRefiningAttributeId(iter->first);
			unsigned lenOffset = compactRecDeserializer.getRefiningOffset(id);
			jsonBuffer+='"'; jsonBuffer+=iter->first; jsonBuffer+='"';
			jsonBuffer+=':';
			jsonBuffer+='"';
			switch(storedAttrSchema->getTypeOfRefiningAttribute(id)){
			case srch2is::ATTRIBUTE_TYPE_FLOAT:
			{
				float attrdata = *((float *)(buffer.start + lenOffset));
				stringstream ss;
				ss << attrdata;
				jsonBuffer += ss.str();
				break;
			}
			case srch2is::ATTRIBUTE_TYPE_UNSIGNED:
			{
				unsigned attrdata = *((unsigned *)(buffer.start + lenOffset));
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
		if (in[inIdx] < 32) {
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

RecordSerializerUtil::RecordSerializerUtil() {
}

RecordSerializerUtil::~RecordSerializerUtil() {
}

} /* namespace util */
} /* namespace srch2 */
