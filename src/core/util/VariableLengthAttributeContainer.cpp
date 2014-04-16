// $Id$ 06/21/13 Jamshid

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2013 SRCH2 Inc. All rights reserved
 */

#include "VariableLengthAttributeContainer.h"

#include <string>
#include <sstream>
#include <iostream>
#include "Assert.h"

#include "thirdparty/snappy-1.0.4/snappy.h"
#include "util/DateAndTimeHandler.h"
#include "instantsearch/Constants.h"

using namespace snappy;

namespace srch2 {
namespace instantsearch {

// gets values of attributes in iters in Score objects. iters must be ascending.
void VariableLengthAttributeContainer::getBatchOfAttributes(
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

FilterType VariableLengthAttributeContainer::getAttributeType(const string& name,
        const Schema * schema) {
	unsigned id = schema->getRefiningAttributeId(name);
	return schema->getTypeOfRefiningAttribute(id);
}


unsigned VariableLengthAttributeContainer::convertByteArrayToUnsigned(
        unsigned startOffset , const Byte * data) {

    const Byte * bytePointer = data + startOffset;
    unsigned * unsignedPointer = (unsigned *) bytePointer;
    return *unsignedPointer;
}

float VariableLengthAttributeContainer::convertByteArrayToFloat(unsigned startOffset, const Byte * data) {
    const Byte * bytePointer = data + startOffset;
    float * floatPointer = (float *) bytePointer;
    return *floatPointer;
}

long VariableLengthAttributeContainer::convertByteArrayToLong(
        unsigned startOffset, const Byte * data) {
    const Byte * bytePointer = data + startOffset;
    long * longPointer = (long *) bytePointer;
    return *longPointer;
}

void VariableLengthAttributeContainer::convertByteArrayToTypedValue(const string& name,
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
			startOffset = recSerializer.getRefiningOffset(name);
			intValue = convertByteArrayToUnsigned(startOffset,data);
			result->setTypedValue(intValue);
			break;
		}
		case ATTRIBUTE_TYPE_FLOAT:
		{
			unsigned startOffset = 0;
			startOffset = recSerializer.getRefiningOffset(name);
			floatValue = convertByteArrayToFloat(startOffset,data);
			result->setTypedValue(floatValue);
			break;
		}
		case ATTRIBUTE_TYPE_TEXT:
		{
			unsigned lenOffset = 0;
			lenOffset = recSerializer.getSearchableOffset(name);
			const char *attrdata = data + *((unsigned *)(data + lenOffset));
			unsigned len = *(((unsigned *)(data + lenOffset)) + 1) -
					*((unsigned *)(data + lenOffset));
			snappy::Uncompress(attrdata,len, &stringValue);
			result->setTypedValue(stringValue);
			break;
		}
		case ATTRIBUTE_TYPE_TIME:
		{
			unsigned lenOffset = 0;
			lenOffset = recSerializer.getSearchableOffset(name);
			const char *attrdata = data + *((unsigned *)(data + lenOffset));
			unsigned len = *(((unsigned *)(data + lenOffset)) + 1) -
					*((unsigned *)(data + lenOffset));
			snappy::Uncompress(attrdata,len, &stringValue);
			longValue = DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(stringValue);
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
			multiValues.push_back(result);
			lastpos = pos + 4;
		}
		multiValues.push_back(stringValue.substr(lastpos, stringValue.size()));

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

}
}

