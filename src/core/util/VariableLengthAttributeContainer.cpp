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

namespace srch2 {
namespace instantsearch {

// it calculates and returns the number of bytes that this list will need
unsigned VariableLengthAttributeContainer::getSizeNeededForAllocation(
		const Schema * schema,
		const vector<vector< string> > & nonSearchableAttributeValues){
	ASSERT(nonSearchableAttributeValues.size() == schema->getNumberOfRefiningAttributes());
    unsigned totalLength = 0;
    for (int i = 0; i < schema->getNumberOfRefiningAttributes(); ++i) { // iterate on attributes in schema
        // find the type of ith attribute
        FilterType type = getAttributeType(i, schema);
        // find out if the i-th attribute is multivalued
        bool isMultiValued = schema->isRefiningAttributeMultiValued(i);
        if(isMultiValued == true){
			switch (type) {
			case ATTRIBUTE_TYPE_UNSIGNED:
				totalLength += sizeof(unsigned) + nonSearchableAttributeValues.at(i).size() * sizeof(unsigned);
				break;
			case ATTRIBUTE_TYPE_FLOAT:
				totalLength += sizeof(float) + nonSearchableAttributeValues.at(i).size() * sizeof(float);
				break;
			case ATTRIBUTE_TYPE_TEXT:
				totalLength += (sizeof(unsigned) + nonSearchableAttributeValues.at(i).size() * sizeof(unsigned)); //directory
				for(vector<string>::const_iterator valueIter = nonSearchableAttributeValues.at(i).begin() ; valueIter != nonSearchableAttributeValues.at(i).end(); ++valueIter){
					totalLength += valueIter->size();
				}
				break;
			case ATTRIBUTE_TYPE_TIME:
				totalLength += sizeof(long)+ nonSearchableAttributeValues.at(i).size() * sizeof(long);
				break;
			case ATTRIBUTE_TYPE_DURATION:
				ASSERT(false);
				break;
			default:
				ASSERT(false);
				break;
			}
        }else{ // single value case
        	ASSERT(nonSearchableAttributeValues.at(i).size() == 1);
			switch (type) {
			case ATTRIBUTE_TYPE_UNSIGNED:
				totalLength += sizeof(unsigned);
				break;
			case ATTRIBUTE_TYPE_FLOAT:
				totalLength += sizeof(float);
				break;
			case ATTRIBUTE_TYPE_TEXT:
				totalLength += (sizeof(unsigned) + nonSearchableAttributeValues.at(i).at(0).size() );
				break;
			case ATTRIBUTE_TYPE_TIME:
				totalLength += sizeof(long);
				break;
			case ATTRIBUTE_TYPE_DURATION:
				ASSERT(false);
				break;
			default:
				ASSERT(false);
				break;
			}
        }
        //
    }
    //
    return totalLength;
}

// fills the container with the values
// Byte *& data is a pass by reference of a pointer variable, data will be allocated and set in this function.
void VariableLengthAttributeContainer::fillWithoutAllocation(
		const Schema * schema,
		const vector<vector< string> > & nonSearchableAttributeValues, Byte * data){

    // iterate on nonsearchableAttributes and make a Byte vector
    unsigned startOffset = 0;
    for (int nSAIndex = 0;
            nSAIndex < schema->getNumberOfRefiningAttributes();
            nSAIndex++) {
        // find the type of nSAIndex-th attribute
        FilterType type = getAttributeType(nSAIndex, schema);
        unsigned usedSizeForThisAttribute = 0;
        if(schema->isRefiningAttributeMultiValued(nSAIndex) == false){ // single value
			convertStringToByteArray(type,
					nonSearchableAttributeValues.at(nSAIndex).at(0), data,
					startOffset, usedSizeForThisAttribute);
        }else{ // multi valued
			convertStringToByteArrayMultiValued(type,
					nonSearchableAttributeValues.at(nSAIndex), data,
					startOffset, usedSizeForThisAttribute);
        }
        startOffset += usedSizeForThisAttribute;
    }

}



// fills the container with the values
void VariableLengthAttributeContainer::fill(const Schema * schema,
        const vector<vector<string> > & nonSearchableAttributeValues, Byte *& data , unsigned & dataSize) {
    // to make sure this class is not updates anywhere in the system
    ASSERT(data == NULL);

    // first allocate the Byte array
    allocate(schema, nonSearchableAttributeValues , data , dataSize);

    // now fill the allocated space
    fillWithoutAllocation(schema, nonSearchableAttributeValues, data);
}


// deallocates the data and clears the container. After calling this function it can be filled again.
void VariableLengthAttributeContainer::clear(Byte *& data , unsigned dataSize){
    if(data != NULL) {
        delete data;
        data = NULL;
    }
    dataSize = 0;
}

std::string VariableLengthAttributeContainer::getAttribute(
        unsigned nonSearchableAttributeIndex, const Schema * schema, const Byte * data)  {

    unsigned startOffset = 0;
    for(int i=0; i < schema->getNumberOfRefiningAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        bool isMultiValued = schema->isRefiningAttributeMultiValued(i);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
            return convertByteArrayToString(type ,startOffset,data );
        }else{ // skip this attribute because we are looking for a certain attribute which is not this one
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , isMultiValued , startOffset , data);
            startOffset += numberOfBytesToSkip;
        }
    }
    ASSERT(false);
    return "";
}
// gets the attribute value wrapped in a Score object
void VariableLengthAttributeContainer::getAttribute(const unsigned nonSearchableAttributeIndex,
        const Schema * schema, const Byte * data, TypedValue * typedValue) {
    unsigned startOffset = 0;
    for(int i=0; i < schema->getNumberOfRefiningAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        bool isMultiValued = schema->isRefiningAttributeMultiValued(i);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
            convertByteArrayToTypedValue(type , isMultiValued ,startOffset, data , typedValue );
            return;
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type, isMultiValued , startOffset, data);
            startOffset += numberOfBytesToSkip;
        }
    }
    ASSERT(false);
}

