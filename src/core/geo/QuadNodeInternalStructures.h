// $Id: QuadNodeInternalStructures.h 3456 2013-06-14 02:11:13Z jiaying $

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

#ifndef __QUADNODEINTERNALSTRUCTURES_H__
#define __QUADNODEINTERNALSTRUCTURES_H__

#include <bitset>
#include <map>
#include <vector>

#include <instantsearch/Term.h>
#include <instantsearch/Record.h>

#include "index/Trie.h"
#include "record/LocationRecordUtil.h"

using namespace std;

namespace srch2
{
namespace instantsearch
{

class Prefix;
class GeoElement;
class GeoElementList;

const unsigned CHILD_NUM_SQRT = 4;    // Square root of the maximum number of children each intermediate node can have
const unsigned CHILD_NUM = (CHILD_NUM_SQRT * CHILD_NUM_SQRT);
const unsigned BRANCH_FACTOR = 32;    // The maximum number of geoElements each leaf node can have

/*
Determines the threshold which decides if the keyword information
is to be kept in the node.  For example : if CHILD_SELECTIVITY_THRESHOLD = 10 and a keyword is present in more than 10 children
of the node, then the keyword is said to be non selective and should not be kept as a Child Filter
*/
const unsigned CHILD_SELECTIVITY_THRESHOLD = 17;//14;

/*
The threshold which decides if a keyword is qualified for oFilter.
The maximum number of records that a oFilter keyword can be associated with
*/
const unsigned OBJECT_SELECTIVITY_THRESHOLD = 16;//30;

const double TOP_RIGHT_X = 200.0;    // The top right point of the maximum rectangle range of the whole quadtree
const double TOP_RIGHT_Y = 200.0;
const double BOTTOM_LEFT_X = -200.0;    // The bottom left point of the maximum rectangle range of the whole quadtree
const double BOTTOM_LEFT_Y = -200.0;

const double MBR_LIMIT = (0.0005 * 0.0005); //0.005 The min size of a single mbr

const double MIN_SEARCH_RANGE_SQUARE = (0.24 * 0.24);    // The largest range we should search for, in degree
const double MIN_DISTANCE_SCORE = 0.05;

typedef std::bitset<CHILD_NUM> ChildBitSet;

/**
 *  Stored in the QuadTree's leaf node
 *  Each geoElement contains a record's id and location information
 */
class GeoElement
{
public:

    // the location point of the object
    Point point;

    // Offset of this record in forwardlist directory
    unsigned forwardListID;

    GeoElement(){}

    GeoElement(const Record *record, unsigned recordInternalId)
    {
        this->point.x = record->getLocationAttributeValue().first;
        this->point.y = record->getLocationAttributeValue().second;
        this->forwardListID = recordInternalId;
    }

    virtual ~GeoElement(){};

    bool operator==(const GeoElement &e) const
    {
        return forwardListID == e.forwardListID && point == e.point;
    };

private:

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & this->forwardListID;
        ar & this->point;
    }
};
/*
 * Jamshid : When new records are inserted in update time this structure keeps their information.
 */
class InfoToFixBroadenPrefixesOnFilters
{
public:
    vector<Prefix> *oldParentOrSelfAndAncs;
    pair<unsigned, unsigned> problemKeywordIdPair;
    bool hadExactlyOneChild;

    InfoToFixBroadenPrefixesOnFilters() {}
    InfoToFixBroadenPrefixesOnFilters(vector<Prefix> *o, unsigned p, bool h, unsigned leftOrRight)
    {
        ASSERT(leftOrRight == 1 || leftOrRight == 2);

        this->oldParentOrSelfAndAncs = o;

        if (leftOrRight == 1) // CHEN: Break left
        {
            problemKeywordIdPair.first = p;
            problemKeywordIdPair.second = Trie::MAX_KEYWORD_ID; // CHEN: a flag to indicate it's a boundary
        }
        else if (leftOrRight == 2) // CHEN: Break right
        {
            problemKeywordIdPair.first = Trie::MAX_KEYWORD_ID;
            problemKeywordIdPair.second= p;
        }

        this->hadExactlyOneChild = h;
    }
};

// TODO more comments
class ExpansionStructure
{
public:
    ExpansionStructure( unsigned minId, unsigned maxId, unsigned char editDistance, const TrieNode* expansionNodePtr)
    {
        this->prefix.minId = minId;
        this->prefix.maxId = maxId;
        this->editDistance = editDistance;
        this->expansionNodePtr = expansionNodePtr;
    }

    Prefix prefix;
    //unsigned editDistance;
    unsigned char editDistance;
    const TrieNode* expansionNodePtr;  // TODO opt remove from here, keep it somewhere else

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & this->prefix;
        ar & this->editDistance;
        ar & this->expansionNodePtr;
    }
};

class MapSearcherTerm
{
public:
    // TODO Make the members private and write constructor, destructor, accessors, setters.
    vector<ExpansionStructure> expansionStructureVector;
    Term* termPtr;

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & this->expansionStructureVector;
        ar & this->termPtr;
    }

};

class GeoElementList
{
public:
    vector<unsigned> geoElementOffsets;

    unsigned freq;

    GeoElementList()
    {
        init();
    }

    ~GeoElementList()
    {
        uninit();
    }

