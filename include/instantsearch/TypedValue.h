
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
#include <instantsearch/Schema.h>
#include <instantsearch/DateTime.h>
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

    	TypedValue(){
    	};
    	void setTypedValue(unsigned intTypeValue);
    	void setTypedValue(float floatTypeValue);
    	void setTypedValue(double doubleTypeValue);
    	void setTypedValue(string stringTypeValue);
    	void setTypedValue(long timeTypeValue);
    	void setTypedValue(const srch2::instantsearch::TimeDuration & duration);
    	void setTypedValue(const TypedValue& typeValue);
    	void setTypedValue(FilterType type , string value);

    	FilterType getType() const{
    		return valueType;
    	}

    	unsigned getIntTypedValue() const;
    	float getFloatTypedValue() const;
    	double getDoubleTypedValue() const;
    	string getTextTypedValue() const;
    	long getTimeTypedValue() const;
    	TimeDuration getTimeDuration() const;


    	TypedValue minimumValue();

    	float castToFloat();

    	/*
    	 * returns 0 if this < start
    	 * otherwise, returns floor((this - start) / gap)+1
    	 */
    	unsigned findIndexOfContainingInterval(TypedValue & start , TypedValue & end, TypedValue & gap) const;


    	string toString() const;


    private:
    	FilterType valueType;
    	unsigned intTypedValue;
    	float floatTypedValue;
    	string stringTypedValue;
    	long timeTypedValue;
    	TimeDuration timeDurationTypedValue;
    };


    }

}

#endif // __INSTANTSEARCH_TYPEDVALUE_H__
