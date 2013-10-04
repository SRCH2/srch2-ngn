
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
		ASSERT(valueType == typedValue.valueType);
    	switch (valueType) {
			case ATTRIBUTE_TYPE_UNSIGNED:
				return intTypedValue == typedValue.intTypedValue;
			case ATTRIBUTE_TYPE_FLOAT:
				return floatTypedValue == typedValue.floatTypedValue;
			case ATTRIBUTE_TYPE_TEXT:
				return (stringTypedValue.compare(typedValue.stringTypedValue) == 0);
			case ATTRIBUTE_TYPE_TIME:
				return timeTypedValue == typedValue.timeTypedValue;
			case ATTRIBUTE_TYPE_DURATION:
				ASSERT(false);
				break;
		}
    	return false;
	}
	bool TypedValue::operator!=(const TypedValue& typedValue) const{
		ASSERT(valueType == typedValue.valueType);
		return !(*this == typedValue);
	}

	bool TypedValue::operator<(const TypedValue& typedValue) const{
		ASSERT(valueType == typedValue.valueType);
    	switch (valueType) {
			case ATTRIBUTE_TYPE_UNSIGNED:
				return intTypedValue < typedValue.intTypedValue;
			case ATTRIBUTE_TYPE_FLOAT:
				return floatTypedValue < typedValue.floatTypedValue;
			case ATTRIBUTE_TYPE_TEXT:
				return stringTypedValue < typedValue.stringTypedValue;
			case ATTRIBUTE_TYPE_TIME:
				return timeTypedValue < typedValue.timeTypedValue;
			case ATTRIBUTE_TYPE_DURATION:
				ASSERT(false);
				break;
		}
    	return false;
	}

	bool TypedValue::operator<=(const TypedValue& typedValue) const {
		ASSERT(valueType == typedValue.valueType);
		return (*this < typedValue) || (*this == typedValue);
	}

	bool TypedValue::operator>(const TypedValue& typedValue) const{
		ASSERT(valueType == typedValue.valueType);
		return !(*this <= typedValue);
	}

	bool TypedValue::operator>=(const TypedValue& typedValue) const{
		ASSERT(valueType == typedValue.valueType);
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
	TimeDuration TypedValue::getTimeDuration() const{
		return timeDurationTypedValue;
	}

	TypedValue TypedValue::minimumValue(){
		TypedValue result ;
		switch (valueType) {
				case ATTRIBUTE_TYPE_UNSIGNED:
					result.setTypedValue((unsigned)std::numeric_limits<unsigned>::min());
					break;
				case ATTRIBUTE_TYPE_FLOAT:
					result.setTypedValue((float)std::numeric_limits<float>::min());
					break;
				case ATTRIBUTE_TYPE_TEXT:
					result.setTypedValue("NO_MINIMUM_FOR_TEXT");
					break;
				case ATTRIBUTE_TYPE_TIME:
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
					// it returns the value as the number of seconds in this duration
					result = getTimeDuration() + (long)0 ;
					ASSERT(false);
					break;
			}

		return result;
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
			default:
				ss << "";
				break;
		}
    	return ss.str();
	}


    }


}