// gets values of attributes in iters in Score objects. iters must be ascending.
void VariableLengthAttributeContainer::getBatchOfAttributes(
        const std::vector<unsigned> & nonSearchableAttributeIndexesArg, const Schema * schema, const Byte * data,
        std::vector<TypedValue> * typedValuesArg)  {

    std::vector<TypedValue> typedValues;
    std::vector<unsigned> nonSearchableAttributeIndexes = nonSearchableAttributeIndexesArg;

    // first make sure input IDs are sorted.
    std::sort(nonSearchableAttributeIndexes.begin() , nonSearchableAttributeIndexes.end());
    // now extract the scores
    unsigned startOffset = 0;
    for(int nonSearchableAttributeIndex=0; nonSearchableAttributeIndex < schema->getNumberOfRefiningAttributes() ; nonSearchableAttributeIndex++){
        FilterType type = getAttributeType(nonSearchableAttributeIndex , schema);
        bool isMultivalued = schema->isRefiningAttributeMultiValued(nonSearchableAttributeIndex);
        if(find(nonSearchableAttributeIndexes.begin()
                ,nonSearchableAttributeIndexes.end() , nonSearchableAttributeIndex) != nonSearchableAttributeIndexes.end()){ // index is among requested indexes
            TypedValue attributeValue;
            convertByteArrayToTypedValue(type , isMultivalued , startOffset,data , &attributeValue);
            typedValues.push_back(attributeValue);
            // also must move the startOffset forward
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , isMultivalued,  startOffset ,data);
            startOffset += numberOfBytesToSkip;
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , isMultivalued,  startOffset , data );
            startOffset += numberOfBytesToSkip;
        }
    }

    // return the scores in the requested orders.
    for(std::vector<unsigned>::const_iterator originalID = nonSearchableAttributeIndexesArg.begin() ;
            originalID != nonSearchableAttributeIndexesArg.end() ; ++originalID){
        unsigned indexOfOriginalIDInNewVector =
                std::distance(
                        nonSearchableAttributeIndexes.begin(),
                        std::find(nonSearchableAttributeIndexes.begin() , nonSearchableAttributeIndexes.end() , *originalID ));
        typedValuesArg->push_back(typedValues.at(indexOfOriginalIDInNewVector));
    }
}

