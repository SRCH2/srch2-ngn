
// $Id: Score.cpp 2013-06-19 02:11:13Z Jamshid $

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

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */



#include <instantsearch/TypedValue.h>
#include <sstream>
#include <cstdlib>
#include "util/Assert.h"
#include <limits>
#include <cmath>
#include <algorithm>
#include "util/DateAndTimeHandler.h"

namespace srch2
{
    namespace instantsearch
    {



	bool TypedValue::operator==(const TypedValue& typedValue) const{
    	switch (valueType) {
			case ATTRIBUTE_TYPE_UNSIGNED:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_UNSIGNED:
						return intTypedValue == typedValue.intTypedValue;
					case ATTRIBUTE_TYPE_MULTI_UNSIGNED:{
						vector<unsigned> values = typedValue.getMultiIntTypedValue();
						return std::find(values.begin() , values.end() , intTypedValue) != values.end();
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_FLOAT:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_FLOAT:
						return floatTypedValue == typedValue.floatTypedValue;
					case ATTRIBUTE_TYPE_MULTI_FLOAT:{
						vector<float> values = typedValue.getMultiFloatTypedValue();
						return std::find(values.begin() , values.end() , floatTypedValue) != values.end();
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_TEXT:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_TEXT:
						return (stringTypedValue.compare(typedValue.stringTypedValue) == 0);
					case ATTRIBUTE_TYPE_MULTI_TEXT:{
						vector<string> values = typedValue.getMultiTextTypedValue();
						return std::find(values.begin() , values.end() , stringTypedValue) != values.end();
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_TIME:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_TIME:
						return timeTypedValue == typedValue.timeTypedValue;
					case ATTRIBUTE_TYPE_MULTI_TIME:{
						vector<long> values = typedValue.getMultiTimeTypedValue();
						return std::find(values.begin() , values.end() , timeTypedValue) != values.end();
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_MULTI_UNSIGNED:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_UNSIGNED:{
						vector<unsigned> values = this->getMultiIntTypedValue();
						return std::find(values.begin() , values.end() , typedValue.intTypedValue) != values.end();
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_MULTI_FLOAT:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_FLOAT:{
						vector<float> values = this->getMultiFloatTypedValue();
						return std::find(values.begin() , values.end() , typedValue.floatTypedValue) != values.end();
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_MULTI_TEXT:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_TEXT:{
						vector<string> values = this->getMultiTextTypedValue();
						return std::find(values.begin() , values.end() , typedValue.stringTypedValue) != values.end();
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_MULTI_TIME:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_TIME:{
						vector<long> values = this->getMultiTimeTypedValue();
						return std::find(values.begin() , values.end() , typedValue.timeTypedValue) != values.end();
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_DURATION:
				ASSERT(false);
				break;
		}
    	return false;
	}
	bool TypedValue::operator!=(const TypedValue& typedValue) const{
		return !(*this == typedValue);
	}

	bool TypedValue::operator<(const TypedValue& typedValue) const{
    	switch (valueType) {
			case ATTRIBUTE_TYPE_UNSIGNED:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_UNSIGNED:
						return intTypedValue < typedValue.intTypedValue;
					case ATTRIBUTE_TYPE_MULTI_UNSIGNED:{
						vector<unsigned> values = typedValue.getMultiIntTypedValue();
						for(vector<unsigned>::iterator value = values.begin() ; value != values.end() ; ++value){
							if(intTypedValue < *value) return true;
						}
						return false;
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_FLOAT:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_FLOAT:
						return floatTypedValue < typedValue.floatTypedValue;
					case ATTRIBUTE_TYPE_MULTI_FLOAT:{
						vector<float> values = typedValue.getMultiFloatTypedValue();
						for(vector<float>::iterator value = values.begin() ; value != values.end() ; ++value){
							if(floatTypedValue < *value) return true;
						}
						return false;
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_TEXT:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_TEXT:
						return stringTypedValue < typedValue.stringTypedValue;
					case ATTRIBUTE_TYPE_MULTI_TEXT:{
						vector<string> values = typedValue.getMultiTextTypedValue();
						for(vector<string>::iterator value = values.begin() ; value != values.end() ; ++value){
							if(stringTypedValue < *value) return true;
						}
						return false;
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_TIME:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_TIME:
						return timeTypedValue < typedValue.timeTypedValue;
					case ATTRIBUTE_TYPE_MULTI_TIME:{
						vector<long> values = typedValue.getMultiTimeTypedValue();
						for(vector<long>::iterator value = values.begin() ; value != values.end() ; ++value){
							if(timeTypedValue < *value) return true;
						}
						return false;
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_MULTI_UNSIGNED:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_UNSIGNED:{
						vector<unsigned> values = this->getMultiIntTypedValue();
						for(vector<unsigned>::iterator value = values.begin() ; value != values.end() ; ++value){
							if(*value < typedValue.intTypedValue) return true;
						}
						return false;
					}
					case ATTRIBUTE_TYPE_MULTI_UNSIGNED:{
						vector<unsigned> valuesThis = this->getMultiIntTypedValue();
						vector<unsigned> valuesInput = typedValue.getMultiIntTypedValue();
						for(unsigned valueIndex=0 ; valueIndex != valuesThis.size() ; ++valueIndex){
							if(valueIndex >= valuesInput.size()){
								return false;
							}
							if(valuesThis.at(valueIndex) < valuesInput.at(valueIndex)) return true;
						}
						return true;
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_MULTI_FLOAT:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_FLOAT:{
						vector<float> values = this->getMultiFloatTypedValue();
						for(vector<float>::iterator value = values.begin() ; value != values.end() ; ++value){
							if(*value < typedValue.floatTypedValue) return true;
						}
						return false;
					}
					case ATTRIBUTE_TYPE_MULTI_FLOAT:{
						vector<float> valuesThis = this->getMultiFloatTypedValue();
						vector<float> valuesInput = typedValue.getMultiFloatTypedValue();
						for(unsigned valueIndex=0 ; valueIndex != valuesThis.size() ; ++valueIndex){
							if(valueIndex >= valuesInput.size()){
								return false;
							}
							if(valuesThis.at(valueIndex) < valuesInput.at(valueIndex)) return true;
						}
						return true;
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_MULTI_TEXT:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_TEXT:{
						vector<string> values = this->getMultiTextTypedValue();
						for(vector<string>::iterator value = values.begin() ; value != values.end() ; ++value){
							if(*value < typedValue.stringTypedValue) return true;
						}
						return false;
					}
					case ATTRIBUTE_TYPE_MULTI_TEXT:{
						vector<string> valuesThis = this->getMultiTextTypedValue();
						vector<string> valuesInput = typedValue.getMultiTextTypedValue();
						for(unsigned valueIndex=0 ; valueIndex != valuesThis.size() ; ++valueIndex){
							if(valueIndex >= valuesInput.size()){
								return false;
							}
							if(valuesThis.at(valueIndex) < valuesInput.at(valueIndex)) return true;
						}
						return true;
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_MULTI_TIME:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_TIME:{
						vector<long> values = this->getMultiTimeTypedValue();
						for(vector<long>::iterator value = values.begin() ; value != values.end() ; ++value){
							if(*value < typedValue.timeTypedValue) return true;
						}
						return false;
					}
					case ATTRIBUTE_TYPE_MULTI_TIME:{
						vector<long> valuesThis = this->getMultiTimeTypedValue();
						vector<long> valuesInput = typedValue.getMultiTimeTypedValue();
						for(unsigned valueIndex=0 ; valueIndex != valuesThis.size() ; ++valueIndex){
							if(valueIndex >= valuesInput.size()){
								return false;
							}
							if(valuesThis.at(valueIndex) < valuesInput.at(valueIndex)) return true;
						}
						return true;
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_DURATION:
				ASSERT(false);
				break;
		}
    	return false;
	}

	bool TypedValue::operator<=(const TypedValue& typedValue) const {
		return (*this < typedValue) || (*this == typedValue);
	}

	bool TypedValue::operator>(const TypedValue& typedValue) const{
		return !(*this <= typedValue);
	}

	bool TypedValue::operator>=(const TypedValue& typedValue) const{
		return !(*this < typedValue);
	}
	TypedValue TypedValue::operator+(const TypedValue& typedValue){
    	TypedValue result;
    	// Since order of operands is important for operator +, these two if-statements are needed to
    	// enable both orders of time + duration and duration + time
		if(typedValue.valueType == ATTRIBUTE_TYPE_TIME && this->valueType == ATTRIBUTE_TYPE_DURATION){
			result.setTypedValue(this->getTimeDuration() + typedValue.getTimeTypedValue());
			return result;
		}
		if(this->valueType == ATTRIBUTE_TYPE_TIME && typedValue.valueType == ATTRIBUTE_TYPE_DURATION){
			result.setTypedValue(typedValue.getTimeDuration() + this->getTimeTypedValue());
			return result;
		}
		ASSERT(typedValue.valueType == this->valueType);
		ASSERT(this->valueType != ATTRIBUTE_TYPE_MULTI_UNSIGNED);
		ASSERT(this->valueType != ATTRIBUTE_TYPE_MULTI_FLOAT);
		ASSERT(this->valueType != ATTRIBUTE_TYPE_MULTI_TEXT);
		ASSERT(this->valueType != ATTRIBUTE_TYPE_MULTI_TIME);
    	switch (this->valueType) {
			case ATTRIBUTE_TYPE_UNSIGNED:
				result.setTypedValue(typedValue.getIntTypedValue() + this->getIntTypedValue());
				break;
			case ATTRIBUTE_TYPE_FLOAT:
				result.setTypedValue(typedValue.getFloatTypedValue() + this->getFloatTypedValue());
				break;
			case ATTRIBUTE_TYPE_TEXT:
				result.setTypedValue(typedValue.getTextTypedValue() + this->getTextTypedValue());
				break;
			case ATTRIBUTE_TYPE_TIME:
				result.setTypedValue(typedValue.getTimeTypedValue() + this->getTimeTypedValue());
				break;
			case ATTRIBUTE_TYPE_DURATION:
				result.setTypedValue(typedValue.getTimeDuration() + this->getTimeDuration());
				break;
			default:
				break;
		}

    	return result;
	}




	void TypedValue::setTypedValue(unsigned intTypedValue){
		valueType = ATTRIBUTE_TYPE_UNSIGNED;
		this->intTypedValue = intTypedValue;
	}
	void TypedValue::setTypedValue(float floatTypedValue){
		valueType = ATTRIBUTE_TYPE_FLOAT;
		this->floatTypedValue = floatTypedValue;
	}
	void TypedValue::setTypedValue(double doubleTypedValue){
		setTypedValue((float)doubleTypedValue);
	}
	void TypedValue::setTypedValue(string stringTypedValue){
		valueType = ATTRIBUTE_TYPE_TEXT;
		this->stringTypedValue = stringTypedValue;
	}
	void TypedValue::setTypedValue(long timeTypedValue){
		valueType = ATTRIBUTE_TYPE_TIME;
		this->timeTypedValue = timeTypedValue;
	}

	void TypedValue::setTypedValue(vector< unsigned> intTypeValue){
		valueType = ATTRIBUTE_TYPE_MULTI_UNSIGNED;
		this->intTypedValueMulti = intTypeValue;
	}
	void TypedValue::setTypedValue(vector<float> floatTypeValue){
		valueType = ATTRIBUTE_TYPE_MULTI_FLOAT;
		this->floatTypedValueMulti = floatTypeValue;
	}
	void TypedValue::setTypedValue(vector<string> stringTypeValue){
		valueType = ATTRIBUTE_TYPE_MULTI_TEXT;
		this->stringTypedValueMulti = stringTypeValue;
	}
	void TypedValue::setTypedValue(vector<long> timeTypeValue){
		valueType = ATTRIBUTE_TYPE_MULTI_TIME;
		this->timeTypedValueMulti = timeTypeValue;
	}


	void TypedValue::setTypedValue(const TimeDuration & durationTypedValue){
		valueType = ATTRIBUTE_TYPE_DURATION;
		this->timeDurationTypedValue = durationTypedValue;
	}

	void TypedValue::setTypedValue(const TypedValue& typedValue){
    	valueType = typedValue.valueType;
    	switch (valueType) {
			case ATTRIBUTE_TYPE_UNSIGNED:
				intTypedValue = typedValue.intTypedValue;
				break;
			case ATTRIBUTE_TYPE_FLOAT:
				floatTypedValue = typedValue.floatTypedValue;
				break;
			case ATTRIBUTE_TYPE_TEXT:
				stringTypedValue = typedValue.stringTypedValue;
				break;
			case ATTRIBUTE_TYPE_TIME:
				timeTypedValue = typedValue.timeTypedValue;
				break;
			case ATTRIBUTE_TYPE_DURATION:
				timeDurationTypedValue = typedValue.timeDurationTypedValue;
				break;
			case ATTRIBUTE_TYPE_MULTI_UNSIGNED:
				intTypedValueMulti = typedValue.intTypedValueMulti;
				break;
			case ATTRIBUTE_TYPE_MULTI_FLOAT:
				floatTypedValueMulti = typedValue.floatTypedValueMulti;
				break;
			case ATTRIBUTE_TYPE_MULTI_TEXT:
				stringTypedValueMulti = typedValue.stringTypedValueMulti;
				break;
			case ATTRIBUTE_TYPE_MULTI_TIME:
				timeTypedValueMulti = typedValue.timeTypedValueMulti;
				break;
		}
	}

	void TypedValue::setTypedValue(FilterType type , string value){
	    // TODO : do some validation to make sure engine does not crash.
	    //        NOTE: The input to this function is supposed to be validated once...
		this->valueType = type;
		switch (type) {
			case ATTRIBUTE_TYPE_UNSIGNED:
				this->setTypedValue((unsigned)atoi(value.c_str()));
				break;
			case ATTRIBUTE_TYPE_FLOAT:
				this->setTypedValue(atof(value.c_str()));
				break;
			case ATTRIBUTE_TYPE_TEXT:
				this->setTypedValue(value);
				break;
			case ATTRIBUTE_TYPE_TIME:
				this->setTypedValue(atol(value.c_str()));
				break;
			case ATTRIBUTE_TYPE_DURATION:
				this->setTypedValue(DateAndTimeHandler::convertDurationTimeStringToTimeDurationObject(value));
				break;
			default :
				ASSERT(false);
				break;
		}
	}

	unsigned TypedValue::getIntTypedValue() const{
		return intTypedValue;
	}
	float TypedValue::getFloatTypedValue() const{
		return floatTypedValue;
	}
	double TypedValue::getDoubleTypedValue() const{
		return (double)getFloatTypedValue();
	}
	string TypedValue::getTextTypedValue() const{
		return stringTypedValue;
	}
	long TypedValue::getTimeTypedValue() const{
		return timeTypedValue;
	}
	vector<unsigned> TypedValue::getMultiIntTypedValue() const{
		return intTypedValueMulti;
	}
	vector<float> TypedValue::getMultiFloatTypedValue() const{
		return floatTypedValueMulti;
	}
	vector<string> TypedValue::getMultiTextTypedValue() const{
		return stringTypedValueMulti;
	}
	vector<long> TypedValue::getMultiTimeTypedValue() const{
		return timeTypedValueMulti;
	}

	TimeDuration TypedValue::getTimeDuration() const{
		return timeDurationTypedValue;
	}

	TypedValue TypedValue::minimumValue(){
		TypedValue result ;
		switch (valueType) {
				case ATTRIBUTE_TYPE_UNSIGNED:
				case ATTRIBUTE_TYPE_MULTI_UNSIGNED:
					result.setTypedValue((unsigned)std::numeric_limits<unsigned>::min());
					break;
				case ATTRIBUTE_TYPE_FLOAT:
				case ATTRIBUTE_TYPE_MULTI_FLOAT:
					result.setTypedValue((float)std::numeric_limits<float>::min());
					break;
				case ATTRIBUTE_TYPE_TEXT:
				case ATTRIBUTE_TYPE_MULTI_TEXT:
					result.setTypedValue("NO_MINIMUM_FOR_TEXT");
					break;
				case ATTRIBUTE_TYPE_TIME:
				case ATTRIBUTE_TYPE_MULTI_TIME:
					result.setTypedValue((long)-1893456000); // This number is the number of seconds from Jan-1st, 1910
					break;
				case ATTRIBUTE_TYPE_DURATION:
					TimeDuration tD;
					result.setTypedValue(tD);
					break;
			}

		return result;
	}

	float TypedValue::castToFloat(){
		float result = 0;
		switch (valueType) {
				case ATTRIBUTE_TYPE_UNSIGNED:
					result = (float) getIntTypedValue();
					break;
				case ATTRIBUTE_TYPE_FLOAT:
					result = getFloatTypedValue();
					break;
				case ATTRIBUTE_TYPE_TEXT:
					result = -1;
					ASSERT(false);
					break;
				case ATTRIBUTE_TYPE_TIME:
					result = (float) getTimeTypedValue();
					break;
				case ATTRIBUTE_TYPE_DURATION:
				case ATTRIBUTE_TYPE_MULTI_UNSIGNED:
				case ATTRIBUTE_TYPE_MULTI_FLOAT:
				case ATTRIBUTE_TYPE_MULTI_TEXT:
				case ATTRIBUTE_TYPE_MULTI_TIME:
					// it returns the value as the number of seconds in this duration
					result = getTimeDuration() + (long)0 ;
					ASSERT(false);
					break;
			}

		return result;
	}

	void TypedValue::breakMultiValueIntoSingleValueTypedValueObjects(vector<TypedValue> * output) const{

		switch (this->valueType) {
			case ATTRIBUTE_TYPE_MULTI_UNSIGNED:
				for(vector<unsigned>::const_iterator value = intTypedValueMulti.begin(); value != intTypedValueMulti.end() ; ++value){
					TypedValue singleValue;
					singleValue.setTypedValue(*value);
					output->push_back(singleValue);
				}
				break;
			case ATTRIBUTE_TYPE_MULTI_FLOAT:
				for(vector<float>::const_iterator value = floatTypedValueMulti.begin(); value != floatTypedValueMulti.end() ; ++value){
					TypedValue singleValue;
					singleValue.setTypedValue(*value);
					output->push_back(singleValue);
				}
				break;
			case ATTRIBUTE_TYPE_MULTI_TEXT:
				for(vector<string>::const_iterator value = stringTypedValueMulti.begin(); value != stringTypedValueMulti.end() ; ++value){
					TypedValue singleValue;
					singleValue.setTypedValue(*value);
					output->push_back(singleValue);
				}
				break;
			case ATTRIBUTE_TYPE_MULTI_TIME:
				for(vector<long>::const_iterator value = timeTypedValueMulti.begin(); value != timeTypedValueMulti.end() ; ++value){
					TypedValue singleValue;
					singleValue.setTypedValue(*value);
					output->push_back(singleValue);
				}
				break;
			default:
				ASSERT(false);
				break;
		}
	}

	// Example
	// if start = 10, end = 100, gap = 10
	// this->value = 1 => returns 0
	// this->value = 10 => returns 1
	// this->value = 21 => returns 2
    unsigned TypedValue::findIndexOfContainingInterval(TypedValue & start , TypedValue & end, TypedValue & gap) const{
        float thisTypedValue = 0;
        float startTypedValue = 0;
        float endTypedValue = 0;
        float gapTypedValue = 0;
        switch (this->getType()) {
            case ATTRIBUTE_TYPE_UNSIGNED:
                // first bucket which covers less-than-start values is zero
                if(this->intTypedValue < start.getIntTypedValue()){
                    return 0;
                }
                thisTypedValue = this->getIntTypedValue();
                startTypedValue = start.getIntTypedValue();
                endTypedValue = end.getIntTypedValue();
                gapTypedValue = gap.getIntTypedValue();
                break;
            case ATTRIBUTE_TYPE_FLOAT:
                // first bucket which covers less-than-start values is zero
                if(this->floatTypedValue < start.getFloatTypedValue()){
                    return 0;
                }
                thisTypedValue = this->getFloatTypedValue();
                startTypedValue = start.getFloatTypedValue();
                endTypedValue = end.getFloatTypedValue();
                gapTypedValue = gap.getFloatTypedValue();
                break;
            case ATTRIBUTE_TYPE_TIME:
            {
            	// Since time is represented in the system as long but
            	// time duration is a class and used for gap, we can't use math to calculate the
            	// index so we add gap until the result is bigger than data point
            	ASSERT(gap.getType() == ATTRIBUTE_TYPE_DURATION && start.getType() == ATTRIBUTE_TYPE_TIME &&  end.getType() == ATTRIBUTE_TYPE_TIME);
                // first bucket which covers less-than-start values is zero
            	if(this->getTimeTypedValue() < start.getTimeTypedValue()){
            		return 0;
            	}
            	long currentLowerBound = start.getTimeTypedValue();
            	unsigned indexToReturn = 1;
            	while(true){
            		currentLowerBound = gap.getTimeDuration() + currentLowerBound;
            		if(this->getTimeTypedValue() < currentLowerBound || currentLowerBound >= end.getTimeTypedValue()){
            			break;
            		}
            		//
            		indexToReturn ++;
            	}
            	return indexToReturn;
            	break;
            }
            default:
                ASSERT(false);
                return -1; // invalid group id
        }
        // generating group id for intervals, the buckets given to values greater than start are grater than or equal to 1
        return floor((thisTypedValue - startTypedValue) / gapTypedValue) + 1 ;
    }


	vector<unsigned> TypedValue::findIndicesOfContainingIntervals(TypedValue & start , TypedValue & end, TypedValue & gap) const{
		vector<unsigned> result;
		// move on all single values and find the index for each one of them
		vector<TypedValue> singleValues;
		breakMultiValueIntoSingleValueTypedValueObjects(&singleValues);
		for(vector<TypedValue>::iterator singleValue = singleValues.begin() ; singleValue != singleValues.end() ; ++singleValue){
			result.push_back(singleValue->findIndexOfContainingInterval(start  , end , gap));
		}
		return result;
	}

    TypedValue::TypedValue(const TypedValue& typedValue){
    	setTypedValue(typedValue);
    }

	string TypedValue::toString()  const{
		std::stringstream ss;

    	switch (valueType) {
			case ATTRIBUTE_TYPE_UNSIGNED:
				ss << intTypedValue ;
				break;
			case ATTRIBUTE_TYPE_FLOAT:
				ss << floatTypedValue;
				break;
			case ATTRIBUTE_TYPE_TEXT:
				ss << stringTypedValue ;
				break;
			case ATTRIBUTE_TYPE_TIME:

				ss << timeTypedValue ;
				break;
			case ATTRIBUTE_TYPE_DURATION:
				ss << this->getTimeDuration().toString();
				break;
			case ATTRIBUTE_TYPE_MULTI_UNSIGNED:
				for(vector<unsigned>::const_iterator value = intTypedValueMulti.begin() ; value != intTypedValueMulti.end() ; ++value){
					if(value == intTypedValueMulti.begin()){
						ss << *value ;
					}else{
						ss << "," << *value ;
					}
				}
				break;
			case ATTRIBUTE_TYPE_MULTI_FLOAT:
				for(vector<float>::const_iterator value = floatTypedValueMulti.begin() ; value != floatTypedValueMulti.end() ; ++value){
					if(value == floatTypedValueMulti.begin()){
						ss << *value ;
					}else{
						ss << "," << *value ;
					}
				}
				break;
			case ATTRIBUTE_TYPE_MULTI_TEXT:
				for(vector<string>::const_iterator value = stringTypedValueMulti.begin() ; value != stringTypedValueMulti.end() ; ++value){
					if(value == stringTypedValueMulti.begin()){
						ss << *value ;
					}else{
						ss << "," << *value ;
					}
				}
				break;
			case ATTRIBUTE_TYPE_MULTI_TIME:
				for(vector<long>::const_iterator value = timeTypedValueMulti.begin() ; value != timeTypedValueMulti.end() ; ++value){
					if(value == timeTypedValueMulti.begin()){
						ss << *value ;
					}else{
						ss << "," << *value ;
					}
				}
				break;
			default:
				ss << "";
				break;
		}
    	return ss.str();
	}


    }


}
