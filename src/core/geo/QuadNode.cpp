// $Id: QuadNode.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

#include <iomanip>
#include "QuadTree.h"
#include "index/ForwardIndex.h"
#include "util/Logger.h"
using namespace std;
using srch2::util::Logger;

namespace srch2
{
namespace instantsearch
{

// If needToConstructFilters is true, then do NOT consider the geoElement's keyword frequencies
// when constructing the O/C filters of a new QuadNode.
//
// "geoElementOffset" is the offset of the geoElement (for a leaf node) or a pointer to a child (for
// an intermediate node.  It has a type of "QuanNode *" since it shares the same space
// as the child pointers of a node)
void QuadNode::insertGeoElement(QuadTree* quadTree, GeoElement &geoElement, unsigned geoElementOffset,
                bool needToConstructFilters, vector<Prefix> *skippedPrefixesFromAncestors)
{
    if (this->isLeaf) { // a leaf node
        // split the leaf if full
        if (this->numElements > BRANCH_FACTOR
        		// We don't want to have too small regions, also it is possible to have many elements on the same point
                && ((this->mbr.max.x - this->mbr.min.x) * (this->mbr.max.y - this->mbr.min.y)) > MBR_LIMIT) {
            this->split(quadTree, geoElement, geoElementOffset, needToConstructFilters, skippedPrefixesFromAncestors);
            return;
        }

        // Not full.  Add the element into the leaf
        this->numElements ++;
        // add the geoElementOffset (integer) in the node
        // cast it to a type of "QuanNode *" since it shares the same space as the child pointers of a node
        // TODO try to figure out a way to avoid casting
        this->entries.push_back((QuadNode *)geoElementOffset);
        return;
    }

    // the node is an intermediate node
    this->numElements ++;

    // Finds the child based on location information
    unsigned child = findChildContainingPoint(geoElement.point);

    if (this->entries[child] != NULL) { // there is a child where the geoElement is to be inserted
        if (needToConstructFilters) {

        	// Jamshid : This branch is used in update phase only
            // use the skippedPrefixesFromAncestors and the O-filter on this quad node
            // to create the skippedPrefixesToChild

            vector<Prefix> *skippedPrefixesToChild = new vector<Prefix>();
            quadTree->createSkipListToChild(this, skippedPrefixesFromAncestors, skippedPrefixesToChild);



            // recursively call this function at the corresponding child
            this->entries[child]->insertGeoElement(quadTree, geoElement, geoElementOffset,
                                                     needToConstructFilters, skippedPrefixesToChild);

            delete skippedPrefixesToChild;
        }

        else
        {
            // recursively call this function at the corresponding child
            this->entries[child]->insertGeoElement(quadTree, geoElement, geoElementOffset);
        }

        return;
    }

    // The node doesn't have a child.  We need to create a new child.
    // In case we need to construct filters for these nodes, we don't do the construction here.
    // Instead, we will do the construction in the split() function.
    QuadNode *newNode = new QuadNode();
    newNode->level = this->level + 1;
    newNode->isLeaf = true;
    createNewMBR(newNode->mbr, this->mbr, child);
    
    // add the geoElement
    newNode->numElements = 1;
    // cast it to a type of "QuanNode *" since it shares the same space as the child pointers of a node
    newNode->entries.push_back((QuadNode *)geoElementOffset);
    
    // link current node to the new node
    this->entries[child] = newNode;
    return;
}

void QuadNode::split(QuadTree* quadTree, GeoElement &geoElement, unsigned geoElementOffset,
             bool needToConstructFilters, vector<Prefix> *skippedPrefixesFromAncestors)
{
    ASSERT(this->isLeaf == true);
    this->isLeaf = false; // after split, the node will no longer be a leaf
    quadTree->increaseInternalQuadNodeNumber();
    
    vector<QuadNode *> geoElementVector;
    for (unsigned i = 0; i < this->numElements; i++) // put all records contained by this node into a vector
        geoElementVector.push_back(this->entries[i]);

    this->numElements = 0;
    this->entries.clear();
    for (unsigned i = 0; i < CHILD_NUM; i++) // set child points to NULL.
        this->entries.push_back(NULL);

    // reinsert the original elements to this quad node, which is now an intermediate node
    for (unsigned i = 0; i < geoElementVector.size(); i++) {
        intptr_t offset = (intptr_t)geoElementVector[i];

        // During this reinsertion, we do NOT need to construct filters
        this->insertGeoElement(quadTree, *quadTree->geoElementIndex[offset], offset);
    }

    // if the quadtree is committed, we need to build the C-filter and O-filter of
    // this quad node (without considering the new geoElement, since its frequencies
    // should not be included in these filters. These frequences will be utlized
    // when we later traverse the quad tree topdown.
    if (needToConstructFilters) {
        unsigned processedInternalQuadNodesNum;
        quadTree->createFilters(skippedPrefixesFromAncestors, this, processedInternalQuadNodesNum);
    }
    
    // Now we need to insert the new geoElement.
    // We still need to pass the needToConstructFilters to the call in case the new element
    // caused more split of the child
    this->insertGeoElement(quadTree, geoElement, geoElementOffset,
            needToConstructFilters, skippedPrefixesFromAncestors);
}

// define the mbr for a child according to its parent's mbr
void QuadNode::createNewMBR(Rectangle &mbrNew, const Rectangle &mbr, const unsigned child)
{
    unsigned x = (unsigned)(child % CHILD_NUM_SQRT);
    unsigned y = (unsigned)(child / CHILD_NUM_SQRT);

    double single = (mbr.max.x - mbr.min.x) / CHILD_NUM_SQRT;

    mbrNew.min.x = mbr.min.x + x * single;
    mbrNew.min.y = mbr.min.y + y * single;
    mbrNew.max.x = mbr.min.x + (x + 1) * single;
    mbrNew.max.y = mbr.min.y + (y + 1) * single;
}

// tell which child the location point belongs to
unsigned QuadNode::findChildContainingPoint(Point point) const
{
    double xRatio = (point.x - this->mbr.min.x) / (this->mbr.max.x - this->mbr.min.x);
    double yRatio = (point.y - this->mbr.min.y) / (this->mbr.max.y - this->mbr.min.y);

    unsigned x = (unsigned)(xRatio * CHILD_NUM_SQRT);
    unsigned y = (unsigned)(yRatio * CHILD_NUM_SQRT);

    if((x + y * CHILD_NUM_SQRT)>=CHILD_NUM){
        Logger::debug("%.10f,%d,%.10f,%.10f", yRatio, y, this->mbr.min.y,  this->mbr.max.y);
        Logger::debug("%.10f,%.10f", point.x, point.y);
    }

    return x + y * CHILD_NUM_SQRT;
}

void QuadNode::fetchRawFiltersFromleafNodesUnderThisNode(const Trie *trie, const ForwardIndex *forwardIndex, const vector<GeoElement*> &geoElementIndex, CFilterMapPtr cMapPtr, OFilterMapPtr oMapPtr, bool isDirectChild, unsigned childBit)
{
    if(this->isLeaf) // quad  node is a leaf node
    {
        for(unsigned i = 0; i < this->numElements; i++)
        {
            intptr_t geoElementOffset = (intptr_t)this->entries[i];
            bool isValidForwardList = false;
            const ForwardList* forwardList = forwardIndex->getForwardList(geoElementIndex[geoElementOffset]->forwardListID, isValidForwardList);
            if (!isValidForwardList)
                continue;

            for (unsigned counter = 0; counter < forwardList->getNumberOfKeywords(); counter++)
            {
                // get keywords from each geoElement's corresponding forward list
                unsigned keywordId = forwardList->getKeywordId(counter);

                Prefix prefix;
                trie->getPrefixFromKeywordId(keywordId, prefix);

                // add keywords to cFilter
                if(cMapPtr->cmp->find(prefix) == cMapPtr->cmp->end())
                {
                    (*cMapPtr->cmp)[prefix].reset();
                }
                (*cMapPtr->cmp)[prefix].set(childBit);

                // add keywords and corresponding geoElement to oFilter
                oMapPtr->updateOFilter(prefix, (unsigned)geoElementOffset, true);
            }
        }
    }
    else
    { // the current quad node is not a leaf node
        for(unsigned i = 0; i < CHILD_NUM; i++)
        {
        	// all the subtree below the ith child receives childBit i, so if it is direct
        	// child it sets it to i, and if it is under the direct child it just passes the childBit to children.
            if(this->entries[i] != NULL)
            {
                unsigned childBitToChildren;
                if(isDirectChild) // fix which bit we want to set for the original caller node at the first level, don't change it below
                    childBitToChildren = i;
                else
                    childBitToChildren = childBit;

                this->entries[i]->fetchRawFiltersFromleafNodesUnderThisNode(trie, forwardIndex, geoElementIndex, cMapPtr, oMapPtr, false, childBitToChildren);
            }
        }
    }
}


/**
 * Jamshid :
 * 1. Finds the OFilter
 * 2. Either it just adds the new record to the prefix.
 * 3. Or, splits the filter.
 *
 * @param prefix coming from the new record.
 * @param prefixFromParent the shortest longer prefix than @param matchingPrefixOnOFilter
 * @param geoElementList the list of records associated with this matching prefix in OFilter
 * @param geoElementOffset the new record
 * @param a list of OFilters from ancestors
 * @param stores the inserted keywords for the current geoElement (record)
 * @param matchingPrefixOnOFilter @param prefix's
 * match in OFilter (the match is either itself or one of its prefixes)
 * @param prefixToChild if we get some shorter prefix in this OFilter we need to update it. so @param prefixToChild
 * is always a prefix of @param prefixFromParent
 */
void QuadNode::foundOnOFilter(QuadTree *quadtree, const Trie *trie, OFilterMap *oMapPtr, CFilterMap *cMapPtr,
        const Prefix &prefix, const Prefix &prefixFromParent, GeoElementList *geoElementList, unsigned geoElementOffset, const vector<OFilterMapPtr> &skipList, const set<unsigned> &insertedKeywords,
        Prefix &matchingPrefixOnOFilter, Prefix &prefixToChild, bool &canReturn) 
{
    // add geoElementOffset to the found prefix in the OFilter
    geoElementList->add(geoElementOffset);

    if (geoElementList->geoElementOffsets.size() + geoElementList->freq <= OBJECT_SELECTIVITY_THRESHOLD){
        Logger::debug("Add to O-Filter without split.");
    }

    if (geoElementList->geoElementOffsets.size() + geoElementList->freq > OBJECT_SELECTIVITY_THRESHOLD)
    { // if oFilter's frequency exceeds the threshold after we add this geoElementId TODO change freq to the true size
      // we need to split the oFilter
        Prefix prefixToSplit = matchingPrefixOnOFilter;
        matchingPrefixOnOFilter = Prefix(Trie::MAX_KEYWORD_ID, Trie::MAX_KEYWORD_ID);
        this->splitOFilter(quadtree, trie, oMapPtr, cMapPtr, geoElementList, prefixToSplit, geoElementOffset, prefix, prefixFromParent, skipList, insertedKeywords, matchingPrefixOnOFilter);
    }
    // else, don't need to do anything

    vector<Prefix> ancestorPrefixes;
    if (matchingPrefixOnOFilter.minId != Trie::MAX_KEYWORD_ID || matchingPrefixOnOFilter.maxId != Trie::MAX_KEYWORD_ID)
    { // if add to a prefix on o-filter
        // if this is the shortest possible prefix, then no need to go down further
        // because all queries for the new element will be catched by this o-filter
        trie->getAncestorPrefixes(matchingPrefixOnOFilter, ancestorPrefixes);
        if (ancestorPrefixes.size() == 0)
            canReturn = true;

        else
            prefixToChild = ancestorPrefixes.back();
    }

}
/*
 * Jamshid: This function is called when the new prefix is not found in the OFilter.
 * It checks the CFilter, if found, it updates CFilter,
 * if not, it means this prefix is new so it adds the shortest prefix to the OFilter.
 */
void QuadNode::ifNeedNewOFilterAndUpdateCFilter(QuadTree *quadtree, const Trie *trie, const Prefix &prefix, unsigned geoElementOffset, unsigned child,
                                                OFilterMap *oMapPtr, CFilterMap *cMapPtr, Prefix &prefixToChild, bool &canReturn)
{
    vector<Prefix> ancestorPrefixes;
    trie->getAncestorPrefixes(prefix, ancestorPrefixes);

    // if we can find it on the c-filter
    if (cMapPtr->cmp->find(prefix) != cMapPtr->cmp->end())
    {
        Logger::debug("\tbranch 1");
        // update c-filter of this prefix and ancsetor prefixes
        for (unsigned i = 0; i < ancestorPrefixes.size(); i++)
            cMapPtr->updateCFilter(ancestorPrefixes[i], child);
        cMapPtr->updateCFilter(prefix, child);
        return;
    }

    else
    {
        // the prefix has never been here 
        // search ancestor prefixes on c-filter (impossible on o-filter, otherwise won't be here)
        Logger::debug("\tbranch 2");

        // get the shortest ancester prefix that's NOT on c-filter
        unsigned i = 0;
        for (; i < ancestorPrefixes.size(); i++)
            if (cMapPtr->cmp->find(ancestorPrefixes[i]) == cMapPtr->cmp->end())
                break;
        // put it on ofilter
        Prefix newPrefix = (i == ancestorPrefixes.size() ? prefix: ancestorPrefixes[i]);
        ASSERT(oMapPtr->omp->find(newPrefix) == oMapPtr->omp->end());
        oMapPtr->updateOFilter(newPrefix, geoElementOffset, false);
        Logger::debug("\tcreate new o-filter for %d %d" ,newPrefix.minId ,newPrefix.maxId );
        ASSERT(newPrefix.isAncestor(prefix));

        if (i == 0)
        {
            canReturn = true;
            return;
        }
        prefixToChild = ancestorPrefixes[i-1];

        // update c-filters for child
        for (unsigned i = 0; ancestorPrefixes[i].isAncestor(prefixToChild); i++)
            cMapPtr->updateCFilter(ancestorPrefixes[i], child);
    }
}

void QuadNode::addRecordToSubQuadTreeAfterCommit(QuadTree *quadtree, unsigned keywordId, unsigned geoElementOffset,
                                                    Prefix prefixFromParent, vector<OFilterMapPtr> &skipList, const set<unsigned> &insertedKeywords) 
{
    if ( this->isLeaf ) // if it's a leaf node
    {
        // don't need to add the geoElementOffset to the elementList of the leaf quadNode;
        // it's already inserted with QuadTree::addRecordAfterCommit().
        return;
    }

    /// it's an intermediate node
    
    // get the trie
    const Trie* trie = quadtree->getTrie();

    // get the oFilter map and cFilter map of this quad node
    OFilterMapPtr oMapPtr = quadtree->getOFilterMapPtr(this);
    
    ASSERT(oMapPtr != NULL);
    // not same as the skip list we use in construction phase
    skipList.push_back(oMapPtr);

    CFilterMapPtr cMapPtr = quadtree->getCFilterMapPtr(this);

    // get the prefix corresponding to the keyword
    Prefix prefix;
    trie->getPrefixFromKeywordId(keywordId, prefix);

    // similar to the use of skiplist
    // it's the parent prefix of the shortest prefix on all the ancestor's o-filters
    Prefix prefixToChild = prefixFromParent;

    // which child quad node to go down
    const unsigned child = this->findChildContainingPoint(quadtree->geoElementIndex[geoElementOffset]->point);

    // look for the prefix on the oFilter of this quadNode
    // it could find the ancestor of this prefix on the oFilter as the matchingPrefixOnOFilter
    // e.g. when we search for "candy", we could find "can" on the oFilter
    Prefix matchingPrefixOnOFilter;
    GeoElementList *geoElementList = oMapPtr->searchOFilter(prefix, matchingPrefixOnOFilter);

    if (geoElementList != NULL){
        Logger::debug("search for %d %d, found %d %d", prefix.minId, prefix.maxId, matchingPrefixOnOFilter.minId, matchingPrefixOnOFilter.maxId);
    }else{
        Logger::debug("search for %d %d, found nothing",prefix.minId, prefix.maxId); 
    }

    if (geoElementList != NULL) // find the prefix or its ancestor on the oFilter
    {
        bool canReturn = false;

        foundOnOFilter(quadtree, trie, oMapPtr, cMapPtr, prefix, prefixFromParent, geoElementList, geoElementOffset, skipList, insertedKeywords, matchingPrefixOnOFilter, prefixToChild, canReturn);

        if (canReturn)
            return;

        // update c-filters
        vector<Prefix> ancestorPrefixes;
        if (prefixToChild.minId != Trie::MAX_KEYWORD_ID || prefixToChild.maxId != Trie::MAX_KEYWORD_ID)
        {
            trie->getAncestorPrefixes(prefixToChild, ancestorPrefixes);
            ancestorPrefixes.push_back(prefixToChild);
        }
        else
        {
            trie->getAncestorPrefixes(prefix, ancestorPrefixes);
            ancestorPrefixes.push_back(prefix);
        }
        for (unsigned i = 0; i < ancestorPrefixes.size(); i++)
            cMapPtr->updateCFilter(ancestorPrefixes[i], child);


        Logger::debug("go to child");

        if (matchingPrefixOnOFilter.minId != Trie::MAX_KEYWORD_ID || matchingPrefixOnOFilter.maxId != Trie::MAX_KEYWORD_ID)
            ASSERT(prefixToChild.isStrictAncestor(matchingPrefixOnOFilter));

        // needs to go down the tree. Uses CFilter to call the same function recursively.
        this->entries[child]->addRecordToSubQuadTreeAfterCommit(quadtree, keywordId, geoElementOffset, prefixToChild, skipList, insertedKeywords);
        return;
    }

    // Jamshid : this prefix is not found in OFilter.
    bool canReturn = false;
    // if (prefixFromParent is None) or ( prefix.isAncestor(prefixFromParent) )
    if ( (prefixFromParent.minId == Trie::MAX_KEYWORD_ID && prefixFromParent.maxId == Trie::MAX_KEYWORD_ID)
            || prefix.isAncestor(prefixFromParent) )
    {
        Logger::debug("case 1");

        ifNeedNewOFilterAndUpdateCFilter(quadtree, trie, prefix, geoElementOffset, child, oMapPtr, cMapPtr, prefixToChild, canReturn);
    }
    // else if prefix is descendant of prefixFromParent
    else if (prefixFromParent.isStrictAncestor(prefix))
    {
        Logger::debug("case 2");

        ifNeedNewOFilterAndUpdateCFilter(quadtree, trie, prefixFromParent, geoElementOffset, child, oMapPtr, cMapPtr, prefixToChild, canReturn);
    }
    else
        ASSERT(false);

    if (canReturn)
        return;

    // recursively add the element to the subtree
    Logger::debug("go to child %d", child);

    this->entries[child]->addRecordToSubQuadTreeAfterCommit(quadtree, keywordId, geoElementOffset, prefixToChild, skipList, insertedKeywords);
    return;

}

/* Jamshid
 * @param prefixToSplit is a prefix which has passed the threshold after adding the new record and needs to split
 * @param newGeoElementOffset is the record which caused the split
 * @param keywordPrefixFromNewGeoElem prefixToSplit is an ancestor prefix of this keyword
 * @param prefixFromParent this one is not needed because we can't handle its job through skiplist but since we are
 * dealing only with one keyword keeping this prefix is better. It comes down from ancestors. If a node has multiple prefixes of the new
 * keyword the shortest prefix is passed down.
 * @param used for detecting duplicate keywords
 * @param matchingPrefixOnOFilter output of the function. The new filter after split.
 *
 */
void QuadNode::splitOFilter(QuadTree *quadtree, const Trie *trie, OFilterMapPtr oMapPtr, CFilterMapPtr cMapPtr, GeoElementList *geoElementList,
                            const Prefix &prefixToSplit, unsigned newGeoElementOffset, const Prefix &keywordPrefixFromNewGeoElem,
                            const Prefix &prefixFromParent, const vector<OFilterMapPtr> &skipList, const set<unsigned> &insertedKeywords, Prefix &matchingPrefixOnOFilter)
{

	// the structure to keep the new OFilter.
    OFilterMapPtr deltaOFilterMap = new OFilterMap();

    // iterates on all the records on the GEO list of this filter
    for (unsigned i = 0; i < geoElementList->geoElementOffsets.size(); i++)
    {
        // get the forward list
        unsigned geoElementOffset = geoElementList->geoElementOffsets[i];
        unsigned forwardListID = quadtree->geoElementIndex[geoElementOffset]->forwardListID;
        bool isValidForwardList = false;
        const ForwardList *fl = quadtree->getForwardIndex()->getForwardList(forwardListID, isValidForwardList);
        if (!isValidForwardList)
            continue;

        // get all the keywords in the record (on the forward list) corresponding to the prefix on the O-filter
        // e.g. prefix: "can"   keyword: "candy", "canada", etc.
        vector<unsigned> matchingKeywordIds;
        fl->getWordsInRange(quadtree->getForwardIndex()->getSchema(), prefixToSplit.minId, prefixToSplit.maxId, -1, matchingKeywordIds);
        
        // add each keyword to the delta oFilter along with the geoElement
        for (unsigned j = 0; j < matchingKeywordIds.size(); j++)
        {
        	// if the keyword is from the new geoelement it's not checked here
            if (geoElementOffset == newGeoElementOffset
                    && insertedKeywords.find(matchingKeywordIds[j]) == insertedKeywords.end() ) // don't count a keyword that hasn't been inserted yet in
                continue;

            Prefix keywordPrefix;
            trie->getPrefixFromKeywordId(matchingKeywordIds[j], keywordPrefix);

            deltaOFilterMap->updateOFilter(keywordPrefix, geoElementOffset, true);
        }
    }

    //add the <keyword prefix, new geo element> pair to the deltaOFilter
    deltaOFilterMap->updateOFilter(keywordPrefixFromNewGeoElem, newGeoElementOffset, true);

    // Add this prefix to c-filter using the geoElements corresponding to the prefix on the o-filter to be deleted shortly
    // Then, for each geoElement on the o-filter to be deleted shortly,
    // go to its corresponding child quad node to add it to the prefixToSplit o-filter (if it's not on prefixToSplit's ancestor o-filter there already)
    // this operation can add some prefixes to the oFilter of children. Note that if prefixToPlit is not in their oFilter it means it's been
    // in the skip list, so it can't be in cFilter neither so we don't have to check cFilter.
    recoverCFiltersOnThisNodeAndOFiltersOnChildrenNode(quadtree, oMapPtr, cMapPtr, geoElementList, prefixToSplit);

    GeoElementList copyOfOriginalElementList = *geoElementList;
    // delete this prefix from the o-filter
    oMapPtr->omp->erase(prefixToSplit);
    
    deltaOFilterMap->gatherFreqOfAllAncestorPrefixesForOFilters(trie, &prefixToSplit); // longer than prefix

    deltaOFilterMap->omp->erase(prefixToSplit);


    // Jamshid : deltaOFilterMap is the delta part of the new filter. Now we need to prune it.
    // This part is the same as construction except that we don't have skipList from parent and skipList to children.
    // We iterate on deltaOFilter, first we check if we should delete a prefix because some shorter prefix exists in one of the ancestors
    // , then if not, we search for a shorter prefix in delta to see if any ancestor prefix is already iterated from deltaOFilter.
    map< Prefix, GeoElementList >::iterator deltaOFilterMapIter = deltaOFilterMap->omp->begin();
    while( deltaOFilterMapIter != deltaOFilterMap->omp->end() )
    {
        bool coveredBySkipList = false;

        for (unsigned i = 0; i < skipList.size(); i++)
        {
            if (skipList[i]->getGeoElementList(deltaOFilterMapIter->first) != NULL)
            {
                coveredBySkipList = true;
                break;
            }
        }

        if (
              ( (prefixFromParent.minId != Trie::MAX_KEYWORD_ID || prefixFromParent.maxId != Trie::MAX_KEYWORD_ID)
                && prefixFromParent.isStrictAncestor(deltaOFilterMapIter->first)
                && (deltaOFilterMapIter->first.isAncestor(keywordPrefixFromNewGeoElem) || keywordPrefixFromNewGeoElem.isAncestor(deltaOFilterMapIter->first)) ) // ensure it's on the same path
              || coveredBySkipList
           )
        { // remove this one and its descendants

            // do binary search for the last descendant of the prefix
            // will return the position of the "smallest" one that is "bigger" than it
            Prefix lastDescendant;
            lastDescendant.minId = deltaOFilterMapIter->first.maxId;
            lastDescendant.maxId = deltaOFilterMapIter->first.maxId;
            map< Prefix, GeoElementList >::iterator lastDescendantPos = deltaOFilterMap->omp->upper_bound(lastDescendant);

            // the position we will begin from in the next loop
            map< Prefix, GeoElementList >::iterator nextPos = lastDescendantPos;

            // the actual position of the last descendant
            lastDescendantPos--;

            map< Prefix, GeoElementList >::iterator iter = deltaOFilterMapIter;
            if(iter != lastDescendantPos)//if has any descendants, remove them
            {
                iter++;
                deltaOFilterMap->omp->erase(iter, lastDescendantPos); // will be no-op if iter == lastDescendantPos
                deltaOFilterMap->omp->erase(lastDescendantPos); // the erase above doesn't erase the last one
            }

            deltaOFilterMap->omp->erase(deltaOFilterMapIter); // remove itself, we don't need it to be in the deltaOFilter

            deltaOFilterMapIter = nextPos;

            continue; // TODO optmize?
        }


        // Jamshid : adds the rest of delta to the actual oFilter (same as construction).
        if((deltaOFilterMapIter->second.geoElementOffsets.size() + deltaOFilterMapIter->second.freq) <= OBJECT_SELECTIVITY_THRESHOLD)
        {
            if (deltaOFilterMapIter->first.isAncestor(keywordPrefixFromNewGeoElem))
                matchingPrefixOnOFilter = deltaOFilterMapIter->first;

            Logger::debug("split to current Quad Node with prefix %d %d" , deltaOFilterMapIter->first.minId ,deltaOFilterMapIter->first.maxId );

            ASSERT(cMapPtr->cmp->find(deltaOFilterMapIter->first) == cMapPtr->cmp->end());

            oMapPtr->updateOFilter(deltaOFilterMapIter->first, deltaOFilterMapIter->second, false);

            // do binary search for the last descendant of the prefix
            // will return the position of the "smallest" one that "bigger" than it
            Prefix lastDescendant;
            lastDescendant.minId = deltaOFilterMapIter->first.maxId;
            lastDescendant.maxId = deltaOFilterMapIter->first.maxId;
            map< Prefix, GeoElementList >::iterator lastDescendantPos = deltaOFilterMap->omp->upper_bound(lastDescendant);

            // the position we will begin from in the next loop
            map< Prefix, GeoElementList >::iterator nextPos = lastDescendantPos;

            // the actual position of the last descendant
            lastDescendantPos--;

            // union current prefix's descendants geoElement list and remove those descendants, from oFilter
            map< Prefix, GeoElementList >::iterator iter = deltaOFilterMapIter;
            iter++;
            while(iter != nextPos)
            {
                // The reason we don't count freq here is that if this prefixs' desendant's freq > 0,
                // then this prefix must have already exceeded the threashold and we shouldn't even be here.
                if(iter->second.geoElementOffsets.size() > 0)
                {
                    (*oMapPtr->omp)[deltaOFilterMapIter->first].add(iter->second);
                }
                iter++;
            }
            iter = deltaOFilterMapIter;
            if(iter != lastDescendantPos)//if has any descendants, remove them
            {
                iter++;
                deltaOFilterMap->omp->erase(iter, lastDescendantPos); // will be no-op if iter == lastDescendantPos
                deltaOFilterMap->omp->erase(lastDescendantPos); // the erase above doesn't erase the last one
            }

            deltaOFilterMap->omp->erase(deltaOFilterMapIter); // remove itself, we don't need it to be in the deltaOFilter

            deltaOFilterMapIter = nextPos;

            continue;
        }

        // only the prefix on the same path with the inserted prefix can be here
        // need to recover the c-filters
        // TODO ASSERT(deltaOFilterMapIter->first.isAncestor(keywordPrefixFromNewGeoElem));
        recoverCFiltersOnThisNode(quadtree, cMapPtr, copyOfOriginalElementList, deltaOFilterMapIter->first, newGeoElementOffset, keywordPrefixFromNewGeoElem);

        deltaOFilterMap->omp->erase(deltaOFilterMapIter++);
    }

    ASSERT(deltaOFilterMap->omp->size() == 0);

    delete deltaOFilterMap;

    return;
}

void QuadNode::recoverCFiltersOnThisNode(QuadTree *quadtree, CFilterMapPtr cMapPtr, const GeoElementList &geoElementList, const Prefix &prefixToRecover,
                                            unsigned newGeoElementOffset, const Prefix &keywordPrefixFromNewGeoElem)
{
    ASSERT( cMapPtr->cmp->find(prefixToRecover) == cMapPtr->cmp->end() );

    (*cMapPtr->cmp)[prefixToRecover].reset();

    for (unsigned i = 0; i < geoElementList.geoElementOffsets.size(); i++)
    {
        // get the forward list
        unsigned geoElementOffset = geoElementList.geoElementOffsets[i];
        unsigned forwardListID = quadtree->geoElementIndex[geoElementOffset]->forwardListID;
        bool isValidForwardList = false;
        const ForwardList *fl = quadtree->getForwardIndex()->getForwardList(forwardListID, isValidForwardList);
        if (!isValidForwardList)
            continue;

        // get all the keywords in the record (on the forward list) corresponding to the prefix on the O-filter
        // e.g. prefix: "can"   keyword: "candy", "canada", etc.
        vector<unsigned> matchingKeywordIds;
        fl->getWordsInRange(quadtree->getForwardIndex()->getSchema(), prefixToRecover.minId, prefixToRecover.maxId, -1, matchingKeywordIds);
        
        // if any, add this record to the c-filter of prefixToRecover
        if (matchingKeywordIds.size() > 0)
        {
            unsigned child = this->findChildContainingPoint(quadtree->geoElementIndex[geoElementOffset]->point);
            cMapPtr->updateCFilter(prefixToRecover, child);
            Logger::debug("   further recover c-filter for %d %d at child %d for record %d", prefixToRecover.minId, prefixToRecover.maxId, child, geoElementOffset);
        }
    }
    
    if (prefixToRecover.isAncestor(keywordPrefixFromNewGeoElem))
    {
        unsigned child = this->findChildContainingPoint(quadtree->geoElementIndex[newGeoElementOffset]->point);
        cMapPtr->updateCFilter(prefixToRecover, child);
        Logger::debug("   further recover c-filter for %d %d at child %d for record %d", prefixToRecover.minId, prefixToRecover.maxId, child, newGeoElementOffset);
    }

}

void QuadNode::recoverCFiltersOnThisNodeAndOFiltersOnChildrenNode(QuadTree *quadtree, OFilterMapPtr oMapPtr, CFilterMapPtr cMapPtr, GeoElementList *geoElementList, const Prefix &prefixToSplit)
{
    // Add this prefix to c-filter using the geoElements corresponding to the prefix on the o-filter to be deleted shortly

    ASSERT( cMapPtr->cmp->find(prefixToSplit) == cMapPtr->cmp->end() );

    ChildBitSet *tempBitVector = new ChildBitSet ();
    map<unsigned, unsigned> *tempOffsetChildBitMap = new map<unsigned, unsigned> ();

    for (unsigned i = 0; i < geoElementList->geoElementOffsets.size(); i++)
    {
        unsigned offset = geoElementList->geoElementOffsets[i];
        unsigned child = this->findChildContainingPoint(quadtree->geoElementIndex[offset]->point);
        tempBitVector->set(child);
        (*tempOffsetChildBitMap)[offset] = child;
    }
    if (tempBitVector->count() <= CHILD_SELECTIVITY_THRESHOLD)
    {
        (*cMapPtr->cmp)[prefixToSplit] = *tempBitVector;
        Logger::debug("recover c-filter for %d %d", prefixToSplit.minId, prefixToSplit.maxId);
    }

    // Then, for each geoElement on the o-filter to be deleted shortly,
    // go to its corresponding child quad node to add it to the prefixToSplit o-filter (if it's not on prefixToSplit's ancestor o-filter there already)
    for(map<unsigned, unsigned>::const_iterator cIter = tempOffsetChildBitMap->begin();
                                                cIter != tempOffsetChildBitMap->end();
                                                cIter++ )
    {
        // if no ancestor o-filter on that child quad node
        // other wise will set this bit to 0
        if (tempBitVector->test(cIter->second))
        {
            QuadNode *childNode = this->entries[cIter->second];
            if (childNode->isLeaf)
                continue;
            OFilterMapPtr childNodeOMapPtr = quadtree->getOFilterMapPtr(childNode);
            map<Prefix , GeoElementList >::iterator oFilterPosition = childNodeOMapPtr->omp->upper_bound(prefixToSplit);
            if(oFilterPosition != childNodeOMapPtr->omp->begin())
            {
                oFilterPosition--;
                if (oFilterPosition->first == prefixToSplit) // if the prefixToSplit is already added to the o-filter
                {                                            // add the geoElement offset to it
                    oFilterPosition->second.add(cIter->first);
                    continue;
                }
                else if ( oFilterPosition->first.isStrictAncestor(prefixToSplit) ) // if find the ancestor o-filter
                {                                                           // set this bit to 0, so that we won't come later
                    tempBitVector->reset(cIter->second);
                    continue;
                }
                //else
            }
            // if neither prefixToSplit and its ancestor is on o-filter
            // add prefixToSplit with this geoElement offset to o-filter
            childNodeOMapPtr->updateOFilter(prefixToSplit, cIter->first, false);
    
            ASSERT( quadtree->getCFilterMapPtr(childNode)->cmp->find(prefixToSplit) == quadtree->getCFilterMapPtr(childNode)->cmp->end() );
        }
    }

    delete tempBitVector;
    delete tempOffsetChildBitMap;
}

// CHEN: In the current quadnode, use the collected "keyword and interval-break" info to fix
// the intervals in the C/O-filters
void QuadNode::fixFiltersBroadenOnOneNode(const QuadTree *quadtree, vector<Prefix> *oldParentOrSelfAndAncs,
		const pair<unsigned, unsigned> &problemKeywordIdPair, bool hadExactlyOneChild)
{
    if (this->isLeaf)
        return;

    OFilterMapPtr oMapPtr = quadtree->getOFilterMapPtr(this);
    CFilterMapPtr cMapPtr = quadtree->getCFilterMapPtr(this);

    // TODO this is the "narrowest" prefix in the vector,
    // search for this one on o-filter can either find it or it's ancestors
    Prefix oldParentPrefixOrSelf = oldParentOrSelfAndAncs->back();

    // fix o-filter

    // search to see if oldParentPrefixOrSelf or any of its ancestors is on o-filter
    // CHEN: suppose oldParentPrefixOrSelf (original interval) is [4, 7], which should be
    // expanded to [2, 7] due to the new keyword.  If [4,7] appears in the O-filter,
    // we need to change [4 7] in the O-fiter to [2, 7].
    Prefix matchingPrefixOnOFilter;
    GeoElementList *geoElementList = oMapPtr->searchOFilter(oldParentPrefixOrSelf, matchingPrefixOnOFilter);
    if (geoElementList != NULL)
    {
    	// Jamshid : Since we don't know what's found in o filter and matchingPrefixOnOFilter can be
    	// an ancestor of problemKeywordIdPair we should check if this interval
    	// needs to be updated.
        if (QuadTree::needToFix(problemKeywordIdPair, matchingPrefixOnOFilter))
        { // this prefix on o-filter needs to be broaden
            Prefix newPrefix = matchingPrefixOnOFilter;
            newPrefix.broaden(problemKeywordIdPair.first, problemKeywordIdPair.second, Trie::MAX_KEYWORD_ID);
            
            oMapPtr->changePrefix(matchingPrefixOnOFilter, newPrefix);
        }
    }

    // fix c-filters

    ChildBitSet childrenBitVector;
    childrenBitVector.reset();
    // search to see if oldParentPrefixOrSelf or any of its ancestors is on c-filter
    unsigned end = 0;
    if (hadExactlyOneChild)
    { // in this case, we add, NOT replace
// CHEN:   	[1, 7]             [1, 8]
//       	   |      =>      |      |
//    	    [1, 7]          [1, 7]    New node
    	// CHEN: we need to add a new prefix [1, 8] to the C-filter
// CHEN: STOPPED HERE ON 1/15/2013
        end = oldParentOrSelfAndAncs->size() - 1; // Jamshid: so that we don't include the last one in the change loop

        ASSERT( QuadTree::needToFix(problemKeywordIdPair, oldParentPrefixOrSelf));
        if (cMapPtr->cmp->find(oldParentPrefixOrSelf) != cMapPtr->cmp->end())
        {
            Prefix newPrefix = oldParentPrefixOrSelf;
            newPrefix.broaden(problemKeywordIdPair.first, problemKeywordIdPair.second, Trie::MAX_KEYWORD_ID);
            // Jamshid : Adding a new prefix to the C-filter.
            (*cMapPtr->cmp)[newPrefix].reset();
            (*cMapPtr->cmp)[newPrefix] = cMapPtr->cmp->at(oldParentPrefixOrSelf);
        
            // TODO check if this is really needed, should be covered by ancesters below
            childrenBitVector |= cMapPtr->cmp->at(oldParentPrefixOrSelf);
        }
    }
    else
        end = oldParentOrSelfAndAncs->size(); // Jamshid: so that all nodes are included in the change loop

    // Jamshid : change loop
    for (unsigned i = 0; i < end; i++)
    {
        if (!QuadTree::needToFix(problemKeywordIdPair, oldParentOrSelfAndAncs->at(i)))
            continue; // if this ancestor prefix don't need to fix, go to the next
        if (cMapPtr->cmp->find(oldParentOrSelfAndAncs->at(i)) == cMapPtr->cmp->end()) //  Jamshid : if this ancestor prefix doesn't exit on c-filter
            break;                                                             /// the ones after it won't either (because we move from top to bottom)
        
        Prefix newPrefix = oldParentOrSelfAndAncs->at(i);
        newPrefix.broaden(problemKeywordIdPair.first, problemKeywordIdPair.second, Trie::MAX_KEYWORD_ID);
        // Jamshid : adding a new one and removing the old one
        (*cMapPtr->cmp)[newPrefix].reset();
        (*cMapPtr->cmp)[newPrefix] = cMapPtr->cmp->at(oldParentOrSelfAndAncs->at(i));

        childrenBitVector |= cMapPtr->cmp->at(oldParentOrSelfAndAncs->at(i));

        cMapPtr->cmp->erase(oldParentOrSelfAndAncs->at(i));
        
    }

    // go to children quad nodes, call this function recursively
    if(!childrenBitVector.none())
    {
        for(unsigned counter = 0; counter < CHILD_NUM; counter++)
        {
            if(this->entries[counter] != NULL && childrenBitVector.test(counter))
                this->entries[counter]->fixFiltersBroadenOnOneNode(quadtree, oldParentOrSelfAndAncs, problemKeywordIdPair, hadExactlyOneChild);
        }
    }
}
/*
 * Jamshid : Recursively goes through the quad tree to find all the forwardlists containing keywords to-be-changed. and
 * also updating O/C filters.
 *
 *
 */
void QuadNode::gatherForwardListsAndAdjustOCFilters(QuadTree *quadtree, unsigned oldKeywordId, unsigned newKeywordId, map<unsigned, unsigned> &recordIdsToProcess) const
{
    if (this->isLeaf)
    {
    	// if it is a leaf we just check the geo elements to see if they should be updated or not
        for (unsigned elementCounter = 0; elementCounter < this->numElements; elementCounter++)
        {
            intptr_t geoElementOffset = (intptr_t)this->entries[elementCounter];
            unsigned forwardListID = quadtree->geoElementIndex[geoElementOffset]->forwardListID;
            bool isValidForwardList = false;
            const ForwardList *fl = quadtree->getForwardIndex()->getForwardList(forwardListID, isValidForwardList);
            if (!isValidForwardList)
                continue;

            // check if this geoElement has keywords to be reassign
            int termSearchableAttributeIdToFilterTermHits = -1;
            float score;
            unsigned keywordId;
            unsigned attributeBitmap;
            if (fl->haveWordInRange(quadtree->getForwardIndex()->getSchema(), oldKeywordId, oldKeywordId, termSearchableAttributeIdToFilterTermHits, keywordId, attributeBitmap, score))
            {
                if (recordIdsToProcess.find (forwardListID) == recordIdsToProcess.end())
                {
                    recordIdsToProcess[forwardListID] = 0; // add it to the set
                }
            }
        }

        return;
    }

    Logger::debug("adjusting filters for %d-%d reassignment", oldKeywordId, newKeywordId);

    // if not leaf we should update filters
    OFilterMapPtr oMapPtr = quadtree->getOFilterMapPtr(this);
    CFilterMapPtr cMapPtr = quadtree->getCFilterMapPtr(this);

    Prefix oldKeywordPrefix(oldKeywordId, oldKeywordId);
    Prefix newKeywordPrefix(newKeywordId, newKeywordId);

    // fix o-filter

    // search to see if oldParentPrefixOrSelf or any of its ancestors is on o-filter
    // just changing the left or right boundary of this prefix (and its ancestors) to
    // the new keywordId is enough.
    Prefix matchingPrefixOnOFilter;
    GeoElementList *geoElementList = oMapPtr->searchOFilter(oldKeywordPrefix, matchingPrefixOnOFilter);
    if (geoElementList != NULL)
    {
        // collect the forwordlists candidates from O filters too
        // TODO test if keyword is really on forwordlists candidtates now?
        for (unsigned i = 0; i < geoElementList->geoElementOffsets.size(); i++)
        {
            unsigned recordId = quadtree->geoElementIndex[geoElementList->geoElementOffsets[i]]->forwardListID;
            if (recordIdsToProcess.find (recordId) == recordIdsToProcess.end())
            {
                recordIdsToProcess[recordId] = 0; // add it to the set
            }
        }

        if (matchingPrefixOnOFilter.minId == oldKeywordId || matchingPrefixOnOFilter.maxId == oldKeywordId)
        { // this prefix on o-filter needs to be fixed 
            Prefix newPrefix = matchingPrefixOnOFilter;
            newPrefix.replace(oldKeywordId, newKeywordId);

            oMapPtr->changePrefix(matchingPrefixOnOFilter, newPrefix);
            Logger::debug("---change on o-filter %d %d to %d %d", matchingPrefixOnOFilter.minId, matchingPrefixOnOFilter.maxId, newPrefix.minId, newPrefix.maxId);
        }
    }

    // fix c-filters
    //TODO optimize? : Since C-Filters are not pruned and can contain two prefixes A and B that A is a prefix of B (example : search for
    // 2,2 on "1,2|1,1|2,2"), we can't have such an efficient search method on Cfilter lie Ofilter.
    // So we have to iterate on all of them. Maybe there is a better way though.
    ChildBitSet childrenBitVector;
    childrenBitVector.reset();
    vector<Prefix> oldAncestorsPrefixes;

    for (map< Prefix, ChildBitSet >::const_iterator pos = cMapPtr->cmp->begin();
                pos != cMapPtr->cmp->end(); pos++)
    {
        if(pos->first.minId == oldKeywordId || pos->first.maxId == oldKeywordId)
            oldAncestorsPrefixes.insert(oldAncestorsPrefixes.begin(), pos->first);
    }

    for (unsigned i = 0; i < oldAncestorsPrefixes.size(); i++)
    {
        
        Prefix newPrefix = oldAncestorsPrefixes[i];
        newPrefix.replace(oldKeywordId, newKeywordId);
        (*cMapPtr->cmp)[newPrefix].reset();
        (*cMapPtr->cmp)[newPrefix] = cMapPtr->cmp->at(oldAncestorsPrefixes[i]);

        childrenBitVector |= cMapPtr->cmp->at(oldAncestorsPrefixes[i]);

        cMapPtr->cmp->erase(oldAncestorsPrefixes[i]);
        Logger::debug(" ---change on c-filter %d %d to %d %d", oldAncestorsPrefixes[i].minId, oldAncestorsPrefixes[i].maxId, newPrefix.minId ,newPrefix.maxId );
        
    }

    // go to children quad nodes, call this function recursively
    if(!childrenBitVector.none())
    {
        for(unsigned counter = 0; counter < CHILD_NUM; counter++)
        {
            if(this->entries[counter] != NULL && childrenBitVector.test(counter))
                this->entries[counter]->gatherForwardListsAndAdjustOCFilters(quadtree, oldKeywordId, newKeywordId, recordIdsToProcess);
        }
    }
}


}}