unsigned VariableLengthAttributeContainer::getUnsignedAttribute(const unsigned nonSearchableAttributeIndex,
        const Schema * schema, const Byte * data)  {
    unsigned startOffset = 0;
    for(int i=0; i < schema->getNumberOfRefiningAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        bool isMultiValued = schema->isRefiningAttributeMultiValued(i);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
			ASSERT(isMultiValued == false);
            ASSERT(type == ATTRIBUTE_TYPE_UNSIGNED);
            if(type != ATTRIBUTE_TYPE_UNSIGNED) return 0; // zero is returned in case the type is not unsigned
            return convertByteArrayToUnsigned(startOffset,data);
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , isMultiValued , startOffset,data );
            startOffset += numberOfBytesToSkip;
        }
    }
    ASSERT(false);
    return 0;

}
float VariableLengthAttributeContainer::getFloatAttribute(const unsigned nonSearchableAttributeIndex,
        const Schema * schema, const Byte * data) {
    unsigned startOffset = 0;
    for(int i=0; i < schema->getNumberOfRefiningAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        bool isMultiValued = schema->isRefiningAttributeMultiValued(i);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
			ASSERT(isMultiValued == false);
            ASSERT(type == ATTRIBUTE_TYPE_FLOAT);
            return convertByteArrayToFloat(startOffset,data);
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type, isMultiValued , startOffset,data );
            startOffset += numberOfBytesToSkip;
        }
    }
    ASSERT(false);
    return 0;
}
double VariableLengthAttributeContainer::getDoubleAttribute(const unsigned nonSearchableAttributeIndex,
        const Schema * schema, const Byte * data) {
    return (double) getFloatAttribute(nonSearchableAttributeIndex, schema,data);
}
std::string VariableLengthAttributeContainer::getTextAttribute(const unsigned nonSearchableAttributeIndex,
        const Schema * schema, const Byte * data) {

    unsigned startOffset = 0;
    for(int i=0; i < schema->getNumberOfRefiningAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        bool isMultiValued = schema->isRefiningAttributeMultiValued(i);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
			ASSERT(isMultiValued == false);
            ASSERT(type == ATTRIBUTE_TYPE_TEXT);
            if(type != ATTRIBUTE_TYPE_TEXT) return "";
            return convertByteArrayToString(type, startOffset,data);
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type, isMultiValued , startOffset, data );
            startOffset += numberOfBytesToSkip;
        }
    }
    ASSERT(false);
    return "";

}
long VariableLengthAttributeContainer::getTimeAttribute(const unsigned nonSearchableAttributeIndex,
        const Schema * schema, const Byte * data) {

    unsigned startOffset = 0;
    for(int i=0; i < schema->getNumberOfRefiningAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        bool isMultiValued = schema->isRefiningAttributeMultiValued(i);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
			ASSERT(isMultiValued == false);
            ASSERT(type == ATTRIBUTE_TYPE_TIME);
            return convertByteArrayToLong( startOffset,data);
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , isMultiValued, startOffset, data );
            startOffset += numberOfBytesToSkip;
        }
    }
    ASSERT(false);
    return 0;

}



