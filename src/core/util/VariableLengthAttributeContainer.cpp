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

// it calculates and returns the number of butes that this list will need
unsigned VariableLengthAttributeContainer::getSizeNeededForAllocation(
		const Schema * schema,
		const vector<string> & nonSearchableAttributeValues){
    unsigned totalLength = 0;
    for (int i = 0; i < schema->getNumberOfNonSearchableAttributes(); ++i) { // iterate on attributes in schema
        // find the type of ith attribute
        FilterType type = getAttributeType(i, schema);
        //
        switch (type) {
        case ATTRIBUTE_TYPE_UNSIGNED:
            totalLength += sizeof(unsigned);
            break;
        case ATTRIBUTE_TYPE_FLOAT:
            totalLength += sizeof(float);
            break;
        case ATTRIBUTE_TYPE_TEXT:
            totalLength += (sizeof(unsigned)
                    + nonSearchableAttributeValues.at(i).size());
            break;
        case ATTRIBUTE_TYPE_TIME:
            totalLength += sizeof(long);
            break;
        case ATTRIBUTE_TYPE_DURATION:
        	ASSERT(false);
        	break;
        }
    }
    //
    return totalLength;
}

// fills the container with the values
// Byte *& data is a pass by reference of a pointer variable, data will be allocated and set in this function.
void VariableLengthAttributeContainer::fillWithoutAllocation(
		const Schema * schema,
		const vector<string> & nonSearchableAttributeValues, Byte * data){

    // iterate on nonsearchableAttributes and make a Byte vector
    unsigned startOffset = 0;
    for (int nSAIndex = 0;
            nSAIndex < schema->getNumberOfNonSearchableAttributes();
            nSAIndex++) {
        // find the type of nSAIndex-th attribute
        FilterType type = getAttributeType(nSAIndex, schema);
        unsigned usedSizeForThisAttribute = 0;
        convertStringToByteArray(type,
                nonSearchableAttributeValues.at(nSAIndex), data,
                startOffset, usedSizeForThisAttribute);
        startOffset += usedSizeForThisAttribute;
    }

}



// fills the container with the values
void VariableLengthAttributeContainer::fill(const Schema * schema,
        const vector<string> & nonSearchableAttributeValues, Byte *& data , unsigned & dataSize) {
    // to make sure this class is not updates anywhere in the system
    if (data != NULL) {
        ASSERT(false);
    }

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
    for(int i=0; i < schema->getNumberOfNonSearchableAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
            return convertByteArrayToString(type ,startOffset,data );
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , startOffset , data);
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
    for(int i=0; i < schema->getNumberOfNonSearchableAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
            convertByteArrayToTypedValue(type ,startOffset, data , typedValue );
            return;
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , startOffset, data);
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
    for(int nonSearchableAttributeIndex=0; nonSearchableAttributeIndex < schema->getNumberOfNonSearchableAttributes() ; nonSearchableAttributeIndex++){
        FilterType type = getAttributeType(nonSearchableAttributeIndex , schema);
        if(find(nonSearchableAttributeIndexes.begin()
                ,nonSearchableAttributeIndexes.end() , nonSearchableAttributeIndex) != nonSearchableAttributeIndexes.end()){ // index is among requested indexes
            TypedValue attributeValue;
            convertByteArrayToTypedValue(type , startOffset,data , &attributeValue);
            typedValues.push_back(attributeValue);
            // also must move the startOffset forward
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , startOffset ,data);
            startOffset += numberOfBytesToSkip;
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , startOffset , data );
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
    for(int i=0; i < schema->getNumberOfNonSearchableAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
            ASSERT(type == ATTRIBUTE_TYPE_UNSIGNED);
            if(type != ATTRIBUTE_TYPE_UNSIGNED) return 0; // zero is returned in case the type is not unsigned
            return convertByteArrayToUnsigned(startOffset,data);
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , startOffset,data );
            startOffset += numberOfBytesToSkip;
        }
    }
    ASSERT(false);
    return 0;

}
float VariableLengthAttributeContainer::getFloatAttribute(const unsigned nonSearchableAttributeIndex,
        const Schema * schema, const Byte * data) {
    unsigned startOffset = 0;
    for(int i=0; i < schema->getNumberOfNonSearchableAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
            ASSERT(type == ATTRIBUTE_TYPE_FLOAT);
            return convertByteArrayToFloat(startOffset,data);
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , startOffset,data );
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
    for(int i=0; i < schema->getNumberOfNonSearchableAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
            ASSERT(type == ATTRIBUTE_TYPE_TEXT);
            if(type != ATTRIBUTE_TYPE_TEXT) return "";
            return convertByteArrayToString(type, startOffset,data);
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , startOffset, data );
            startOffset += numberOfBytesToSkip;
        }
    }
    ASSERT(false);
    return "";

}
long VariableLengthAttributeContainer::getTimeAttribute(const unsigned nonSearchableAttributeIndex,
        const Schema * schema, const Byte * data) {

    unsigned startOffset = 0;
    for(int i=0; i < schema->getNumberOfNonSearchableAttributes() ; i++){
        FilterType type = getAttributeType(i , schema);
        if(i == nonSearchableAttributeIndex) {// this is the wanted attribute
            ASSERT(type == ATTRIBUTE_TYPE_TIME);
            return convertByteArrayToLong( startOffset,data);
        }else{ // skip this attribute
            unsigned numberOfBytesToSkip = getSizeOfNonSearchableAttributeValueInData(type , startOffset, data );
            startOffset += numberOfBytesToSkip;
        }
    }
    ASSERT(false);
    return 0;

}

void VariableLengthAttributeContainer::allocate(const Schema * schema,
        const vector<string> & nonSearchableAttributeValues, Byte *& data, unsigned & dataSize) {

    unsigned totalLength = getSizeNeededForAllocation(schema , nonSearchableAttributeValues);
    //
    data = new Byte[totalLength];
    dataSize = totalLength;
}


FilterType VariableLengthAttributeContainer::getAttributeType(unsigned iter,
        const Schema * schema) {
    ASSERT(iter < schema->getNumberOfNonSearchableAttributes());
    return schema->getTypeOfNonSearchableAttribute(iter);
}



unsigned VariableLengthAttributeContainer::getSizeOfNonSearchableAttributeValueInData(
        FilterType filterType, unsigned startOffset, const Byte * data) {
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
        std::string value, Byte * output, unsigned startOffset,
        unsigned & sizeInBytes) {

    unsigned intValue = 0;
    float floatValue = 0;
    long longValue = 0;
    int i=0;
    switch (type) {
    case ATTRIBUTE_TYPE_UNSIGNED:
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
    }

    return result;
}

void VariableLengthAttributeContainer::convertByteArrayToTypedValue(FilterType type,
        unsigned startOffset, const Byte * data, TypedValue * result) {
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
    }

}

}
}

