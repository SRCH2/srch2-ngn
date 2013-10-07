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

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

#ifndef __UTIL_VARIABLE_ATTRIBUTE_CONTAINER_H__
#define __UTIL_VARIABLE_ATTRIBUTE_CONTAINER_H__

#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <cstring>
#include <ctime>
#include <vector>

#include "instantsearch/Schema.h"
#include "instantsearch/TypedValue.h"

#define Byte unsigned char
namespace srch2 {
namespace instantsearch {
/*
 * This class implements a directory-less variable length encoding. It has a Byte array which keeps
 * the values of the fields. The encoding uses sizeof(unsigned), sizeof(float) and sizeof(long) bytes
 * to store unsigned, float and long values respectively. For strings, this encoding first stores the
 * size of the string in 4 bytes and then the bytes of the actual string.
 * Example:
 * fields : price(FLOAT), discount(FLOAT), citation(UNSIGNED), name(TEXT), id(UNSIGNED)
 * sample data :  12.5  , 43.45              , 34                , johnson      , 253
 * | 4 bytes (12.5) | 4 bytes (43.45) | 4 bytes (citation) | 4 bytes (7) | 7 bytes (johnson) | 4 bytes (253)
 */
class VariableLengthAttributeContainer {

public:

	// it calculates and returns the number of bytes that this list will need
	static unsigned getSizeNeededForAllocation(const Schema * schema,const vector<string> & nonSearchableAttributeValues);

    // fills the container with the values
    static void fillWithoutAllocation(const Schema * schema,const vector<string> & nonSearchableAttributeValues, Byte * data);

    // fills the container with the values
	// Byte *& data is a pass by reference of a pointer variable, data will be allocated and set in this function.
    static void fill(const Schema * schema,const vector<string> & nonSearchableAttributeValues, Byte *& data, unsigned & dataSize);

    // deallocates the data and clears the container. After calling this function it can be filled again.
	// Byte *& data is a pass by reference of a pointer variable, data will be deallocated and set to NULL in this function.
    static void clear(Byte *& data , unsigned dataSize);
    // gets string representation of the attribute value
    static std::string getAttribute(unsigned nonSearchableAttributeIndex,
            const Schema * schema, const Byte * data);

    // gets the attribute value wrapped in a Score object
    static void getAttribute(const unsigned nonSearchableAttributeIndex,
            const Schema * schema, const Byte * data, TypedValue * score);

    // gets values of attributes in iters in Score objects. iters must be ascending.
    static void getBatchOfAttributes(
            const std::vector<unsigned> & nonSearchableAttributeIndexs,
            const Schema * schema, const Byte * data, std::vector<TypedValue> * scores);

    static unsigned getUnsignedAttribute(const unsigned nonSearchableAttributeIndex,
            const Schema * schema, const Byte * data);
    static float getFloatAttribute(const unsigned nonSearchableAttributeIndex,
            const Schema * schem, const Byte * data);
    static double getDoubleAttribute(const unsigned nonSearchableAttributeIndex,
            const Schema * schema, const Byte * data) ;
    static std::string getTextAttribute(const unsigned nonSearchableAttributeIndex,
            const Schema * schema, const Byte * data) ;
    static long getTimeAttribute(const unsigned nonSearchableAttributeIndex,
            const Schema * schema, const Byte * data) ;

private:

    // initializes the data array
    static void allocate(const Schema * schema,
            const vector<string> & nonSearchableAttributeValues, Byte *& data , unsigned & dataSize);
    // uses Schema interface to get the type of an attribute indexed by iter
    static FilterType getAttributeType(unsigned iter, const Schema * schema) ;

    // this function tells us the number of bytes used in the data for the attribute starting from startOffset
    // 4 bytes for size is included for TEXT case ...
    static unsigned getSizeOfNonSearchableAttributeValueInData(FilterType type,
            unsigned startOffset, const Byte * data) ;

    // Converting unsigned and char vector together.
    static void convertUnsignedToByteArray(unsigned input, Byte * output,
            unsigned startOffset) ;
    static unsigned convertByteArrayToUnsigned(unsigned startOffset, const Byte * data) ;

    // Converting float and char vector together.
    static void convertFloatToByteArray(float input, Byte * output,
            unsigned startOffset) ;
    static float convertByteArrayToFloat(unsigned startOffset, const Byte * data) ;

    // Converting long and char vector together.
    static void convertLongToByteArray(long input, Byte * output,
            unsigned startOffset) ;
    static long convertByteArrayToLong(unsigned startOffset, const Byte * data) ;

    // Based on the type, converts the string representation of the value to char vector
    // appends the results to the output
    static void convertStringToByteArray(FilterType type, std::string value,
            Byte * output, unsigned startOffset, unsigned & size) ;

    // Based on type, converts char vector to the string representation of the value, can also be used to convert charvector to string
    static std::string convertByteArrayToString(FilterType type,
            unsigned stringOffset, const Byte * data) ;

    static void convertByteArrayToTypedValue(FilterType type, unsigned startOffset, const Byte * data,
            TypedValue * result) ;

};

}
}

#endif // __UTIL_VARIABLE_ATTRIBUTE_CONTAINER_H_