vector<unsigned> VariableLengthAttributeContainer::getMultiUnsignedAttribute(const unsigned nonSearchableAttributeIndex,
        const Schema * schema, const Byte * data){

	vector<unsigned> values;
    unsigned startOffset = 0;
    for(int i=0; i < schema->getNumberOfRefiningAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        bool isMultiValued = schema->isRefiningAttributeMultiValued(i);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
			ASSERT(isMultiValued == true);
            ASSERT(type == ATTRIBUTE_TYPE_UNSIGNED);
            unsigned numberOfValues = convertByteArrayToUnsigned(startOffset , data);
            startOffset += sizeof(unsigned);
            for(int i=0; i< numberOfValues ; i++){
            	values.push_back(convertByteArrayToUnsigned( startOffset,data));
            	startOffset += sizeof(unsigned);
            }
            return values;
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , isMultiValued, startOffset, data );
            startOffset += numberOfBytesToSkip;
        }
    }
    ASSERT(false);
    return values;

}

vector<float> VariableLengthAttributeContainer::getMultiFloatAttribute(const unsigned nonSearchableAttributeIndex,
        const Schema * schema, const Byte * data){

	vector<float> values;
    unsigned startOffset = 0;
    for(int i=0; i < schema->getNumberOfRefiningAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        bool isMultiValued = schema->isRefiningAttributeMultiValued(i);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
			ASSERT(isMultiValued == true);
            ASSERT(type == ATTRIBUTE_TYPE_FLOAT);
            unsigned numberOfValues = convertByteArrayToUnsigned(startOffset , data);
            startOffset += sizeof(unsigned);
            for(int i=0; i< numberOfValues ; i++){
            	values.push_back(convertByteArrayToFloat( startOffset,data));
            	startOffset += sizeof(float);
            }
            return values;
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , isMultiValued, startOffset, data );
            startOffset += numberOfBytesToSkip;
        }
    }
    ASSERT(false);
    return values;

}
vector<std::string> VariableLengthAttributeContainer::getMultiTextAttribute(const unsigned nonSearchableAttributeIndex,
        const Schema * schema, const Byte * data) {

	vector<string> values;
    unsigned startOffset = 0;
    for(int i=0; i < schema->getNumberOfRefiningAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        bool isMultiValued = schema->isRefiningAttributeMultiValued(i);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
			ASSERT(isMultiValued == true);
            ASSERT(type == ATTRIBUTE_TYPE_TEXT);
            unsigned numberOfValues = convertByteArrayToUnsigned(startOffset , data);
            startOffset += sizeof(unsigned);
            for(int i=0; i< numberOfValues ; i++){
            	string value = convertByteArrayToString(ATTRIBUTE_TYPE_TEXT, startOffset,data);
            	values.push_back(value);
            	startOffset += value.size() + sizeof(unsigned);
            }
            return values;
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , isMultiValued, startOffset, data );
            startOffset += numberOfBytesToSkip;
        }
    }
    ASSERT(false);
    return values;

}
vector<long> VariableLengthAttributeContainer::getMultiTimeAttribute(const unsigned nonSearchableAttributeIndex,
        const Schema * schema, const Byte * data) {

    vector<long> values;
    unsigned startOffset = 0;
    for(int i=0; i < schema->getNumberOfRefiningAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        bool isMultiValued = schema->isRefiningAttributeMultiValued(i);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
			ASSERT(isMultiValued == true);
            ASSERT(type == ATTRIBUTE_TYPE_TIME);
            unsigned numberOfValues = convertByteArrayToUnsigned(startOffset , data);
            startOffset += sizeof(unsigned);
            for(int i=0; i< numberOfValues ; i++){
            	values.push_back(convertByteArrayToLong( startOffset,data));
            	startOffset += sizeof(long);
            }
            return values;
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , isMultiValued, startOffset, data );
            startOffset += numberOfBytesToSkip;
        }
    }
    ASSERT(false);
    return values;

}


