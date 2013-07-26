
// $Id: QuadTree.h 3456 2013-06-14 02:11:13Z jiaying $

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

#ifndef __QUADTREE_H__
#define __QUADTREE_H__

#include <vector>
#include <bitset>
#include <map>
#include <set>
#include <assert.h>
// TODO: Move to PCH
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/split_member.hpp>

#include <instantsearch/Analyzer.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/Ranker.h>

#include "index/Trie.h"
#include "record/LocationRecordUtil.h"
#include "query/QueryResultsInternal.h"

#include "geo/FilterIndex.h"
#include "geo/QuadNodeInternalStructures.h"

using std::vector;
using std::bitset;
using std::string;
using std::set;

namespace srch2
{
namespace instantsearch
{
class ForwardIndex;
class ForwardList;
class QuadTree;
class QueryResultsInternal;
class QuadNode
{
public:

    QuadNode():cFilterOffset(0xFFFFFFFF), oFilterOffset(0xFFFFFFFF){ };

    virtual ~QuadNode()
    {
        // for internal QuadNode, we store the pointers of QuadNode
        // so we need to free each QuadNode
        // for leaf QuadNode, what we store are just offsets to the geoElementIndex
        // and QuadTree's destructore will clean those geoElements
        if(!isLeaf)
        {
            for(unsigned i = 0; i < entries.size(); i++)
            {
                if(entries[i] != NULL)
                    delete entries[i];
            }
        }

        entries.clear();
    }

    // the node's level in the tree, level 0 is the root level
    // TODO could be removed
    unsigned level;

    // whether this node is leaf node
    bool isLeaf;

    // the number of geoElements in the leaf nodes under this node
    unsigned numElements;

    Rectangle mbr;

    // If it is an intermediate node, the vector stores pointers to
    // children. If so, the size is always CHILD_NUM.  Some of the
    // child pointers can be NULL.  If it is a leaf node, the vector
    // stores pointers to geoElements, and the maximal size is BRANCH_FACTOR.
    std::vector<QuadNode*> entries;

    // pointer to the global directory of the C-Filter
    // This directory is in CFilterIndex in QuadTree class
    // TODO consider to move cFilters into the QuadNode
    unsigned cFilterOffset;
    
    // pointer to the global directory of the O-Filter
    // This directory is in OFilterIndex in QuadTree class
    // TODO consider to move oFilters into the QuadNode
    unsigned oFilterOffset;

    // Insert a geoElement under the specified node
    /*
     * It's called in construction phase. We insert all elements to shape the Quad Tree
     * In this phase no O/C filters are built. In updates, this function is called again
     * w/o changing O/C filters.
     *
     * @param quadTree
     * @param geoElement
     * @geoElementOffset
     * @needToConstructFilters
     */
    void insertGeoElement(QuadTree* quadTree, GeoElement &geoElement, unsigned geoElementOffset,
                  bool needToConstructFilters = false,
                  vector<Prefix> *skippedPrefixesFromAncestors = NULL);

    // add the record to the sub quadtree of this quadnode, taking care of O/C filters
    void addRecordToSubQuadTreeAfterCommit(QuadTree *quadtree, unsigned keywordId, unsigned geoElementOffset, const Prefix prefixFromParent,
                                            vector<OFilterMapPtr> &skipList, const set<unsigned> &insertedKeywords);

    // go down to the leaf node to gather information for building unpurged oFilters and cFilters
    void fetchRawFiltersFromleafNodesUnderThisNode(const Trie *trie, const ForwardIndex *forwardIndex, const vector<GeoElement*> &geoElementIndex, CFilterMapPtr cMapPtr, OFilterMapPtr oMapPtr, bool isDirectChild, unsigned childBit = CHILD_NUM);

    void fixFiltersBroadenOnOneNode(const QuadTree *quadtree, vector<Prefix> *oldParentOrSelfAndAncs, const pair<unsigned, unsigned> &problemKeywordIdPair, bool hadExactlyOneChild);

    // search o/c filters for the keyword ids to reassign
    // gather the corresponding forwardlists using the o-filter; decide which children quadnodes to go using the c-filter;
    // adjust the o/c filters prefix
    void gatherForwardListsAndAdjustOCFilters(QuadTree *quadtree, unsigned oldKeywordId, unsigned newKeywordId, map<unsigned, unsigned> &recordIdsToProcess) const;

    unsigned getNumberOfKeywordsInCFilter(CFilterIndex* cFilter)
    {
        return cFilter->cFilterIndex.at(this->cFilterOffset)->cmp->size();
    }

    unsigned getNumberOfKeywordsInOFilter(OFilterIndex* oFilter)
    {
        return oFilter->oFilterIndex.at(this->oFilterOffset)->omp->size();
    }

