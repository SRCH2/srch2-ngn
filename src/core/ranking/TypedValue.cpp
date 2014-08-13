
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
			case ATTRIBUTE_TYPE_INT:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_INT:
					// comparing single-valued vs. single-valued : Example : 1 == 1
						return intTypedValue == typedValue.intTypedValue;
					case ATTRIBUTE_TYPE_MULTI_INT:{
				    	// comparing single-valued vs. multi-valued. Example : 1 == <1,3,23>
						vector<int> values = typedValue.getMultiIntTypedValue();
						return std::find(values.begin() , values.end() , intTypedValue) != values.end();
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
            case ATTRIBUTE_TYPE_LONG:{

                switch (typedValue.valueType) {
                    case ATTRIBUTE_TYPE_LONG:
                        // comparing single-valued vs. single-valued : Example : 1.4 == 1.4
                        return longTypedValue == typedValue.longTypedValue;
                    case ATTRIBUTE_TYPE_MULTI_LONG:{
                        // comparing single-valued vs. multi-valued. Example : 1.4 == <1.3,3.2,23.0>
                        vector<long> values = typedValue.getMultiLongTypedValue();
                        return std::find(values.begin() , values.end() , longTypedValue) != values.end();
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
						// comparing single-valued vs. single-valued : Example : 1.4 == 1.4
						return floatTypedValue == typedValue.floatTypedValue;
					case ATTRIBUTE_TYPE_MULTI_FLOAT:{
				    	// comparing single-valued vs. multi-valued. Example : 1.4 == <1.3,3.2,23.0>
						vector<float> values = typedValue.getMultiFloatTypedValue();
						return std::find(values.begin() , values.end() , floatTypedValue) != values.end();
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
            case ATTRIBUTE_TYPE_DOUBLE:{

                switch (typedValue.valueType) {
                    case ATTRIBUTE_TYPE_DOUBLE:
                        // comparing single-valued vs. single-valued : Example : 1.4 == 1.4
                        return doubleTypedValue == typedValue.doubleTypedValue;
                    case ATTRIBUTE_TYPE_MULTI_DOUBLE:{
                        // comparing single-valued vs. multi-valued. Example : 1.4 == <1.3,3.2,23.0>
                        vector<double> values = typedValue.getMultiDoubleTypedValue();
                        return std::find(values.begin() , values.end() , doubleTypedValue) != values.end();
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
						// comparing single-valued vs. single-valued : Example : "toyota == toyota"
						return (stringTypedValue.compare(typedValue.stringTypedValue) == 0);
					case ATTRIBUTE_TYPE_MULTI_TEXT:{
				    	// comparing single-valued vs. multi-valued. Example : "toyota" == <"toyota", "honda", "ford">
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
						// comparing single-valued vs. single-valued : Example : 12343245 == 2424244
						return timeTypedValue == typedValue.timeTypedValue;
					case ATTRIBUTE_TYPE_MULTI_TIME:{
				    	// comparing single-valued vs. multi-valued. Example : 13213132 == <13213132,2435433,5345532623>
						vector<long> values = typedValue.getMultiTimeTypedValue();
						return std::find(values.begin() , values.end() , timeTypedValue) != values.end();
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
			case ATTRIBUTE_TYPE_MULTI_INT:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_INT:{
						// comparing multi-valued vs. single-valued. Example : <1,3,3> == 2
						vector<int> values = this->getMultiIntTypedValue();
						return std::find(values.begin() , values.end() , typedValue.intTypedValue) != values.end();
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
            case ATTRIBUTE_TYPE_MULTI_LONG:{

                switch (typedValue.valueType) {
                    case ATTRIBUTE_TYPE_LONG:{
                        // comparing multi-valued vs. single-valued. Example : <1,3,3> == 2
                        vector<long> values = this->getMultiLongTypedValue();
                        return std::find(values.begin() , values.end() , typedValue.longTypedValue) != values.end();
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
						// comparing multi-valued vs. single-valued. Example : <1.3,3.4,3.45> == 2.4
						vector<float> values = this->getMultiFloatTypedValue();
						return std::find(values.begin() , values.end() , typedValue.floatTypedValue) != values.end();
					}
					default:
						ASSERT(false);
						break;
				}
		    	break;
			}
            case ATTRIBUTE_TYPE_MULTI_DOUBLE:{

                switch (typedValue.valueType) {
                    case ATTRIBUTE_TYPE_DOUBLE:{
                        // comparing multi-valued vs. single-valued. Example : <1,3,3> == 2
                        vector<double> values = this->getMultiDoubleTypedValue();
                        return std::find(values.begin() , values.end() , typedValue.doubleTypedValue) != values.end();
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
						// comparing multi-valued vs. single-valued. Example : <"toyota", "honda", "ford"> == "toyota"
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
						// comparing multi-valued vs. single-valued. Example : <13213132,2435433,5345532623> == 13213132
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
			case ATTRIBUTE_TYPE_INT:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_INT:
						return intTypedValue < typedValue.intTypedValue;
					case ATTRIBUTE_TYPE_MULTI_INT:{
						vector<int> values = typedValue.getMultiIntTypedValue();
						for(vector<int>::iterator value = values.begin() ; value != values.end() ; ++value){
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
            case ATTRIBUTE_TYPE_LONG:{

                switch (typedValue.valueType) {
                    case ATTRIBUTE_TYPE_LONG:
                        return longTypedValue < typedValue.longTypedValue;
                    case ATTRIBUTE_TYPE_MULTI_LONG:{
                        vector<long> values = typedValue.getMultiLongTypedValue();
                        for(vector<long>::iterator value = values.begin() ; value != values.end() ; ++value){
                            if(longTypedValue < *value) return true;
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
            case ATTRIBUTE_TYPE_DOUBLE:{

                switch (typedValue.valueType) {
                    case ATTRIBUTE_TYPE_DOUBLE:
                        return doubleTypedValue < typedValue.doubleTypedValue;
                    case ATTRIBUTE_TYPE_MULTI_DOUBLE:{
                        vector<double> values = typedValue.getMultiDoubleTypedValue();
                        for(vector<double>::iterator value = values.begin() ; value != values.end() ; ++value){
                            if(doubleTypedValue < *value) return true;
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
			case ATTRIBUTE_TYPE_MULTI_INT:{

		    	switch (typedValue.valueType) {
					case ATTRIBUTE_TYPE_INT:{
						vector<int> values = this->getMultiIntTypedValue();
						for(vector<int>::iterator value = values.begin() ; value != values.end() ; ++value){
							if(*value < typedValue.intTypedValue) return true;
						}
						return false;
					}
					case ATTRIBUTE_TYPE_MULTI_INT:{
						vector<int> valuesThis = this->getMultiIntTypedValue();
						vector<int> valuesInput = typedValue.getMultiIntTypedValue();
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
            case ATTRIBUTE_TYPE_MULTI_LONG:{

                switch (typedValue.valueType) {
                    case ATTRIBUTE_TYPE_LONG:{
                        vector<long> values = this->getMultiLongTypedValue();
                        for(vector<long>::iterator value = values.begin() ; value != values.end() ; ++value){
                            if(*value < typedValue.longTypedValue) return true;
                        }
                        return false;
                    }
                    case ATTRIBUTE_TYPE_MULTI_LONG:{
                        vector<long> valuesThis = this->getMultiLongTypedValue();
                        vector<long> valuesInput = typedValue.getMultiLongTypedValue();
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
            case ATTRIBUTE_TYPE_MULTI_DOUBLE:{

                switch (typedValue.valueType) {
                    case ATTRIBUTE_TYPE_DOUBLE:{
                        vector<double> values = this->getMultiDoubleTypedValue();
                        for(vector<double>::iterator value = values.begin() ; value != values.end() ; ++value){
                            if(*value < typedValue.doubleTypedValue) return true;
                        }
                        return false;
                    }
                    case ATTRIBUTE_TYPE_MULTI_DOUBLE:{
                        vector<double> valuesThis = this->getMultiDoubleTypedValue();
                        vector<double> valuesInput = typedValue.getMultiDoubleTypedValue();
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
			result.setTypedValue(this->getTimeDuration() + typedValue.getTimeTypedValue(),this->valueType);
			return result;
		}
		if(this->valueType == ATTRIBUTE_TYPE_TIME && typedValue.valueType == ATTRIBUTE_TYPE_DURATION){
			result.setTypedValue(typedValue.getTimeDuration() + this->getTimeTypedValue(),this->valueType);
			return result;
		}
		ASSERT(typedValue.valueType == this->valueType);
		ASSERT(this->valueType != ATTRIBUTE_TYPE_MULTI_INT);
		ASSERT(this->valueType != ATTRIBUTE_TYPE_MULTI_LONG);
		ASSERT(this->valueType != ATTRIBUTE_TYPE_MULTI_FLOAT);
		ASSERT(this->valueType != ATTRIBUTE_TYPE_MULTI_DOUBLE);
		ASSERT(this->valueType != ATTRIBUTE_TYPE_MULTI_TEXT);
		ASSERT(this->valueType != ATTRIBUTE_TYPE_MULTI_TIME);
    	switch (this->valueType) {
			case ATTRIBUTE_TYPE_INT:
				result.setTypedValue(typedValue.getIntTypedValue() + this->getIntTypedValue(),this->valueType);
				break;
			case ATTRIBUTE_TYPE_LONG:
			    result.setTypedValue(typedValue.getLongTypedValue() + this->getLongTypedValue(),this->valueType);
			    break;
			case ATTRIBUTE_TYPE_FLOAT:
				result.setTypedValue(typedValue.getFloatTypedValue() + this->getFloatTypedValue(),this->valueType);
				break;
			case ATTRIBUTE_TYPE_DOUBLE:
			    result.setTypedValue(typedValue.getDoubleTypedValue() + this->getDoubleTypedValue(),this->valueType);
			    break;
			case ATTRIBUTE_TYPE_TEXT:
				result.setTypedValue(typedValue.getTextTypedValue() + this->getTextTypedValue(),this->valueType);
				break;
			case ATTRIBUTE_TYPE_TIME:
				result.setTypedValue(typedValue.getTimeTypedValue() + this->getTimeTypedValue(),this->valueType);
				break;
			case ATTRIBUTE_TYPE_DURATION:
				result.setTypedValue(typedValue.getTimeDuration() + this->getTimeDuration(),this->valueType);
				break;
			default:
				break;
		}

    	return result;
	}




	void TypedValue::setTypedValue(int intTypedValue,FilterType valueType){
		this->valueType = valueType;
		this->intTypedValue = intTypedValue;
	}
    void TypedValue::setTypedValue(long longTypedValue,FilterType valueType){
        this->valueType = valueType;
        if(this->valueType == ATTRIBUTE_TYPE_TIME){
            this->timeTypedValue = longTypedValue;
        }else if(this->valueType == ATTRIBUTE_TYPE_LONG){
            this->longTypedValue = longTypedValue;
        }else{
            ASSERT(false);
        }
    }
	void TypedValue::setTypedValue(float floatTypedValue,FilterType valueType){
	    this->valueType = valueType;
		this->floatTypedValue = floatTypedValue;
	}
	void TypedValue::setTypedValue(double doubleTypedValue,FilterType valueType){
	    this->valueType = valueType;
	    this->doubleTypedValue = doubleTypedValue;
	}
	void TypedValue::setTypedValue(string stringTypedValue,FilterType valueType){
	    this->valueType = valueType;
		this->stringTypedValue = stringTypedValue;
	}
	void TypedValue::setTypedValue(vector<int> intTypedValue,FilterType valueType){
	    this->valueType = valueType;
		this->intTypedMultiValue = intTypedValue;
	}
    void TypedValue::setTypedValue(vector<long> longTypedValue,FilterType valueType){
        this->valueType = valueType;
        if(this->valueType == ATTRIBUTE_TYPE_MULTI_TIME){
            this->timeTypedMultiValue = longTypedValue;
        }else if(this->valueType == ATTRIBUTE_TYPE_MULTI_LONG){
            this->longTypedMultiValue = longTypedValue;
        }
    }
	void TypedValue::setTypedValue(vector<float> floatTypedValue,FilterType valueType){
	    this->valueType = valueType;
		this->floatTypedMultiValue = floatTypedValue;
	}
    void TypedValue::setTypedValue(vector<double> doubleTypedValue,FilterType valueType){
        this->valueType = valueType;
        this->doubleTypedMultiValue = doubleTypedValue;
    }
	void TypedValue::setTypedValue(vector<string> stringTypedValue,FilterType valueType){
	    this->valueType = valueType;
		this->stringTypedMultiValue = stringTypedValue;
	}


	void TypedValue::setTypedValue(const srch2::instantsearch::TimeDuration & duration,FilterType valueType){
		valueType = ATTRIBUTE_TYPE_DURATION;
		this->timeDurationTypedValue = duration;
	}

	void TypedValue::setTypedValue(const TypedValue& typedValue){
    	valueType = typedValue.valueType;
    	switch (valueType) {
			case ATTRIBUTE_TYPE_INT:
				intTypedValue = typedValue.intTypedValue;
				break;
            case ATTRIBUTE_TYPE_LONG:
                longTypedValue = typedValue.longTypedValue;
                break;
			case ATTRIBUTE_TYPE_FLOAT:
				floatTypedValue = typedValue.floatTypedValue;
				break;
			case ATTRIBUTE_TYPE_DOUBLE:
			    doubleTypedValue = typedValue.doubleTypedValue;
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
			case ATTRIBUTE_TYPE_MULTI_INT:
				intTypedMultiValue = typedValue.intTypedMultiValue;
				break;
			case ATTRIBUTE_TYPE_MULTI_LONG:
			    longTypedMultiValue = typedValue.longTypedMultiValue;
			    break;
			case ATTRIBUTE_TYPE_MULTI_FLOAT:
				floatTypedMultiValue = typedValue.floatTypedMultiValue;
				break;
			case ATTRIBUTE_TYPE_MULTI_DOUBLE:
			    doubleTypedMultiValue = typedValue.doubleTypedMultiValue;
			    break;
			case ATTRIBUTE_TYPE_MULTI_TEXT:
				stringTypedMultiValue = typedValue.stringTypedMultiValue;
				break;
			case ATTRIBUTE_TYPE_MULTI_TIME:
				timeTypedMultiValue = typedValue.timeTypedMultiValue;
				break;
		}
	}

	void TypedValue::setTypedValue(FilterType type , string value){
	    // TODO : do some validation to make sure engine does not crash.
	    //        NOTE: The input to this function is supposed to be validated once...
		this->valueType = type;
		switch (type) {
			case ATTRIBUTE_TYPE_INT:
				this->setTypedValue(static_cast<int>(strtol(value.c_str(), NULL, 10)),ATTRIBUTE_TYPE_INT);
				break;
            case ATTRIBUTE_TYPE_LONG:
                this->setTypedValue(strtol(value.c_str(), NULL, 10),ATTRIBUTE_TYPE_LONG);
                break;
			case ATTRIBUTE_TYPE_FLOAT:
				this->setTypedValue(static_cast<float>(strtod(value.c_str(), NULL)),ATTRIBUTE_TYPE_FLOAT);
				break;
            case ATTRIBUTE_TYPE_DOUBLE:
                this->setTypedValue(strtod(value.c_str(), NULL),ATTRIBUTE_TYPE_DOUBLE);
                break;
			case ATTRIBUTE_TYPE_TEXT:
				this->setTypedValue(value,ATTRIBUTE_TYPE_TEXT);
				break;
			case ATTRIBUTE_TYPE_TIME:
				this->setTypedValue(strtol(value.c_str(), NULL, 10),ATTRIBUTE_TYPE_TIME);
				break;
			case ATTRIBUTE_TYPE_DURATION:
				this->setTypedValue(DateAndTimeHandler::convertDurationTimeStringToTimeDurationObject(value),ATTRIBUTE_TYPE_DURATION);
				break;
			case ATTRIBUTE_TYPE_MULTI_INT:
				this->setTypedValue(ATTRIBUTE_TYPE_INT , value);
				break;
            case ATTRIBUTE_TYPE_MULTI_LONG:
                this->setTypedValue(ATTRIBUTE_TYPE_LONG , value);
                break;
			case ATTRIBUTE_TYPE_MULTI_FLOAT:
				this->setTypedValue(ATTRIBUTE_TYPE_FLOAT , value);
				break;
            case ATTRIBUTE_TYPE_MULTI_DOUBLE:
                this->setTypedValue(ATTRIBUTE_TYPE_DOUBLE , value);
                break;
			case ATTRIBUTE_TYPE_MULTI_TEXT:
				this->setTypedValue(ATTRIBUTE_TYPE_TEXT , value);
				break;
			case ATTRIBUTE_TYPE_MULTI_TIME:
				this->setTypedValue(ATTRIBUTE_TYPE_TIME , value);
				break;
		}
	}

	int TypedValue::getIntTypedValue() const{
		return intTypedValue;
	}
    long TypedValue::getLongTypedValue() const{
        return longTypedValue;
    }
	float TypedValue::getFloatTypedValue() const{
		return floatTypedValue;
	}
    double TypedValue::getDoubleTypedValue() const{
        return doubleTypedValue;
    }
	string TypedValue::getTextTypedValue() const{
		return stringTypedValue;
	}
	long TypedValue::getTimeTypedValue() const{
		return timeTypedValue;
	}
	vector<int> TypedValue::getMultiIntTypedValue() const{
		return intTypedMultiValue;
	}
    vector<long> TypedValue::getMultiLongTypedValue() const{
        return longTypedMultiValue;
    }
	vector<float> TypedValue::getMultiFloatTypedValue() const{
		return floatTypedMultiValue;
	}
    vector<double> TypedValue::getMultiDoubleTypedValue() const{
        return doubleTypedMultiValue;
    }
	vector<string> TypedValue::getMultiTextTypedValue() const{
		return stringTypedMultiValue;
	}
	vector<long> TypedValue::getMultiTimeTypedValue() const{
		return timeTypedMultiValue;
	}

	TimeDuration TypedValue::getTimeDuration() const{
		return timeDurationTypedValue;
	}

	TypedValue TypedValue::minimumValue(){
		TypedValue result ;
		switch (valueType) {
				case ATTRIBUTE_TYPE_INT:
				case ATTRIBUTE_TYPE_MULTI_INT:
					result.setTypedValue((int)std::numeric_limits<int>::min(),valueType);
					break;
                case ATTRIBUTE_TYPE_LONG:
                case ATTRIBUTE_TYPE_MULTI_LONG:
                    result.setTypedValue((long)std::numeric_limits<long>::min(),valueType);
                    break;
				case ATTRIBUTE_TYPE_FLOAT:
				case ATTRIBUTE_TYPE_MULTI_FLOAT:
					result.setTypedValue((float)std::numeric_limits<float>::min(),valueType);
					break;
                case ATTRIBUTE_TYPE_DOUBLE:
                case ATTRIBUTE_TYPE_MULTI_DOUBLE:
                    result.setTypedValue((double)std::numeric_limits<double>::min(),valueType);
                    break;
				case ATTRIBUTE_TYPE_TEXT:
				case ATTRIBUTE_TYPE_MULTI_TEXT:
					result.setTypedValue("NO_MINIMUM_FOR_TEXT",valueType);
					break;
				case ATTRIBUTE_TYPE_TIME:
				case ATTRIBUTE_TYPE_MULTI_TIME:
					result.setTypedValue((long)-1893456000,valueType); // This number is the number of seconds from Jan-1st, 1910
					break;
				case ATTRIBUTE_TYPE_DURATION:
					TimeDuration tD;
					result.setTypedValue(tD,valueType);
					break;
			}

		return result;
	}

	float TypedValue::castToFloat(){
		float result = 0;
		switch (valueType) {
				case ATTRIBUTE_TYPE_INT:
					result = (float) getIntTypedValue();
					break;
                case ATTRIBUTE_TYPE_LONG:
                    result = (float) getLongTypedValue();
                    break;
				case ATTRIBUTE_TYPE_FLOAT:
					result = getFloatTypedValue();
					break;
                case ATTRIBUTE_TYPE_DOUBLE:
                    result = (float) getDoubleTypedValue();
                    break;
				case ATTRIBUTE_TYPE_TEXT:
					result = -1;
					ASSERT(false);
					break;
				case ATTRIBUTE_TYPE_TIME:
					result = (float) getTimeTypedValue();
					break;
				case ATTRIBUTE_TYPE_DURATION:
				case ATTRIBUTE_TYPE_MULTI_INT:
				case ATTRIBUTE_TYPE_MULTI_LONG:
				case ATTRIBUTE_TYPE_MULTI_FLOAT:
				case ATTRIBUTE_TYPE_MULTI_DOUBLE:
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
			case ATTRIBUTE_TYPE_MULTI_INT:
				for(vector<int>::const_iterator value = intTypedMultiValue.begin(); value != intTypedMultiValue.end() ; ++value){
					TypedValue singleValue;
					singleValue.setTypedValue(*value,ATTRIBUTE_TYPE_INT);
					output->push_back(singleValue);
				}
				break;
            case ATTRIBUTE_TYPE_MULTI_LONG:
                for(vector<long>::const_iterator value = longTypedMultiValue.begin(); value != longTypedMultiValue.end() ; ++value){
                    TypedValue singleValue;
                    singleValue.setTypedValue(*value,ATTRIBUTE_TYPE_LONG);
                    output->push_back(singleValue);
                }
                break;
			case ATTRIBUTE_TYPE_MULTI_FLOAT:
				for(vector<float>::const_iterator value = floatTypedMultiValue.begin(); value != floatTypedMultiValue.end() ; ++value){
					TypedValue singleValue;
					singleValue.setTypedValue(*value,ATTRIBUTE_TYPE_FLOAT);
					output->push_back(singleValue);
				}
				break;
            case ATTRIBUTE_TYPE_MULTI_DOUBLE:
                for(vector<double>::const_iterator value = doubleTypedMultiValue.begin(); value != doubleTypedMultiValue.end() ; ++value){
                    TypedValue singleValue;
                    singleValue.setTypedValue(*value,ATTRIBUTE_TYPE_DOUBLE);
                    output->push_back(singleValue);
                }
                break;
			case ATTRIBUTE_TYPE_MULTI_TEXT:
				for(vector<string>::const_iterator value = stringTypedMultiValue.begin(); value != stringTypedMultiValue.end() ; ++value){
					TypedValue singleValue;
					singleValue.setTypedValue(*value,ATTRIBUTE_TYPE_TEXT);
					output->push_back(singleValue);
				}
				break;
			case ATTRIBUTE_TYPE_MULTI_TIME:
				for(vector<long>::const_iterator value = timeTypedMultiValue.begin(); value != timeTypedMultiValue.end() ; ++value){
					TypedValue singleValue;
					singleValue.setTypedValue(*value,ATTRIBUTE_TYPE_TIME);
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
            case ATTRIBUTE_TYPE_INT:
                // first bucket which covers less-than-start values is zero
                if(this->intTypedValue < start.getIntTypedValue()){
                    return 0;
                }
                thisTypedValue = this->getIntTypedValue();
                startTypedValue = start.getIntTypedValue();
                endTypedValue = end.getIntTypedValue();
                gapTypedValue = gap.getIntTypedValue();
                break;
            case ATTRIBUTE_TYPE_LONG:
                // first bucket which covers less-than-start values is zero
                if(this->longTypedValue < start.getLongTypedValue()){
                    return 0;
                }
                thisTypedValue = this->getLongTypedValue();
                startTypedValue = start.getLongTypedValue();
                endTypedValue = end.getLongTypedValue();
                gapTypedValue = gap.getLongTypedValue();
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
            case ATTRIBUTE_TYPE_DOUBLE:
                // first bucket which covers less-than-start values is zero
                if(this->doubleTypedValue < start.getDoubleTypedValue()){
                    return 0;
                }
                thisTypedValue = this->getDoubleTypedValue();
                startTypedValue = start.getDoubleTypedValue();
                endTypedValue = end.getDoubleTypedValue();
                gapTypedValue = gap.getDoubleTypedValue();
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
			case ATTRIBUTE_TYPE_INT:
				ss << intTypedValue ;
				break;
            case ATTRIBUTE_TYPE_LONG:
                ss << longTypedValue ;
                break;
			case ATTRIBUTE_TYPE_FLOAT:
				ss << floatTypedValue;
				break;
            case ATTRIBUTE_TYPE_DOUBLE:
                ss << doubleTypedValue ;
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
			case ATTRIBUTE_TYPE_MULTI_INT:
				// example of multivalued int : [12,14,25,54,32,1] , toString will return "12,14,25,54,32,1"
				for(vector<int>::const_iterator value = intTypedMultiValue.begin() ; value != intTypedMultiValue.end() ; ++value){
					if(value == intTypedMultiValue.begin()){
						ss << *value ;
					}else{
						ss << "," << *value ;
					}
				}
				break;
            case ATTRIBUTE_TYPE_MULTI_LONG:
                // example of multivalued long : [12,14,25,54,32,1] , toString will return "12,14,25,54,32,1"
                for(vector<long>::const_iterator value = longTypedMultiValue.begin() ; value != longTypedMultiValue.end() ; ++value){
                    if(value == longTypedMultiValue.begin()){
                        ss << *value ;
                    }else{
                        ss << "," << *value ;
                    }
                }
                break;
			case ATTRIBUTE_TYPE_MULTI_FLOAT:
				for(vector<float>::const_iterator value = floatTypedMultiValue.begin() ; value != floatTypedMultiValue.end() ; ++value){
					if(value == floatTypedMultiValue.begin()){
						ss << *value ;
					}else{
						ss << "," << *value ;
					}
				}
				break;
            case ATTRIBUTE_TYPE_MULTI_DOUBLE:
                for(vector<double>::const_iterator value = doubleTypedMultiValue.begin() ; value != doubleTypedMultiValue.end() ; ++value){
                    if(value == doubleTypedMultiValue.begin()){
                        ss << *value ;
                    }else{
                        ss << "," << *value ;
                    }
                }
                break;
			case ATTRIBUTE_TYPE_MULTI_TEXT:
				for(vector<string>::const_iterator value = stringTypedMultiValue.begin() ; value != stringTypedMultiValue.end() ; ++value){
					if(value == stringTypedMultiValue.begin()){
						ss << *value ;
					}else{
						ss << "," << *value ;
					}
				}
				break;
			case ATTRIBUTE_TYPE_MULTI_TIME:
				for(vector<long>::const_iterator value = timeTypedMultiValue.begin() ; value != timeTypedMultiValue.end() ; ++value){
					if(value == timeTypedMultiValue.begin()){
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
