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
#include "util/RecordSerializerUtil.h"
using namespace srch2::util;

#define Byte char
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

    // gets values of attributes in iters in Score objects. iters must be ascending.
    static void getBatchOfAttributes(
            const std::vector<string> & nonSearchableAttributeIndexs,
            const Schema * schema, const Byte * data, std::vector<TypedValue> * scores);

private:

    static FilterType getAttributeType(const string& name, const Schema * schema) ;

    static unsigned convertByteArrayToUnsigned(unsigned startOffset, const Byte * data) ;

    static float convertByteArrayToFloat(unsigned startOffset, const Byte * data) ;

    static long convertByteArrayToLong(unsigned startOffset, const Byte * data) ;

    static void convertByteArrayToTypedValue(const string& name, bool isMultiValued, const FilterType& type,
    		RecordSerializer& recSerializer, const Byte * data, TypedValue * result) ;

};

}
}

#endif // __UTIL_VARIABLE_ATTRIBUTE_CONTAINER_H_