void VariableLengthAttributeContainer::allocate(const Schema * schema,
        const vector<vector<string> > & nonSearchableAttributeValues, Byte *& data, unsigned & dataSize) {

    unsigned totalLength = getSizeNeededForAllocation(schema , nonSearchableAttributeValues);
    //
    data = new Byte[totalLength];
    dataSize = totalLength;
}


FilterType VariableLengthAttributeContainer::getAttributeType(unsigned iter,
        const Schema * schema) {
    ASSERT(iter < schema->getNumberOfRefiningAttributes());
    return schema->getTypeOfRefiningAttribute(iter);
}



unsigned VariableLengthAttributeContainer::getSizeOfNonSearchableAttributeValueInData(
        FilterType filterType,bool isMultiValued, unsigned startOffset, const Byte * data) {
	if(isMultiValued == false){// single value
		unsigned sizeOfString = 0;
		switch (filterType) {
		case ATTRIBUTE_TYPE_UNSIGNED:
			return sizeof(unsigned);
			break;
		case ATTRIBUTE_TYPE_FLOAT:
			return sizeof(float);
			break;
		case ATTRIBUTE_TYPE_TEXT:
			sizeOfString = convertByteArrayToUnsigned(startOffset, data);
			return sizeOfString + sizeof(unsigned);
			break;
		case ATTRIBUTE_TYPE_TIME:
			return sizeof(long);
			break;
		case ATTRIBUTE_TYPE_DURATION:
			ASSERT(false);
			break;
		default:
			ASSERT(false);
			break;
		}
	}else{
		unsigned numberOfValues = convertByteArrayToUnsigned(startOffset, data);
		startOffset += sizeof(unsigned);
		unsigned totalLengthToReturn = sizeof(unsigned);
		switch (filterType) {
		case ATTRIBUTE_TYPE_UNSIGNED:
			return totalLengthToReturn + numberOfValues * sizeof(unsigned);
			break;
		case ATTRIBUTE_TYPE_FLOAT:
			return totalLengthToReturn + numberOfValues * sizeof(float);
			break;
		case ATTRIBUTE_TYPE_TEXT:
			for(int i=0; i<numberOfValues; i++){
				unsigned sizeOfString = convertByteArrayToUnsigned(startOffset, data);
				startOffset += sizeOfString + sizeof(unsigned);
				totalLengthToReturn += sizeOfString + sizeof(unsigned);
			}
			return totalLengthToReturn;
			break;
		case ATTRIBUTE_TYPE_TIME:
			return totalLengthToReturn + numberOfValues * sizeof(long);
			break;
		case ATTRIBUTE_TYPE_DURATION:
			ASSERT(false);
			break;
		default:
			ASSERT(false);
			break;
		}
	}
    ASSERT(false);
    return 0;
}

void VariableLengthAttributeContainer::convertUnsignedToByteArray(
        unsigned input, Byte * output, unsigned startOffset) {

    Byte * p = reinterpret_cast<Byte *>(&input);
    for (int i = 0; i < sizeof(unsigned); i++) {
        output[startOffset + i] = p[i];
    }
}
unsigned VariableLengthAttributeContainer::convertByteArrayToUnsigned(
        unsigned startOffset , const Byte * data) {

    const Byte * bytePointer = data + startOffset;
    unsigned * unsignedPointer = (unsigned *) bytePointer;
    return *unsignedPointer;
}

// Converting float and char vector together.
void VariableLengthAttributeContainer::convertFloatToByteArray(float input,
        Byte * output, unsigned startOffset) {

    Byte const * p = reinterpret_cast<Byte const *>(&input);
    for (int i = 0; i < sizeof(float); i++) {
        output[startOffset + i] = p[i];
    }
}
float VariableLengthAttributeContainer::convertByteArrayToFloat(unsigned startOffset, const Byte * data) {
    const Byte * bytePointer = data + startOffset;
    float * floatPointer = (float *) bytePointer;
    return *floatPointer;
}