    void init()
    {
        freq = 0;
    }

    void uninit()
    {
        geoElementOffsets.clear();
    }

    void add(unsigned offset)
    {
        if(geoElementOffsets.size() < OBJECT_SELECTIVITY_THRESHOLD)
            geoElementOffsets.push_back(offset);
        else
            freq++;
    }

    void add(const GeoElementList &geoElementList)
    {
        geoElementOffsets.insert(geoElementOffsets.end(), geoElementList.geoElementOffsets.begin(), geoElementList.geoElementOffsets.end());
    }

    bool equalTo(GeoElementList el)
    {
        if(this->geoElementOffsets.size() != el.geoElementOffsets.size())
            return false;
        for(unsigned i = 0; i < this->geoElementOffsets.size(); i++)
        {
            if(this->geoElementOffsets[i] != el.geoElementOffsets[i])
                return false;
        }

        return this->freq == el.freq;
    }

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & this->geoElementOffsets;
        ar & this->freq;
    }
};

// data structures used by the filter construction phase
// we don't use unordered_map but plain map here is because for prefixes operations we need order information

class OFilterMap
{

public:
    map< Prefix, GeoElementList >* omp;

    OFilterMap()
    {
        omp = new map< Prefix, GeoElementList >();
    };

    ~OFilterMap()
    {
        if (omp != NULL)
            delete omp;
    };

    bool equalTo(OFilterMap* ofp)
    {
        if(this->omp->size() != ofp->omp->size())
            return false;

        map< Prefix, GeoElementList >::iterator oFilterMapIterator;
        for(oFilterMapIterator = this->omp->begin(); oFilterMapIterator != this->omp->end(); oFilterMapIterator++)
        {
            Prefix p = oFilterMapIterator->first;
            if( ! ofp->omp->at(p).equalTo(oFilterMapIterator->second) )
                return false;
        }

        return true;
    }

    void gatherFreqOfAllAncestorPrefixesForOFilters(const Trie *trie, const Prefix *prefix = NULL);

    void removePopularAndUselessDescendantsOFilters(vector<Prefix> &skipListToChildren, const vector<Prefix> &skipListFromParent);

    // Search the prefix on the o-filter and get the corresponding geo element list
    GeoElementList* getGeoElementList(const Prefix &prefix) const;
    // earch the prefix on the o-filter and get the corresponding geo element list and the matching prefix
    GeoElementList* searchOFilter(const Prefix &prefix, Prefix &matchingPrefix) const;

    void getAllDescendantsOnOFilter(const Prefix &prefix, set<Prefix> &descendantPrefixes) const;

    /*
     * Just adds a new record to the filter
     * @param needToPing if this is true it means we already know filter contains this prefix
     */
    void updateOFilter(const Prefix &prefix, unsigned geoElementOffset, bool needToPing)
    {
        if(!needToPing || omp->find(prefix) == omp->end())
        {
            (*omp)[prefix].init();
        }

        (*omp)[prefix].add(geoElementOffset);
    }

    void updateOFilter(const Prefix &prefix, const GeoElementList &geoElementList, bool needToPing)
    {
        if(!needToPing || omp->find(prefix) == omp->end())
        {
            (*omp)[prefix].init();
        }

        (*omp)[prefix].add(geoElementList);
    }

    void addGeoElementToOFilter(const Prefix &prefix, unsigned geoElementOffset)
    {
        (*omp)[prefix].add(geoElementOffset);
    }

    void changePrefix(const Prefix &from, const Prefix &to)
    {
        (*omp)[to].init();
        (*omp)[to].add(omp->at(from));

        omp->erase(from);
    }

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & this->omp;
    }


};

typedef OFilterMap* OFilterMapPtr;


class CFilterMap
{
public:
    map< Prefix, ChildBitSet >* cmp;
    CFilterMap()
    {
        cmp = new map< Prefix, ChildBitSet >();
    };

    ~CFilterMap()
    {
        if (cmp != NULL)
            delete cmp;
    };

    bool equalTo(CFilterMap* cfp)
    {
        if(this->cmp->size() != cfp->cmp->size())
            return false;

        map< Prefix, ChildBitSet >::iterator cFilterMapIterator;
        for(cFilterMapIterator = this->cmp->begin(); cFilterMapIterator != this->cmp->end(); cFilterMapIterator++)
        {
            for( unsigned i = 0; i < CHILD_NUM; i++)
            {
                if( cfp->cmp->at(cFilterMapIterator->first).test(i) != cFilterMapIterator->second.test(i) )
                    return false;
            }
        }

        return true;
    }

    void gatherFreqOfAllAncestorPrefixesForCFilters(const Trie *trie);

    void removePopularAndUselessDescendantsCFilters(const vector<Prefix> &skipListToChildren);
    
    //Get the childrenBitVector directly from a position in the keyword-children map
    ChildBitSet* getChildrenBitVector(const Prefix &prefix) const;

    // Search the prefix on the c-filter, update the bit vector if we found
    // If we the prefix is not on c-filter, create it and set the bit
    void updateCFilter(const Prefix &prefix, unsigned child);

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & this->cmp;
    }
};

typedef CFilterMap* CFilterMapPtr;

}}

#endif /* __QUADNODEINTERNALSTRUCTURES_H__ */
