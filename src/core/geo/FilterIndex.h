//$Id: FilterIndex.h 3456 2013-06-14 02:11:13Z jiaying $

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

#ifndef __FILTERINDEX_H__
#define __FILTERINDEX_H__

#include "geo/QuadNodeInternalStructures.h"
#include "util/Assert.h"

#include <map>
#include <vector>

using std::map;
using std::vector;
using srch2::instantsearch::OFilterMapPtr;
using srch2::instantsearch::CFilterMapPtr;

namespace srch2
{
namespace instantsearch
{

class OFilterIndex
{
public:
    virtual ~OFilterIndex()
    {
        for(unsigned i = 0; i < oFilterIndex.size(); i++)
            delete oFilterIndex[i];
        oFilterIndex.clear();
    }

    std::vector< OFilterMapPtr > oFilterIndex;

    // add the OFilterMap for a single node into the global directory oFilterIndex
    void addOFilterMapToIndex(OFilterMapPtr &oMapPtr)
    {
        oFilterIndex.push_back(oMapPtr);
    }

    // get the NodeOFilterMap for a particular node
    OFilterMapPtr getOFilterMap(unsigned const oFilterOffset)
    {
        ASSERT(oFilterOffset != 0xFFFFFFFF);
        return oFilterIndex.at(oFilterOffset);
    }

    bool equalTo(OFilterIndex* oIndex)
    {
        if(this->oFilterIndex.size() != oIndex->oFilterIndex.size())
            return false;
        for(unsigned i = 0; i < this->oFilterIndex.size(); i++)
        {
            if( !(this->oFilterIndex[i]->equalTo(oIndex->oFilterIndex[i])) )
                return false;
        }

        return true;
    }

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & this->oFilterIndex;
    }
};


class CFilterIndex
{
public:

    virtual ~CFilterIndex()
    {
        for(unsigned i = 0; i < cFilterIndex.size(); i++)
            delete cFilterIndex[i];
        cFilterIndex.clear();
    }

    std::vector< CFilterMapPtr > cFilterIndex;

    // add the CFilterMap for a single node into the global directory cFilterIndex
    void addCFilterMapToIndex(CFilterMapPtr &cMapPtr)
    {
        cFilterIndex.push_back(cMapPtr);
    }

    // get the NodeCFilterMap for a particular node
    CFilterMapPtr getCFilterMap(unsigned const cFilterOffset)
    {
        ASSERT(cFilterOffset != 0xFFFFFFFF);
        return cFilterIndex.at(cFilterOffset);
    }

    bool equalTo(CFilterIndex* cIndex)
    {
        if(this->cFilterIndex.size() != cIndex->cFilterIndex.size())
            return false;
        for(unsigned i = 0; i < this->cFilterIndex.size(); i++)
        {
            if( !(this->cFilterIndex[i]->equalTo(cIndex->cFilterIndex[i])) )
                return false;
        }

        return true;
    }

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & this->cFilterIndex;
    }
};

}}

#endif /*__FILTERINDEX_H__*/