// Converting long and char vector together.
void VariableLengthAttributeContainer::convertLongToByteArray(long input,
        Byte * output, unsigned startOffset) {
    Byte const * p = reinterpret_cast<Byte const *>(&input);
    for (int i = 0; i < sizeof(long); i++) {
        output[startOffset + i] = p[i];
    }
}
long VariableLengthAttributeContainer::convertByteArrayToLong(
        unsigned startOffset, const Byte * data) {
    const Byte * bytePointer = data + startOffset;
    long * longPointer = (long *) bytePointer;
    return *longPointer;
}

void VariableLengthAttributeContainer::convertStringToByteArray(FilterType type,
		string value, Byte * output, unsigned startOffset,
        unsigned & sizeInBytes) {

    unsigned intValue = 0;
    float floatValue = 0;
    long longValue = 0;
    int i=0;
    switch (type) {
    case ATTRIBUTE_TYPE_UNSIGNED:
    	// first write the number of values
        intValue = atoi(value.c_str());
        convertUnsignedToByteArray(intValue, output, startOffset);
        sizeInBytes = sizeof(unsigned);
        break;
    case ATTRIBUTE_TYPE_FLOAT:
        floatValue = atof(value.c_str());
        convertFloatToByteArray(floatValue, output, startOffset);
        sizeInBytes = sizeof(float);
        break;
    case ATTRIBUTE_TYPE_TEXT:
        convertUnsignedToByteArray(value.size(), output, startOffset);
        startOffset += sizeof(unsigned);
        // NOTE : since string iterator iterates on bytes, this implementation is consistent with
        // multi-language because it only copies the bytes of the string to the byte array ...
        for (std::string::iterator it = value.begin(); it != value.end();
                ++it) {
            output[startOffset + i] = *it;
            //
            i++;
        }
        sizeInBytes = sizeof(unsigned) + value.size();
        break;
    case ATTRIBUTE_TYPE_TIME:
        longValue = atol(value.c_str());
        convertLongToByteArray(longValue, output, startOffset);
        sizeInBytes = sizeof(long);
        break;
    case ATTRIBUTE_TYPE_DURATION:
    	ASSERT(false);
    	break;
	default:
		ASSERT(false);
		break;
    }

}

// This function converts a list of values of a multi-valued attribute to a byte array and saves them in output
// Example : <"hello", "goodbye"> =>
// 4 bytes for value 2(number of values) | 4 bytes for value 5 | 5 bytes for hello | 4 bytes for value 7 | 7 bytes for goodbye
void VariableLengthAttributeContainer::convertStringToByteArrayMultiValued(FilterType type,
		vector< string > values, Byte * output, unsigned startOffset,
        unsigned & sizeInBytes) {
	sizeInBytes = 0;
	// first write the number of values
	convertUnsignedToByteArray(values.size(), output, startOffset);
    startOffset += sizeof(unsigned);
    sizeInBytes += sizeof(unsigned);
    // now write all the values after each other ...
    for(vector<string>::iterator unsignedValue = values.begin() ; unsignedValue != values.end() ; ++unsignedValue){
    	unsigned usedSizeForSingleValue;
    	convertStringToByteArray(type , *unsignedValue , output , startOffset , usedSizeForSingleValue);
    	sizeInBytes += usedSizeForSingleValue;
    	startOffset += usedSizeForSingleValue;
    }
}

