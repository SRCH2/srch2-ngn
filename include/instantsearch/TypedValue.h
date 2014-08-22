
// $Id: Score.h 2013-06-19 02:11:13Z Jamshid $

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
