/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */




#ifndef __INSTANTSEARCH_TYPEDVALUE_H__
#define __INSTANTSEARCH_TYPEDVALUE_H__

#include <cstring>
#include <ctime>
#include <instantsearch/Constants.h>
#include <instantsearch/Schema.h>
#include <instantsearch/DateTime.h>
#include <vector>
using namespace std;

using srch2::instantsearch::TimeDuration;

namespace srch2
{
    namespace instantsearch
    {

    class TypedValue
    {
    public:
    	TypedValue(const TypedValue& typedValue);
    	bool operator==(const TypedValue& typedValue) const;
    	bool operator!=(const TypedValue& typedValue) const;
    	bool operator<(const TypedValue& typedValue) const;
    	bool operator>(const TypedValue& typedValue) const;
    	bool operator<=(const TypedValue& typedValue) const;
    	bool operator>=(const TypedValue& typedValue) const;
    	TypedValue operator+(const TypedValue& a);

        TypedValue() : 
            valueType(ATTRIBUTE_TYPE_INT), intTypedValue(0), longTypedValue(0), floatTypedValue(0.0), doubleTypedValue(0.0), timeTypedValue(0)
        {
    	};
    	void setTypedValue(int intTypeValue,FilterType valueType);
    	void setTypedValue(long longTypeValue,FilterType valueType);
    	void setTypedValue(float floatTypeValue,FilterType valueType);
    	void setTypedValue(double doubleTypeValue,FilterType valueType);
    	void setTypedValue(const string& stringTypeValue,FilterType valueType);
    	void setTypedValue(const vector<int> & intTypeValue,FilterType valueType);
        void setTypedValue(const vector<long> & longTypeValue,FilterType valueType);
    	void setTypedValue(const vector<float> & floatTypeValue,FilterType valueType);
        void setTypedValue(const vector<double> & doubleTypeValue,FilterType valueType);
    	void setTypedValue(const vector<string> & stringTypeValue,FilterType valueType);

    	void setTypedValue(const srch2::instantsearch::TimeDuration & duration,FilterType valueType);
    	void setTypedValue(const TypedValue& typeValue);
    	void setTypedValue(FilterType type , const string & value);

    	FilterType getType() const{
    		return valueType;
    	}

    	unsigned getNumberOfBytes() {
    		unsigned capacity = sizeof(TypedValue);
    		capacity += stringTypedValue.capacity();
    		capacity += intTypedMultiValue.capacity() * sizeof(int);  // vector<int>
    		capacity += longTypedMultiValue.capacity() * sizeof(long);  // vector<long>
    		capacity += floatTypedMultiValue.capacity() * sizeof(float); // vector<float> ;
    		capacity += doubleTypedMultiValue.capacity() * sizeof(double);  // vector<double>
    		capacity += stringTypedMultiValue.capacity() * sizeof(string);  //vector<string>
    		for (unsigned i = 0 ; i < stringTypedMultiValue.size(); ++i)
    			capacity += stringTypedMultiValue[i].capacity();
    		capacity += timeTypedMultiValue.capacity() * sizeof(long); ; // vector<long>;
        	return capacity;
    	}
    	int getIntTypedValue() const;
    	long getLongTypedValue() const;
    	float getFloatTypedValue() const;
    	double getDoubleTypedValue() const;
    	const string & getTextTypedValue() const;
    	long getTimeTypedValue() const;
    	const vector<int> & getMultiIntTypedValue() const;
    	const vector<long> & getMultiLongTypedValue() const;
    	const vector<float> & getMultiFloatTypedValue() const;
    	const vector<double> & getMultiDoubleTypedValue() const;
    	const vector<string> & getMultiTextTypedValue() const;
    	const vector<long> & getMultiTimeTypedValue() const;

    	const TimeDuration & getTimeDuration() const;


    	TypedValue minimumValue();

    	float castToFloat();

    	/*
    	 * This function is used for example in findIndicesOfContainingIntervals
    	 * and is called only for multi-valued types. It breaks the multi-valued object and
    	 * returns a list of single-values.
    	 * For example : if the multi value is <"tag1","tag2","tag3">, it returns three objects with values
    	 * tag1, tag2 and tag3.
    	 */
    	void breakMultiValueIntoSingleValueTypedValueObjects(vector<TypedValue> * output) const;

    	/*
    	 * returns 0 if this < start
    	 * otherwise, returns floor((this - start) / gap)+1
    	 */
    	unsigned findIndexOfContainingInterval(TypedValue & start , TypedValue & end, TypedValue & gap) const;

    	/*
    	 * If an attribute is multi-valued, it can be counted for more than one category or interval, so we need a function
    	 * which returns a list of indices (also refer to findIndexOfContainingInterval)
    	 * Example: <1,3,5> will cause calling findIndexOfContainingInterval for 1, 3 and 5 three times.
    	 */
    	const vector<unsigned> findIndicesOfContainingIntervals(TypedValue & start , TypedValue & end, TypedValue & gap) const;

    	string toString() const;


    private:
    	FilterType valueType;
    	int intTypedValue;
    	long longTypedValue;
    	float floatTypedValue;
    	double doubleTypedValue;
    	string stringTypedValue;
    	long timeTypedValue;
    	vector<int> intTypedMultiValue;
    	vector<long> longTypedMultiValue;
    	vector<float> floatTypedMultiValue;
    	vector<double> doubleTypedMultiValue;
    	vector<string> stringTypedMultiValue;
    	vector<long> timeTypedMultiValue;
    	//
    	TimeDuration timeDurationTypedValue;
    };


    }

}

#endif // __INSTANTSEARCH_TYPEDVALUE_H__