std::string VariableLengthAttributeContainer::convertByteArrayToString(
        FilterType type, unsigned startOffset, const Byte * data) {
    std::string result = "";
    unsigned intValue = 0;
    float floatValue = 0;
    long longValue = 0;
    unsigned sizeOfString = 0;
    std::stringstream ss;
    switch (type) {
    case ATTRIBUTE_TYPE_UNSIGNED:
        intValue = convertByteArrayToUnsigned(startOffset,data);
        ss << intValue;
        result = ss.str();
        break;
    case ATTRIBUTE_TYPE_FLOAT:
        floatValue = convertByteArrayToFloat(startOffset,data);
        ss << floatValue;
        result = ss.str();
        break;
    case ATTRIBUTE_TYPE_TEXT:
        sizeOfString = convertByteArrayToUnsigned(startOffset,data);
        startOffset += sizeof(unsigned);
        for (int i = 0; i < sizeOfString; i++) {
            result.push_back(data[startOffset + i]);
        }
        break;
    case ATTRIBUTE_TYPE_TIME:
        longValue = convertByteArrayToLong(startOffset, data);
        ss << longValue;
        result = ss.str();
        break;
    case ATTRIBUTE_TYPE_DURATION:
    	ASSERT(false);
    	break;
	default:
		ASSERT(false);
		break;
    }

    return result;
}

void VariableLengthAttributeContainer::convertByteArrayToTypedValue(FilterType type, bool isMultiValued,
        unsigned startOffset, const Byte * data, TypedValue * result) {
	if(isMultiValued == false){ // case of single value
		unsigned intValue = 0;
		float floatValue = 0;
		long longValue = 0;
		unsigned sizeOfString = 0;
		string stringValue = "";
		switch (type) {
		case ATTRIBUTE_TYPE_UNSIGNED:
			intValue = convertByteArrayToUnsigned(startOffset,data);
			result->setTypedValue(intValue);
			break;
		case ATTRIBUTE_TYPE_FLOAT:
			floatValue = convertByteArrayToFloat(startOffset,data);
			result->setTypedValue(floatValue);
			break;
		case ATTRIBUTE_TYPE_TEXT:
			sizeOfString = convertByteArrayToUnsigned(startOffset,data);
			startOffset += sizeof(unsigned);
			for (int i = 0; i < sizeOfString; i++) {
				stringValue.push_back(data[startOffset + i]);
			}
			result->setTypedValue(stringValue);
			break;
		case ATTRIBUTE_TYPE_TIME:
			longValue = convertByteArrayToLong(startOffset,data);
			result->setTypedValue(longValue);
			break;
		case ATTRIBUTE_TYPE_DURATION:
			ASSERT(false);
			break;
		default:
			ASSERT(false);
			break;
		}
	}else{ // case of multi value : move on bytes and read single values one by one
		unsigned numberOfValues;
		numberOfValues = convertByteArrayToUnsigned(startOffset,data);
		startOffset += sizeof(numberOfValues);
		vector<unsigned> intValues;
		vector<float> floatValues;
		unsigned sizeOfStringValue;
		string stringValue;
		vector<string> textValues;
		vector<long> timeValues;
		switch (type) {
		case ATTRIBUTE_TYPE_UNSIGNED:
			for(int i=0;i<numberOfValues ; i++){
				intValues.push_back(convertByteArrayToUnsigned(startOffset,data));
				startOffset += sizeof(unsigned);
			}
			result->setTypedValue(intValues);
			break;
		case ATTRIBUTE_TYPE_FLOAT:
			for(int i=0;i<numberOfValues ; i++){
				floatValues.push_back(convertByteArrayToFloat(startOffset,data));
				startOffset += sizeof(float);
			}
			result->setTypedValue(floatValues);
			break;
		case ATTRIBUTE_TYPE_TEXT:
			for(int i=0;i<numberOfValues ; i++){
				stringValue = "";
				sizeOfStringValue = convertByteArrayToUnsigned(startOffset,data);
				startOffset += sizeof(unsigned);
				for (int i = 0; i < sizeOfStringValue; i++) {
					stringValue.push_back(data[startOffset + i]);
				}
				textValues.push_back(stringValue);
				startOffset += sizeOfStringValue;
			}
			result->setTypedValue(textValues);
			break;

		case ATTRIBUTE_TYPE_TIME:
			for(int i=0;i<numberOfValues ; i++){
				timeValues.push_back(convertByteArrayToLong(startOffset,data));
				startOffset += sizeof(long);
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

