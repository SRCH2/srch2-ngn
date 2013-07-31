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


namespace srch2
{
namespace instantsearch
{

VariableLengthAttributeContainer::VariableLengthAttributeContainer(){
	data = NULL;
	dataSize = 0;
}

void VariableLengthAttributeContainer::init(const Schema * schema){
	unsigned totalLength = 0;

		for(int i=0;i<schema->getNumberOfNonSearchableAttributes();++i){ // iterate on attributes in schema
			// find the type of ith attribute
			FilterType type = getAttributeType(i,schema);
			//
			switch (type) {
				case UNSIGNED:
					totalLength += sizeof(unsigned);
					break;
				case FLOAT:
					totalLength += sizeof(float);
					break;
				case TEXT:
					totalLength += (sizeof(unsigned) + 0 ); // at this point we only allocate memory for storing the size of string
					break;
				case TIME:
					totalLength += sizeof(long);
					break;
				default:
					ASSERT(false);
					break;
			}
		}
		//
		data = new unsigned char[totalLength];
		for(int i=0;i<totalLength ; i++){
			data[i] = 0 ;
		}
		dataSize = totalLength;
}


VariableLengthAttributeContainer::~VariableLengthAttributeContainer(){
	; // TODO
}

void VariableLengthAttributeContainer::setAttribute(unsigned iter , const Schema * schema , std::string value){

	if(data == NULL) init(schema);
	FilterType attributeType = getAttributeType(iter,schema);
	setAttribute(iter, schema, convertStringToCharVector(attributeType, value));

}

std::string VariableLengthAttributeContainer::getAttribute(unsigned iter, const Schema * schema ) const{
	std::vector<unsigned char> charVectorValue;
	getAttribute(iter, schema, charVectorValue);
	return convertCharVectorToString(getAttributeType(iter, schema) , charVectorValue);
}

// gets the attribute value wrapped in a Score object
void VariableLengthAttributeContainer::getAttribute(unsigned iter, const Schema * schema , Score * score) const{
	std::vector<unsigned char> charVectorValue;
	getAttribute(iter, schema, charVectorValue);
	convertCharVectorToScore(getAttributeType(iter, schema) , charVectorValue , score);
}


// TODO TODO TODO this function is not tested
// gets values of attributes in iters in Score objects. iters must be ascending.
void VariableLengthAttributeContainer::getBatchOfAttributes(std::vector<unsigned> iters , const Schema * schema , std::vector<Score> * scores) const{

	std::vector<std::vector<unsigned char> > charVectorValues;
	getBatchOfAttributes(iters, schema, charVectorValues);
	int i=0;
	for(std::vector<std::vector<unsigned char> >::iterator attrIter = charVectorValues.begin();
			attrIter != charVectorValues.end(); ++attrIter){

		Score score;
		convertCharVectorToScore(getAttributeType(iters.at(i), schema) , *attrIter , &score);

		scores->push_back(score);

		//
		++i;
	}
}

unsigned VariableLengthAttributeContainer::getUnsignedAttribute(unsigned iter , const Schema * schema) const{
	FilterType attributeType = getAttributeType(iter,schema);
	ASSERT(attributeType == UNSIGNED);
	if(attributeType != UNSIGNED) return 0;
	std::vector<unsigned char> charVectorValue;
	getAttribute(iter, schema, charVectorValue);
	return convertCharVectorToUnsigned(charVectorValue, charVectorValue.begin(), charVectorValue.end());

}
float VariableLengthAttributeContainer::getFloatAttribute(unsigned iter , const Schema * schema) const{
	FilterType attributeType = getAttributeType(iter,schema);
	ASSERT(attributeType == FLOAT);
	if(attributeType != FLOAT) return 0;
	std::vector<unsigned char> charVectorValue;
	getAttribute(iter, schema, charVectorValue);
	return convertCharVectorToFloat(charVectorValue, charVectorValue.begin(), charVectorValue.end());
}
double VariableLengthAttributeContainer::getDoubleAttribute(unsigned iter, const Schema * schema) const{
	return (double)getFloatAttribute(iter, schema);
}
std::string VariableLengthAttributeContainer::getTextAttribute(unsigned iter , const Schema * schema) const{

	FilterType attributeType = getAttributeType(iter,schema);
	ASSERT(attributeType == TEXT);
	if(attributeType != TEXT) return 0;
	std::vector<unsigned char> charVectorValue;
	getAttribute(iter, schema, charVectorValue);
	return convertCharVectorToString(attributeType , charVectorValue);

}
long VariableLengthAttributeContainer::getTimeAttribute(unsigned iter , const Schema * schema) const{

	FilterType attributeType = getAttributeType(iter,schema);
	ASSERT(attributeType == TIME);
	if(attributeType != TIME) return 0;
	std::vector<unsigned char> charVectorValue;
	getAttribute(iter, schema, charVectorValue);
	return convertCharVectorToLong(charVectorValue, charVectorValue.begin(), charVectorValue.end());

}






void VariableLengthAttributeContainer::setAttribute(unsigned iter, const Schema * schema, std::vector<unsigned char> attributeValue){
	ASSERT(data != NULL);

	std::vector<unsigned char> newData;
	unsigned startIndex = 0;
	// this flag is set to true if the following for changes the total length of data
	// We need to know this to see if we need to delete/reallocate the char array
	bool totalLengthChanged = false;

	for (unsigned i=0;i<schema->getNumberOfNonSearchableAttributes();++i){ // iterate on attributes from schema
				// find the type of ith attribute
				FilterType type = getAttributeType(i,schema);

				if(i == iter){ // updating this attribute's value
					unsigned oldStartIndex= startIndex;
					processNextAttribute(type, data, startIndex, true, attributeValue, newData, startIndex);
					if ((startIndex - oldStartIndex) != attributeValue.size()){
						if (type == TEXT){
							if ((startIndex - oldStartIndex) !=  (attributeValue.size()+sizeof(unsigned)) ){ // length changes
								totalLengthChanged = true;
							}
						}else{ // length changes
							totalLengthChanged = true;
						}
					}
				}else{
					processNextAttribute(type, data, startIndex, false, attributeValue, newData, startIndex);
				}
	}
	// now the newData vector contains new data, it should be converted to array.
	if(totalLengthChanged){
		delete data;
		data = new unsigned char[newData.size()];
		dataSize = newData.size();
	}

	unsigned i =0;
	for (std::vector<unsigned char>::iterator newDataIter = newData.begin(); newDataIter != newData.end(); ++newDataIter){
		data[i] = *newDataIter;
		i++;
	}

}




FilterType VariableLengthAttributeContainer::getAttributeType(unsigned iter, const Schema * schema) const{
	ASSERT(iter < schema->getNumberOfNonSearchableAttributes());
//	FilterType types[10];
//	types[0]= TEXT;
//	types[1]= UNSIGNED;
//	types[2]= TIME;
//	types[3] = TEXT;
//	types[4] = FLOAT;
//	types[5] = UNSIGNED;
//	types[6] = TEXT;
//	types[7] = TEXT;
//	types[8] = UNSIGNED;
//	types[9] = UNSIGNED;
//
//	return types[iter];

	return schema->getTypeOfNonSearchableAttribute(iter);
}


void VariableLengthAttributeContainer::getAttribute(unsigned iter, const Schema * schema, std::vector<unsigned char>& attributeValue) const{
	ASSERT(data != NULL);

	unsigned startIndex = 0;

	for (unsigned i=0;i<schema->getNumberOfNonSearchableAttributes();++i){ // iterate on attributes from schema
				// find the type of ith attribute
				FilterType type = getAttributeType(i,schema);

				if(i == iter){ // get the value of this attribute
					std::vector<unsigned char> temp;
					processNextAttribute(type, data, startIndex, false, attributeValue, temp, startIndex);
					std::vector<unsigned char>::iterator valueIter = temp.begin();
					if(type == TEXT){
						for(unsigned c = 0 ; c < sizeof(unsigned) ; c++){
							valueIter ++;
						}
					}
					attributeValue.insert(attributeValue.end(), valueIter, temp.end());
					break;
				}else{ // move over this attribute
					std::vector<unsigned char> temp;
					processNextAttribute(type, data, startIndex, false, attributeValue, temp, startIndex);
				}
	}

}

void VariableLengthAttributeContainer::getBatchOfAttributes(std::vector<unsigned> iters,
		const Schema * schema, std::vector<std::vector<unsigned char> >& attributeValues) const{
	ASSERT(data != NULL);

	unsigned attributeItersIndex = 0;
	if(iters.size() == 0){
		return; // no attribute to read
	}

	unsigned startIndex = 0;

	for (unsigned i=0;i<schema->getNumberOfNonSearchableAttributes();++i){ // iterate on attributes from schema
				// find the type of ith attribute
				FilterType type = getAttributeType(i,schema);

				if(i == iters.at(attributeItersIndex)){ // get the value of this attribute
					std::vector<unsigned char> temp;
					std::vector<unsigned char> attributeValue;
					processNextAttribute(type, data, startIndex, false, attributeValue, temp, startIndex);
					std::vector<unsigned char>::iterator valueIter = temp.begin();
					if(type == TEXT){
						for(unsigned c = 0 ; c < sizeof(unsigned) ; c++){
							valueIter ++;
						}
					}
					attributeValue.insert(attributeValue.end(), valueIter, temp.end());
					attributeValues.push_back(attributeValue);
					attributeItersIndex ++ ;
					if(attributeItersIndex == iters.size()){
						break;
					}
				}else{ // move over this attribute
					std::vector<unsigned char> temp;
					std::vector<unsigned char> attributeValue;
					processNextAttribute(type, data, startIndex, false, attributeValue, temp, startIndex);
				}
	}

}


// TODO: OPTIMIZATION : we can add a flag to the header of this function which tells it to just move the cursor and
// not do the copy.
void VariableLengthAttributeContainer::processNextAttribute(FilterType attributeType, const unsigned char * currentData, unsigned startIndex, bool updateFlag,
		std::vector<unsigned char> newAttributeValue, std::vector<unsigned char>& newData , unsigned& newStartIndex ) const{

	if(updateFlag){ // updating this attribute with a new value
		if(attributeType == TEXT){ // if it is text first add the length
			std::vector<unsigned char> valueLengthVector = convertUnsignedToCharVector(newAttributeValue.size());
			newData.insert(newData.end(), valueLengthVector.begin(), valueLengthVector.end());
		}
		// now add the value to the new vector
		newData.insert(newData.end(), newAttributeValue.begin(), newAttributeValue.end());
	}

	// no matter it is update or not we should move the index forward
	// but if it's not update we should copy the old data into the new vector
	unsigned valueLength = 0;
	std::vector<unsigned char> currentDataVector;
	switch (attributeType) {
		case UNSIGNED:
			valueLength = sizeof(unsigned);
			break;
		case FLOAT:
			valueLength = sizeof(float);
			break;
		case TIME:
			valueLength = sizeof(long);
			break;
		case TEXT:
			for(unsigned i=startIndex ; i < startIndex+sizeof(unsigned) ; i++){
				currentDataVector.push_back(currentData[i]);
			}
			unsigned textSize = convertCharVectorToUnsigned(currentDataVector, currentDataVector.begin(), currentDataVector.end());
			valueLength = sizeof(unsigned) + textSize; // |value_length|text_value|
			break;
	}

	if(!updateFlag){
		for(int i=startIndex;i<startIndex+valueLength; ++i){
			newData.push_back(currentData[i]);
		}
	}
	newStartIndex = startIndex+valueLength;


}



std::vector<unsigned char> VariableLengthAttributeContainer::convertUnsignedToCharVector(unsigned input) const{

	unsigned char * p = reinterpret_cast<unsigned char *>(&input);
	std::vector<unsigned char> result;
	for(int i=0 ; i < sizeof(unsigned);i++){
		result.push_back(p[i]);
	}
	return result;
}
unsigned VariableLengthAttributeContainer::convertCharVectorToUnsigned(
		std::vector<unsigned char> input ,std::vector<unsigned char>::iterator start, std::vector<unsigned char>::iterator end) const{

	unsigned char * temp = new unsigned char[sizeof(unsigned)];
	unsigned i=0;
	for(std::vector<unsigned char>::iterator iter = start; iter != end; ++iter){
		temp[i] = *iter;
		i++;
	}
	unsigned * resultP = (unsigned *) temp;
	unsigned result = *resultP;
	delete temp;
	return result;
}


// Converting float and char vector together.
std::vector<unsigned char> VariableLengthAttributeContainer::convertFloatToCharVector(float input) const{

	std::vector<unsigned char> result;
	unsigned char const * p = reinterpret_cast<unsigned char const *>(&input);
	for(int i=0 ;i < sizeof(float);i++){
		result.push_back(p[i]);
	}
	return result;
}
float VariableLengthAttributeContainer::convertCharVectorToFloat(std::vector<unsigned char> input , std::vector<unsigned char>::iterator start, std::vector<unsigned char>::iterator end) const{

	unsigned char * temp = new unsigned char[sizeof(float)];
	unsigned i=0;
	for(std::vector<unsigned char>::iterator iter = start; iter != end; ++iter){
		temp[i] = *iter;
		i++;
	}
	float * resultP = (float *) temp;
	float result = *resultP;
	delete temp;
	return result;
}

// Converting long and char vector together.
std::vector<unsigned char> VariableLengthAttributeContainer::convertLongToCharVector(long input) const{
	std::vector<unsigned char> result;
	unsigned char const * p = reinterpret_cast<unsigned char const *>(&input);
	for(int i=0 ;i < sizeof(long);i++){
		result.push_back(p[i]);
	}
	return result;
}
long VariableLengthAttributeContainer::convertCharVectorToLong(std::vector<unsigned char> input , std::vector<unsigned char>::iterator start, std::vector<unsigned char>::iterator end) const{
	unsigned char * temp = new unsigned char[sizeof(long)];
	unsigned i=0;
	for(std::vector<unsigned char>::iterator iter = start; iter != end; ++iter){
		temp[i] = *iter;
		i++;
	}
	long * resultP = (long *) temp;
	long result = *resultP;
	delete temp;
	return result;
}




std::vector<unsigned char> VariableLengthAttributeContainer::convertStringToCharVector(FilterType type, std::string value) const{

	std::vector<unsigned char> result;
	unsigned intValue = 0;
	float floatValue = 0;
	long longValue = 0;
	switch (type) {
		case UNSIGNED:
			intValue = atoi(value.c_str());
			result = convertUnsignedToCharVector(intValue);
			break;
		case FLOAT:
			floatValue = atof(value.c_str());
			result = convertFloatToCharVector(floatValue);
			break;
		case TEXT:
			for(std::string::iterator it = value.begin(); it != value.end(); ++it) {
			    result.push_back(*it);
			}
			break;
		case TIME:
			longValue = atol(value.c_str());
			result = convertLongToCharVector(longValue);
			break;
		default:
			ASSERT(false);
			break;
	}

	return result;

}

std::string VariableLengthAttributeContainer::convertCharVectorToString(FilterType type, std::vector<unsigned char> value) const{
	std::string result = "";
	unsigned intValue = 0;
	float floatValue = 0;
	long longValue = 0;
	std::stringstream ss;
	switch (type) {
		case UNSIGNED:
			intValue = convertCharVectorToUnsigned(value, value.begin(),value.end());
			ss << intValue;
			result = ss.str();
			break;
		case FLOAT:
			floatValue = convertCharVectorToFloat(value, value.begin(), value.end());
			ss << floatValue;
			result = ss.str();
			break;
		case TEXT:
			ss << "";
			for(std::vector<unsigned char>::iterator it = value.begin(); it != value.end(); ++it) {
			    ss << (char)*it; // TODO: is it safe to cast it to char
			}
			result = ss.str();
			break;
		case TIME:
			longValue = convertCharVectorToLong(value, value.begin(), value.end());
			ss << longValue;
			result = ss.str();
			break;
		default:
			ASSERT(false);
			break;
	}

	return result;
}


void VariableLengthAttributeContainer::convertCharVectorToScore(FilterType type, std::vector<unsigned char> value , Score * result) const{
	unsigned intValue = 0;
	float floatValue = 0;
	long longValue = 0;
	std::stringstream ss;
	switch (type) {
		case UNSIGNED:
			intValue = convertCharVectorToUnsigned(value, value.begin(),value.end());
			result->setScore(intValue);
			break;
		case FLOAT:
			floatValue = convertCharVectorToFloat(value, value.begin(), value.end());
			result->setScore(floatValue);
			break;
		case TEXT:
			ss << "";
			for(std::vector<unsigned char>::iterator it = value.begin(); it != value.end(); ++it) {
			    ss << (char)*it; // TODO: is it safe to cast it to char
			}
			result->setScore(ss.str());

			break;
		case TIME:
			longValue = convertCharVectorToLong(value, value.begin(), value.end());
			result->setScore(longValue);
			break;
		default:
			ASSERT(false);
			break;
	}

}

}
}

