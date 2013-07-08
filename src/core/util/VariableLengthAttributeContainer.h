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


#ifndef VARIABLE_ATTRIBUTE_CONTAINER_H_UTIL_SRC
#define VARIABLE_ATTRIBUTE_CONTAINER_H_UTIL_SRC


#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <cstring>
#include <ctime>
#include <vector>

#include "instantsearch/Schema.h"


namespace srch2
{
namespace instantsearch
{

class VariableLengthAttributeContainer
{

public:

	VariableLengthAttributeContainer();
	~VariableLengthAttributeContainer();


	// initializes the data array
	void init(const Schema * schema);

// set attribute by giving string representation
	void setAttribute(unsigned iter , const Schema * schema , std::string value);

	// getting string representation of the attribute value
	std::string getAttribute(unsigned iter, const Schema * schema ) const;


	unsigned getUnsignedAttribute(unsigned iter , const Schema * schema) const;
	float getFloatAttribute(unsigned iter , const Schema * schema) const;
	double getDoubleAttribute(unsigned iter, const Schema * schema) const;
	std::string getTextAttribute(unsigned iter , const Schema * schema) const;
	long getTimeAttribute(unsigned iter , const Schema * schema) const;

//private:

	unsigned char * data; // byte array to keep the data

	unsigned dataSize;


	// uses Schema interface to get the type of an attribute indexed by iter
	FilterType getAttributeType(unsigned iter, const Schema * schema) const;


	// updates the value of the attribute in the byte array
	void setAttribute(unsigned iter, const Schema * schema, std::vector<unsigned char> attributeValue);


	// read the value of an attribute and copies it in char vector
	void getAttribute(unsigned iter, const Schema * schema, std::vector<unsigned char>& attributeValue) const;

	// if updateFlag is on, reads and appends the newAttributeVlue to the newData vector
	// else reads the attribute value from the currentData and appends it to newData.
	// In both cases based on the type this attribute moves the startIndex to the beginning point of
	// the next attribute and returns this value through newStartIndex.
	void processNextAttribute(FilterType attributeType, const unsigned char * currentData, unsigned startIndex, bool updateFlag,
			std::vector<unsigned char> newAttributeValue, std::vector<unsigned char>& newData , unsigned& newStartIndex ) const;



	// Converting unsigned and char vector together.
	std::vector<unsigned char> convertUnsignedToCharVector(unsigned input) const;
	unsigned convertCharVectorToUnsigned(std::vector<unsigned char> input , std::vector<unsigned char>::iterator start, std::vector<unsigned char>::iterator end) const;

	// Converting float and char vector together.
	std::vector<unsigned char> convertFloatToCharVector(float input) const;
	float convertCharVectorToFloat(std::vector<unsigned char> input , std::vector<unsigned char>::iterator start, std::vector<unsigned char>::iterator end) const;

	// Converting long and char vector together.
	std::vector<unsigned char> convertLongToCharVector(long input) const;
	long convertCharVectorToLong(std::vector<unsigned char> input , std::vector<unsigned char>::iterator start, std::vector<unsigned char>::iterator end) const;




	// Based on the type, converts the string representation of the value to char vector
	std::vector<unsigned char> convertStringToCharVector(FilterType type, std::string value) const;

	// Based on type, converts char vector to the string representation of the value, can also be used to convert charvector to string
	std::string convertCharVectorToString(FilterType type, std::vector<unsigned char> value) const;

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	typename Archive::is_loading load;
        ar & this->dataSize;
        // In loading process, we need to allocate space for the members first.
        if(load)
        {
//        	this->sortableAttributeScores = new float[this->getNumberOfSortableAttributes()];
        	this->data = new unsigned char[this->dataSize];
        }
//        ar & boost::serialization::make_array(this->sortableAttributeScores, this->getNumberOfSortableAttributes());
        ar & boost::serialization::make_array(this->data, this->dataSize);
    }

};

}
}


#endif // VARIABLE_ATTRIBUTE_CONTAINER_H_UTIL_SRC
