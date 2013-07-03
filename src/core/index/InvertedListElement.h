
// $Id: InvertedListElement.h 3429 2013-06-10 09:13:54Z jiaying $

#ifndef __INVERTED_LIST_ELEMENT_H__
#define __INVERTED_LIST_ELEMENT_H__


#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>


namespace bimaple
{
namespace instantsearch
{

struct InvertedListElement {
    unsigned recordId;
    unsigned positionIndexOffset;

    InvertedListElement(): recordId(0), positionIndexOffset(0) {};
	InvertedListElement(unsigned _recordId, unsigned _positionIndexOffset): recordId(_recordId), positionIndexOffset(_positionIndexOffset) {};
	InvertedListElement(const InvertedListElement &invertedlistElement)
	{
		if(this != &invertedlistElement)
		{
			this->recordId = invertedlistElement.recordId;
			this->positionIndexOffset = invertedlistElement.positionIndexOffset;
		}
	}
	InvertedListElement& operator=(const InvertedListElement &invertedlistElement)
	{
		if(this != &invertedlistElement)
		{
			this->recordId = invertedlistElement.recordId;
			this->positionIndexOffset = invertedlistElement.positionIndexOffset;
		}
		return *this;
	}

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & recordId;
        ar & positionIndexOffset;
    }

};
}}
#endif /* INVERTED_LIST_ELEMENT_H */