    unsigned getNumberOfBytes()
    {
        unsigned result = 3*sizeof(unsigned) + sizeof(bool) + 4*sizeof(double);
        if(isLeaf)
        {
            return result += entries.size() * 2 * sizeof(unsigned);
        }
        else
        {
            return result += CHILD_NUM * 2 * sizeof(unsigned);
        }
    }

    bool equalTo(QuadNode* qn)
    {
        if(this->entries.size() != qn->entries.size())
            return false;

        if(this->isLeaf)
        {
            for(unsigned i = 0; i < this->entries.size(); i++)
            {
                if( (intptr_t)this->entries[i] != (intptr_t)qn->entries[i] )
                    return false;
            }
        }
        else
        {
            for(unsigned i = 0; i < this->entries.size(); i++)
            {
                if(this->entries[i] == NULL && qn->entries[i] == NULL)
                    continue;
                else if(this->entries[i] == NULL && qn->entries[i] != NULL)
                    return false;
                else if(this->entries[i] != NULL && qn->entries[i] == NULL)
                    return false;
                else if( !(this->entries[i])->equalTo((QuadNode*)qn->entries[i]) )
                    return false;
            }
        }

        return (this->level == qn->level)
             & (this->isLeaf == qn->isLeaf)
             & (this->numElements == qn->numElements)
             & (this->mbr == qn->mbr)
             & (this->cFilterOffset == qn->cFilterOffset)
             & (this->oFilterOffset == qn->oFilterOffset);
    };

private:

    // If the number of the geoElements of a leaf node is going to be larger than BRANCH_FACTOR,
    // change this leaf node to a intermediate node, add children leaf nodes as needed, and move all geoElements into them
    void split(QuadTree* quadTree, GeoElement &geoElement, unsigned geoElementOffset,
           bool needToConstructFilters, vector<Prefix> *skippedPrefixesFromAncestors);

    // When we add new record to the quadtree after commit, some existing o-filters could exceed the threshold.
    // In this case, we need to split the o-filter
    void splitOFilter(QuadTree *quadtree, const Trie *trie, OFilterMapPtr oMapPtr, CFilterMapPtr cMapPtr, GeoElementList *geoElementList,
                      const Prefix &prefixToSplit, unsigned newGeoElementOffset, const Prefix &keywordPrefixFromNewGeoElem,
                      const Prefix &prefixFromParent, const vector<OFilterMapPtr> &skipList, const set<unsigned> &insertedKeywords, Prefix &prefixToChild);

    // found this prefix or its ancsetor prefix on o-filter
    // add this prefix to the o-filter and adjust the o-filter accordingly
    void foundOnOFilter(QuadTree *quadtree, const Trie *trie, OFilterMap *oMapPtr, CFilterMap *cMapPtr,
                        const Prefix &prefix, const Prefix &prefixFromParent, GeoElementList *geoElementList, unsigned geoElementOffset, const vector<OFilterMapPtr> &skipList, const set<unsigned> &insertedKeywords,
                        Prefix &matchingPrefixOnOFilter, Prefix &prefixToChild, bool &canReturn);

    void ifNeedNewOFilterAndUpdateCFilter(QuadTree *quadtree, const Trie *trie, const Prefix &prefix, unsigned geoElementOffset, unsigned child,
                          OFilterMap *oMapPtr, CFilterMap *cMapPtr, Prefix &prefixToChild, bool &canReturn);

    void recoverCFiltersOnThisNode(QuadTree *quadtree, CFilterMapPtr cMapPtr, const GeoElementList &geoElementList, const Prefix &prefixToRecover,
                                    unsigned newGeoElementOffset, const Prefix &keywordPrefixFromNewGeoElem);

    // Add this prefix to c-filter using the geoElements corresponding to the prefix on the o-filter to be deleted shortly
    // Then, for each geoElement on the o-filter to be deleted shortly,
    // go to its corresponding child quad node to add it to the prefixToSplit o-filter (if it's not on prefixToSplit's ancestor o-filter there already)
    void recoverCFiltersOnThisNodeAndOFiltersOnChildrenNode(QuadTree *quadtree, OFilterMapPtr oMapPtr, CFilterMapPtr cMapPtr, GeoElementList *geoElementList, const Prefix &prefixToSplit);

    // Tell which children the location point belongs to
    unsigned findChildContainingPoint(Point point) const;

    // Define the mbr for a child according to its parent's mbr
    void createNewMBR(Rectangle &mbrNew, const Rectangle &mbr, const unsigned child);

    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        ar << this->level;
        ar << this->isLeaf;
        ar << this->numElements;
        ar << this->mbr;
        ar << this->cFilterOffset;
        ar << this->oFilterOffset;

