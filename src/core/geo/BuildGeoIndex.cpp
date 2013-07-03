
// $Id: BuildGeoIndex.cpp 3294 2013-05-01 03:45:51Z jiaying $

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

#include <iostream>
#include <stdlib.h>
#include <time.h>

#include "QuadTree.h"
#include "index/ForwardIndex.h"

using namespace std;

namespace bimaple
{
namespace instantsearch
{

QuadTree::QuadTree(ForwardIndex *forwardIndex, Trie *trie)
{
    this->log_level = -1;

    this->cFiltersOfQuadTree = new CFilterIndex();
    this->oFiltersOfQuadTree = new OFilterIndex();

    this->forwardIndex = forwardIndex;
    this->trie = trie;

    this->totalNumOfInternalQuadNodes = 0;

    root = new QuadNode();

    root->level = 0;
    root->isLeaf = true;

    // the root includes the whole map
    root->mbr.min.x = BOTTOM_LEFT_X;
    root->mbr.min.y = BOTTOM_LEFT_Y;
    root->mbr.max.x = TOP_RIGHT_X;
    root->mbr.max.y = TOP_RIGHT_Y;

    root->numElements = 0;

}

void QuadTree::createFilters()
{
    if(!forwardIndex->isCommitted())
    {
        cerr << "Trie and ForwardIndex are not committed yet, cannot create filters" << endl;
        return;
    }
    if(!root->isLeaf) // if we only have root, then we don't need filters
    {
        vector<Prefix> *skipListFromParent = new vector<Prefix>();
        unsigned processedInternalQuadNodesNum = 0;
        createFilters(skipListFromParent, root, processedInternalQuadNodesNum);
        delete skipListFromParent;
    }
}

void QuadTree::createFilters(const vector<Prefix>* skipListFromParent, QuadNode *node, unsigned &processedInternalQuadNodesNum)
{
    // a new skiplist that will be passed to children
    const vector<Prefix> *skipListToChildren;
    vector<Prefix> *newSkipListToChildren = NULL;

    CFilterMapPtr cMapPtr = new CFilterMap();
    OFilterMapPtr oMapPtr = new OFilterMap();

    // Go down to every leaf node under the current QuadNode
    // Get raw o-filters and raw c-filters back
    node->fetchRawFiltersFromleafNodesUnderThisNode(this->trie, this->forwardIndex, this->geoElementIndex, cMapPtr, oMapPtr, true);
    oMapPtr->gatherFreqOfAllAncestorPrefixesForOFilters(this->trie);

    bool createdNewSkipList = false;
    if(oMapPtr->omp->size())
    {// if we have o-filter then we need to create a new skiplist, otherwise we don't need to
        newSkipListToChildren = new vector<Prefix>();
        createdNewSkipList = true;

        oMapPtr->removePopularAndUselessDescendantsOFilters(*newSkipListToChildren, *skipListFromParent);

        cMapPtr->gatherFreqOfAllAncestorPrefixesForCFilters(this->trie);
        cMapPtr->removePopularAndUselessDescendantsCFilters(*newSkipListToChildren);

        skipListToChildren = newSkipListToChildren;
    }
    else
    {
        cMapPtr->gatherFreqOfAllAncestorPrefixesForCFilters(this->trie);
        cMapPtr->removePopularAndUselessDescendantsCFilters(*skipListFromParent);

        skipListToChildren = skipListFromParent;
    }

    // add cFilter to this node
    this->cFiltersOfQuadTree->addCFilterMapToIndex(cMapPtr);
    node->cFilterOffset = this->cFiltersOfQuadTree->cFilterIndex.size() - 1;

    // add oFilter to this node
    this->oFiltersOfQuadTree->addOFilterMapToIndex(oMapPtr);
    node->oFilterOffset = this->oFiltersOfQuadTree->oFilterIndex.size() - 1;

    // output progress
    processedInternalQuadNodesNum++;
    //if(processedInternalQuadNodesNum % 100 == 0 || processedInternalQuadNodesNum == this->getTotalNumOfInternalQuadNodes())
    //    cout << "\rNow " << processedInternalQuadNodesNum << " out of " << this->getTotalNumOfInternalQuadNodes() << " internal quadnodes have been processed";

    for(unsigned i = 0; i < CHILD_NUM; i++)
        if(node->entries[i] != NULL && !(node->entries[i])->isLeaf)
            createFilters(skipListToChildren, node->entries[i], processedInternalQuadNodesNum);

    if (createdNewSkipList)
    {
        delete newSkipListToChildren;
    }
}

void QuadTree::addRecordBeforeCommit(const Record *record, unsigned recordInternalId)
{
    GeoElement *geoElement = new GeoElement(record, recordInternalId);
    this->geoElementIndex.push_back(geoElement);
    unsigned geoElementOffset = this->geoElementIndex.size() - 1;
    // Before commit, we do not need to construct the C-filter and O-filter
    root->insertGeoElement(this, *geoElement, geoElementOffset);
}

void QuadTree::addRecordAfterCommit(const Record *record, unsigned recordInternalId, vector<unsigned> *keywordIdVector)
{
    // add the record to the GeoElementIndex
    GeoElement *geoElement = new GeoElement(record, recordInternalId);
    this->geoElementIndex.push_back(geoElement);
    unsigned geoElementOffset = this->geoElementIndex.size() - 1;

    // store the new record to do the merge later, at that time we will take care of O/C Filters
    this->newGeoRecordsToMerge.push_back(make_pair(geoElementOffset, keywordIdVector));
}

void QuadTree::merge()
{
	// CHEN: Use the new intervals to fix those intervals in filters
	// Jamshid : This function goes down the tree and broadens all the intervals
	// so in the following steps of this process that we call insertGeoElement
	// all intervals are already updated and the skipList which is sent down (because of
	// true value of the input argument) is consistent with subtree filters.
    fixFiltersBroaden();

    for (unsigned i = 0; i < this->newGeoRecordsToMerge.size(); i++) // for each newly inserted geoElement
    {
        /// Pre-insert GeoElement to the QuadTree

        unsigned geoElementOffset = this->newGeoRecordsToMerge[i].first;
        //if (geoElementOffset > 310000)
        //    this->log_level = 0;
        //if (geoElementOffset % 1000 == 0)
        //    cout << geoElementOffset / 1000 << endl;
        LOG_REGION(this->log_level,
            cout << "****** " << geoElementOffset << " ******" << endl;
        );
        GeoElement* geoElement = this->geoElementIndex[geoElementOffset];

        // make an empty vector of skipped prefixes before traversing the quad tree
        vector<Prefix> *skippedPrefixesFromAncestors = new vector<Prefix>();

        // Jamshid : "true" means we need to construct the C-filter and O-filter when
        // we split a leaf node
        root->insertGeoElement(this, *geoElement, geoElementOffset, true, skippedPrefixesFromAncestors);

        delete skippedPrefixesFromAncestors;

        /// Take care of changes on O/C Filters brought by the newly inserted GeoElements
    
        set<unsigned> insertedKeywords;
        for (vector<unsigned>::const_iterator constIterator = this->newGeoRecordsToMerge[i].second->begin();
                        constIterator != this->newGeoRecordsToMerge[i].second->end();
                        ++constIterator) { // for each keyword of the newly inserted geoElement, add them to the geo index
            LOG_REGION(this->log_level,
                cout << "== One keyword ==" << endl;
            );
            vector<OFilterMapPtr> skipList;
            root->addRecordToSubQuadTreeAfterCommit(this, *constIterator, this->newGeoRecordsToMerge[i].first, Prefix(Trie::MAX_KEYWORD_ID, Trie::MAX_KEYWORD_ID), skipList, insertedKeywords);
            insertedKeywords.insert(*constIterator);
        }
        delete newGeoRecordsToMerge[i].second;
    }

    this->newGeoRecordsToMerge.clear();

}



bool QuadTree::needToFix(const pair<unsigned, unsigned> &keywordIdPair, const Prefix &prefix)
{
    if (keywordIdPair.first < prefix.minId)
        return true; // CHEN: [3, 7] (original prefix on O-filter) versus [2, MAX_KEYWORD_ID]

    if (keywordIdPair.second == (Trie::MAX_KEYWORD_ID))
        return false;  // CHEN: [3, 7] (original prefix on O-filter) versus [4, MAX_KEYWORD_ID]

    if (keywordIdPair.second > prefix.maxId)
        return true; // CHEN: [3, 7] (original prefix on O-filter) versus [MAX_KEYWORD_ID, 9]
    else
        return false;
}

void QuadTree::fixReassignedIds(const map<unsigned, unsigned> &keywordIdMapper)
{
    // fix the reassigned Ids in the problemKeyword map
    // e.g. we have a reassignment: 17->22, 16->14
    //              an InfoToFixBroadenPrefixesOnFilters set : "can", ([1,16], [3, 16]), 17, true
    //      then we need to change the set to: "can", ([1,14], [3, 14]), 22, true

    for (vector<InfoToFixBroadenPrefixesOnFilters>::iterator it = this->infoToFixBroadenPrefixesOnFilters.begin();
            it != this->infoToFixBroadenPrefixesOnFilters.end(); it++)
    {
        if (keywordIdMapper.find(it->problemKeywordIdPair.first) != keywordIdMapper.end())
            it->problemKeywordIdPair.first = keywordIdMapper.at(it->problemKeywordIdPair.first);
        if (keywordIdMapper.find(it->problemKeywordIdPair.second) != keywordIdMapper.end())
            it->problemKeywordIdPair.second = keywordIdMapper.at(it->problemKeywordIdPair.second);

        for (unsigned i = 0; i < it->oldParentOrSelfAndAncs->size(); i++)
        {
            if (keywordIdMapper.find(it->oldParentOrSelfAndAncs->at(i).minId) != keywordIdMapper.end())
                it->oldParentOrSelfAndAncs->at(i).minId = keywordIdMapper.at(it->oldParentOrSelfAndAncs->at(i).minId);
            if (keywordIdMapper.find(it->oldParentOrSelfAndAncs->at(i).maxId) != keywordIdMapper.end())
                it->oldParentOrSelfAndAncs->at(i).maxId = keywordIdMapper.at(it->oldParentOrSelfAndAncs->at(i).maxId);
        }

    }
}

void QuadTree::fixFiltersBroaden()
{
    for ( vector<InfoToFixBroadenPrefixesOnFilters>::iterator it = this->infoToFixBroadenPrefixesOnFilters.begin();
            it != this->infoToFixBroadenPrefixesOnFilters.end(); it++ )
    {
        LOG_REGION(this->log_level,
            cout << "fix " << it->oldParentOrSelfAndAncs->back().minId << " " << it->oldParentOrSelfAndAncs->back().maxId << " because of "<< it->problemKeywordIdPair.first << " " << it->problemKeywordIdPair.second << " " << it->hadExactlyOneChild << endl;
        );

        root->fixFiltersBroadenOnOneNode(this, it->oldParentOrSelfAndAncs, it->problemKeywordIdPair, it->hadExactlyOneChild);

        delete it->oldParentOrSelfAndAncs;
    }
    infoToFixBroadenPrefixesOnFilters.clear();
}

void QuadTree::gatherForwardListsAndAdjustOCFilters(const map<unsigned, unsigned> &keywordIdMapper, map<unsigned, unsigned> &recordIdsToProcess)
{
    // Jamshid : for the mapping of temporary id --> newly assigned id
    // since the keywords with temporary ids are only in those newly inserted record,
    // we can just go to the ForwardLists of those records to check if there is any the new keyword ids
	// since these records are not added to the quadTree yet we don't have to touch the tree for these new keywords.
	// this loops is just replacing new record keyword temporary ids with new ids.
    for (unsigned i = 0; i < this->newGeoRecordsToMerge.size(); i++)
    {
        bool hasReassignedKeyword = false;

        // check if any keyword ids in the keywordIdIsNewTrieNodeMap need to be change
        vector<unsigned> *keywordIdVector = new vector<unsigned> ();
        for ( vector<unsigned>::iterator iter = newGeoRecordsToMerge[i].second->begin();
                iter != newGeoRecordsToMerge[i].second->end(); iter++ )
        {
            if (keywordIdMapper.find(*iter) != keywordIdMapper.end())
            {
                hasReassignedKeyword = true;
                unsigned newKeywordId = keywordIdMapper.at(*iter);
                keywordIdVector->push_back(newKeywordId);
            }
            else
                keywordIdVector->push_back(*iter);
        }

        // if the record has newly assigned keyword ids
        if (hasReassignedKeyword)
        {
            delete newGeoRecordsToMerge[i].second;
            newGeoRecordsToMerge[i].second = keywordIdVector;
        
            unsigned recordId = this->geoElementIndex[newGeoRecordsToMerge[i].first]->forwardListID;
            recordIdsToProcess[recordId] = 0; // add it to the set
        }
        else
            delete keywordIdVector;
    }

    // Jamshid : find the beginning position of the temporary ids on keywordIdMapper,
    // which is the position of the first old id > MAX_ALLOCATED_KEYWORD_ID
    // we only need to look for the keyword ids before that position on keywordIdMapper
    // because all the temporary ids was already taken care of above
    // this map is sorted by default
    map<unsigned, unsigned>::const_iterator positionBig = keywordIdMapper.upper_bound( (unsigned)(Trie::MAX_ALLOCATED_KEYWORD_ID) );
    ASSERT(positionBig != keywordIdMapper.end());
    ASSERT(positionBig->first > Trie::MAX_ALLOCATED_KEYWORD_ID);

    // for the mapping of old keyword id --> newly assigned id
    // in order to find all the ForwardLists that contain those old keyword ids,
    // we need to go to the QuadTree to search for those old keyword ids.
    // also, we need to adjust the o/c filters

    for (map<unsigned, unsigned>::const_iterator cit = keywordIdMapper.begin(); cit != positionBig; cit++)
    {
        this->root->gatherForwardListsAndAdjustOCFilters(this, cit->first, cit->second, recordIdsToProcess);
    }
}

void QuadTree::getNumberOfBytes(unsigned &cFilterBytes, unsigned &oFilterBytes, unsigned &treeBytes, unsigned &totalNumOfKeywordsInCFilter, unsigned &totalNumOfKeywordsInOFilter)
{
    cFilterBytes = 0;
    oFilterBytes = 0;
    treeBytes = 0;
    totalNumOfKeywordsInCFilter = 0;
    totalNumOfKeywordsInOFilter = 0;

    getNumberOfBytesForSubtree(root, cFilterBytes, oFilterBytes, treeBytes, totalNumOfKeywordsInCFilter, totalNumOfKeywordsInOFilter);


    cFilterBytes += sizeof(cFiltersOfQuadTree->cFilterIndex); // adding the overhead of the global cFilter vector
    oFilterBytes += sizeof(oFiltersOfQuadTree->oFilterIndex); // adding the overhead of the global cFilter vector
}

void QuadTree::getNumberOfBytesForSubtree(QuadNode* node, unsigned &cFilterBytes, unsigned &oFilterBytes, unsigned &treeBytes, unsigned &totalNumOfKeywordsInCFilter, unsigned &totalNumOfKeywordsInOFilter) const
{
    cFilterBytes = cFilterBytes + (sizeof(CFilterMapPtr) + 2 * sizeof(unsigned) + sizeof(ChildBitSet)) * node->getNumberOfKeywordsInCFilter(cFiltersOfQuadTree)
            + sizeof(vector< CFilterMapPtr >);

    oFilterBytes = oFilterBytes + (sizeof(OFilterMapPtr) + 2 * sizeof(unsigned) + OBJECT_SELECTIVITY_THRESHOLD * sizeof(unsigned) + 3 * sizeof(unsigned)) * node->getNumberOfKeywordsInOFilter(oFiltersOfQuadTree)
            + sizeof(vector< OFilterMapPtr >);

    treeBytes += node->getNumberOfBytes();

    totalNumOfKeywordsInCFilter = totalNumOfKeywordsInCFilter + node->getNumberOfKeywordsInCFilter(cFiltersOfQuadTree);
    totalNumOfKeywordsInOFilter = totalNumOfKeywordsInOFilter + node->getNumberOfKeywordsInOFilter(oFiltersOfQuadTree);

    if(this->root->isLeaf) // To handle the case of only one node - which is a root node
            return; // you need to return here, as if its only root node it is a leaf node
                    // Hence we cannot type cast it to QuadNode as the leaf node has geoElements instead of childNodes
    for(unsigned counter = 0; counter< CHILD_NUM; counter++)
    {
        if(node->entries[counter]!= NULL)
        {
            QuadNode* childNode = node->entries[counter];
            if(childNode->isLeaf != true)
            {
                getNumberOfBytesForSubtree(childNode, cFilterBytes, oFilterBytes, treeBytes, totalNumOfKeywordsInCFilter, totalNumOfKeywordsInOFilter);

            }
            else
            {
                treeBytes += childNode->getNumberOfBytes();
                // Calculating the bytes used by the tree by summing the memory usage of each node
            }
        }
    }
}



}}
