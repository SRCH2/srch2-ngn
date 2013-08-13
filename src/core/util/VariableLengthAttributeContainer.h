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

#ifndef _UTIL_VARIABLE_ATTRIBUTE_CONTAINER_H_
#define _UTIL_VARIABLE_ATTRIBUTE_CONTAINER_H_

#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <cstring>
#include <ctime>
#include <vector>

#include "instantsearch/Schema.h"
#include "instantsearch/Score.h"

#define Byte unsigned char

namespace srch2 {
namespace instantsearch {

class VariableLengthAttributeContainer {

public:

    VariableLengthAttributeContainer();
    ~VariableLengthAttributeContainer();

    // fills the container with the values
    void fill(const Schema * schema,
            const vector<string> & nonSearchableAttributeValues);

    // deallocates the data and clears the container. After calling this function it can be filled again.
    void clear();
    // getting string representation of the attribute value
    std::string getAttribute(unsigned nonSearchableAttributeIndex,
            const Schema * schema) const;

    // gets the attribute value wrapped in a Score object
    void getAttribute(const unsigned nonSearchableAttributeIndex,
            const Schema * schema, Score * score) const;

    // gets values of attributes in iters in Score objects. iters must be ascending.
    void getBatchOfAttributes(
            const std::vector<unsigned> & nonSearchableAttributeIndexs,
            const Schema * schema, std::vector<Score> * scores) const;

    unsigned getUnsignedAttribute(const unsigned nonSearchableAttributeIndex,
            const Schema * schema) const;
    float getFloatAttribute(const unsigned nonSearchableAttributeIndex,
            const Schema * schema) const;
    double getDoubleAttribute(const unsigned nonSearchableAttributeIndex,
            const Schema * schema) const;
    std::string getTextAttribute(const unsigned nonSearchableAttributeIndex,
            const Schema * schema) const;
    long getTimeAttribute(const unsigned nonSearchableAttributeIndex,
            const Schema * schema) const;

private:

    Byte * data; // byte array to keep the data

    unsigned dataSize;
    // initializes the data array
    void allocate(const Schema * schema,
            const vector<string> & nonSearchableAttributeValues);
    // uses Schema interface to get the type of an attribute indexed by iter
    FilterType getAttributeType(unsigned iter, const Schema * schema) const;

    // this function tells us the number of bytes used in the data for the attribute starting from startOffset
    // 4 bytes for size is included for TEXT case ...
    void getSizeOfNonSearchableAttributeValueInData(FilterType type,
            unsigned startOffset, unsigned & size) const;

    // Converting unsigned and char vector together.
    void convertUnsignedToByteArray(unsigned input, Byte * output,
            unsigned startOffset) const;
    unsigned convertByteArrayToUnsigned(unsigned startOffset) const;

    // Converting float and char vector together.
    void convertFloatToByteArray(float input, Byte * output,
            unsigned startOffset) const;
    float convertByteArrayToFloat(unsigned startOffset) const;

    // Converting long and char vector together.
    void convertLongToByteArray(long input, Byte * output,
            unsigned startOffset) const;
    long convertByteArrayToLong(unsigned startOffset) const;

    // Based on the type, converts the string representation of the value to char vector
    // appends the results to the output
    void convertStringToByteArray(FilterType type, std::string value,
            Byte * output, unsigned startOffset, unsigned & size) const;

    // Based on type, converts char vector to the string representation of the value, can also be used to convert charvector to string
    std::string convertByteArrayToString(FilterType type,
            unsigned stringOffset) const;

    void convertByteArrayToScore(FilterType type, unsigned startOffset,
            Score * result) const;

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        typename Archive::is_loading load;
        ar & this->dataSize;
        // In loading process, we need to allocate space for the members first.std::string VariableLengthAttributeContainer::getAttribute(unsigned nonSearchableAttributeIndex,
        if (load) {
            this->data = new Byte[this->dataSize];
        }
        ar & boost::serialization::make_array(this->data, this->dataSize);
    }

};

}
}

#endif // _UTIL_VARIABLE_ATTRIBUTE_CONTAINER_H_