        if(!this->isLeaf){
            ar << this->entries;
        }
        else{
            unsigned size = this->entries.size();
            ar << size;
            for (unsigned i = 0; i < size; i++)
            {
                intptr_t offset = (intptr_t)entries[i];
                ar << offset;
            }
        }
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        ar >> this->level;
        ar >> this->isLeaf;
        ar >> this->numElements;
        ar >> this->mbr;
        ar >> this->cFilterOffset;
        ar >> this->oFilterOffset;

        if(!this->isLeaf){
            ar >> this->entries;
        }
        else{
            unsigned size;
            ar >> size;
            entries.resize(size);
            for(unsigned i = 0; i < size; i++)
            {
                intptr_t offset;
                ar >> offset;
                entries[i] = (QuadNode*)offset;
            }
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};
/*
 * Construction phase :
 * Phase 1 : All QuadNodes are inserted w/o any filters.
 * Phase 2 : O and C filters are added to the node using a top-down approach.
 *
 *
 *
 */
class QuadTree
{
private:

    QuadNode *root;

    const ForwardIndex *forwardIndex;
    const Trie *trie;

    CFilterIndex * cFiltersOfQuadTree;
    OFilterIndex * oFiltersOfQuadTree;

    vector< pair< unsigned, vector<unsigned>* > > newGeoRecordsToMerge;
    vector<InfoToFixBroadenPrefixesOnFilters> infoToFixBroadenPrefixesOnFilters;

    unsigned totalNumOfInternalQuadNodes;



    friend class boost::serialization::access;
    friend class QuadNode;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {

        ar & this->root;
        ar & this->cFiltersOfQuadTree;
        ar & this->oFiltersOfQuadTree;
        ar & this->geoElementIndex;

        ar & this->totalNumOfInternalQuadNodes;

    }

    OFilterMapPtr getOFilterMapPtr(const QuadNode * const node) const { return this->oFiltersOfQuadTree->getOFilterMap(node->oFilterOffset); }
    CFilterMapPtr getCFilterMapPtr(const QuadNode * const node) const { return this->cFiltersOfQuadTree->getCFilterMap(node->cFilterOffset); }

    /* *
     * * the filter construction phase
     * */
    void createFilters(const vector<Prefix> *skipListFromParent, QuadNode *node, unsigned &processedInternalQuadNodesNum);

    // calculate the memory usage of CFilters and QuadTree
    void getNumberOfBytesForSubtree(QuadNode* node, unsigned &cFilterBytes, unsigned &oFilterBytes, unsigned &treeBytes, unsigned &totalNumOfKeywordsInCFilter, unsigned &totalNumOfKeywordsInOFilter) const;

    /* *
     * * the search phase
     * */
    // Do a geo-information with single keyword range query under the specified node
    void rangeQueryInternal(QueryResultsInternal *queryResultsInternal, const Shape &range, vector<MapSearcherTerm> mapSearcherTermVector, const SpatialRanker *ranker, QuadNode *node, set<unsigned> &listOfTrueAnswersForPickedTerm, const float prefixMatchPenalty, double &timer) const;

    // verify if all prefixes/keywords appear in the forward list
    bool verify(const ForwardList* forwardList, const SpatialRanker *ranker, unsigned forwardListID, const float prefixMatchPenalty, const vector<MapSearcherTerm> &mapSearcherTermVector, float &overallScore, unsigned termToSkip, vector<unsigned> &selectedExpansions, set<unsigned> &listOfTrueAnswersForPickedTerm, double &timer, ExpansionStructure *skippedExpansion = NULL) const;

    // combine all prefixes/keywords's bit vectors
    void combineBitVectorsOnCFilter(const vector<MapSearcherTerm> &mapSearcherTermVector, const CFilterMapPtr &cMapPtr, ChildBitSet &childrenBitVector) const;

    double getMinDist2UpperBound(const Shape &shape) const;

    double getDistanceScore(const SpatialRanker *ranker, const Shape &shape, const double resultLat, const double resultLng) const;


   // Do a geo query with a range but without keywords
  void rangeQueryWithoutKeywordInformation(QueryResultsInternal *queryResultsInternal, const Shape &shape, QuadNode *node) const;

    /* *
     * *  the update phase
     * */

    void fixFiltersBroaden();

    // get all the prefixes stored in the quad node's o-filter
    void createSkipListToChild(QuadNode *node, vector<Prefix> *prefixVector, vector<Prefix> *skippedPrefixesToChild) const;

public:
     // This is here because we don't want unnecessary copies during node split.
    vector<GeoElement*> geoElementIndex; //TODO make it private

    QuadTree(ForwardIndex *forwardIndex, Trie *trie);

    QuadTree()
    {
        this->root = NULL;
        this->cFiltersOfQuadTree = NULL;
        this->oFiltersOfQuadTree = NULL;
        this->totalNumOfInternalQuadNodes = 0;
    };

    virtual ~QuadTree()
    {
        if(this->root != NULL)
            delete this->root;
        if(this->cFiltersOfQuadTree != NULL)
            delete this->cFiltersOfQuadTree;
        if(this->oFiltersOfQuadTree != NULL)
            delete this->oFiltersOfQuadTree;

        for(unsigned i = 0; i < geoElementIndex.size(); i++)
        {
            if(geoElementIndex[i] != NULL)
                delete geoElementIndex[i];
        }
        this->geoElementIndex.clear();
    }

    void setForwardIndex(ForwardIndex *forwardIndex)
    {
        this->forwardIndex = forwardIndex;
    };

    const ForwardIndex *getForwardIndex() 
    {
        return this->forwardIndex;
    };

    void setTrie(Trie *trie)
    {
        this->trie = trie;
    };

    const Trie* getTrie() const
    {
        return this->trie;
    }

    unsigned getTotalNumOfInternalQuadNodes()
    {
        return this->totalNumOfInternalQuadNodes;
    }

    void increaseInternalQuadNodeNumber()
    {
        this->totalNumOfInternalQuadNodes++;
    }

    // Add a Record to the QuadTree
    void addRecordBeforeCommit(const Record *record, unsigned recordInternalId);

    // Add a Record to the QuadTree after the commit
    void addRecordAfterCommit(const Record *record, unsigned recordInternalId, vector<unsigned> *keywordIdVector);

    void updateInfoToFixBroadenPrefixesOnFilters(unsigned leftOrRight,
                                                 vector<Prefix> *oldParentOrSelfAndAncs,
                                                 unsigned problemKeywordId,
                                                 bool hadExactlyOneChild) 
    {
        this->infoToFixBroadenPrefixesOnFilters.push_back( InfoToFixBroadenPrefixesOnFilters(oldParentOrSelfAndAncs,
                                                                                          problemKeywordId,
                                                                                          hadExactlyOneChild,
                                                                                          leftOrRight)
                                                      );
    }

    static bool needToFix(const pair<unsigned, unsigned> &keywordIdPair, const Prefix &prefix);

    void merge();

    void gatherForwardListsAndAdjustOCFilters(const map<unsigned, unsigned> &keywordIdMapper, map<unsigned, unsigned> &recordIdsToProcess);
    void fixReassignedIds(const map<unsigned, unsigned> &keywordIdMapper);

    // the filter construction phase
    // recursively create cFilters and oFilters for each QuadNode
    // should be only called inside IndexInternal's commit()
    void createFilters();


    // Do a geo query with a range but without keywords
    void rangeQueryWithoutKeywordInformation(const Shape &shape,QueryResultsInternal *queryResultsInternal) const;

    // Do a geo-information with single keyword range query under the specified node
    void rangeQuery(QueryResultsInternal *queryResultsInternal, const Shape &range, vector<MapSearcherTerm> mapSearcherTermVector, const SpatialRanker *ranker, const float prefixMatchPenalty) const;

    // calculate the memory usage of CFilters and QuadTree
    void getNumberOfBytes(unsigned &cFilterBytes, unsigned &oFilterBytes, unsigned &treeBytes, unsigned &totalNumOfKeywordsInCFilter, unsigned &totalNumOfKeywordsInOFilter);

    // serialize the QuadTree
    static void save(const QuadTree &quadTree, const string &quadTreeFullPathFileName)
    {
        ofstream ofs(quadTreeFullPathFileName.c_str(), std::ios::binary);
        boost::archive::binary_oarchive oa(ofs);
        oa << quadTree;
        ofs.close();
    };

    // deserialize the QuadTree
    static void load(QuadTree &quadTree, const string &quadTreeFullPathFileName)
    {
        ifstream ifs(quadTreeFullPathFileName.c_str(), std::ios::binary);
        boost::archive::binary_iarchive ia(ifs);
        ia >> quadTree;
        ifs.close();
    };

    QuadNode* getRoot()
    {
        return this->root;
    };

    CFilterIndex* getCFilterIndex()
    {
        return this->cFiltersOfQuadTree;
    };

    OFilterIndex* getOFilterIndex()
    {
        return this->oFiltersOfQuadTree;
    };

    // if the input QuadTree equals to this QuadTree
    bool equalTo(QuadTree *qt)
    {
        if(this->totalNumOfInternalQuadNodes != qt->totalNumOfInternalQuadNodes)
            return false;

        if(this->geoElementIndex.size() != qt->geoElementIndex.size())
            return false;
        for(unsigned i = 0; i < geoElementIndex.size(); i++)
        {
            if( !(*(this->geoElementIndex[i]) == *(qt->geoElementIndex[i])) )
                return false;
        }

        return this->root->equalTo(qt->getRoot())
             & this->cFiltersOfQuadTree->equalTo(qt->getCFilterIndex())
             & this->oFiltersOfQuadTree->equalTo(qt->getOFilterIndex());
    };

};

}}

#endif /* __QUADTREE_H__ */
