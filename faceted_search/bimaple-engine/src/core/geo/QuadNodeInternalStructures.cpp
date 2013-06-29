//$Id: QuadNodeInternalStructures.cpp 3456 2013-06-14 02:11:13Z jiaying $

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

#include "QuadNodeInternalStructures.h"

using namespace std;

namespace srch2
{
namespace instantsearch
{
/* Jamshid :
 * @param prefix is null in construction phase. it's the prefix to split in update phase.
 *
 */
void OFilterMap::gatherFreqOfAllAncestorPrefixesForOFilters(const Trie *trie, const Prefix *prefix)
{
    map< Prefix, GeoElementList >::iterator oFilterMapIterator;
    for (oFilterMapIterator = omp->begin(); oFilterMapIterator != omp->end(); oFilterMapIterator++)
    {   
        vector<Prefix> ancestorPrefixes;
        // for each prefix in the o-filter, get ancestor prefixes from the trie
        trie->getAncestorPrefixes(oFilterMapIterator->first, ancestorPrefixes);

        unsigned i = 0;

        // Jamshid :
        // This part is for update only, when this function is used in construction phase, the prefix argument will be NULL by default
        // The reason we ignore them in update phase is this : It's called from split in QuadNode and prefix is the prefix to split.
        // this prefix is already too popular (so we want to split it) and there is no point in getting even shorter prefixes.
        if (prefix != NULL)
        { // ignore ancestor prefixes that are shorter than the prefix
            while (i < ancestorPrefixes.size() && ancestorPrefixes[i].isStrictAncestor(*prefix))
                i++;
        }

        for(; i < ancestorPrefixes.size(); i++)
        { // Jamshid :
          // for each ancestor prefix, we search it in the o-filter
          // if it's not in the o-filter, we add it into the o-filter, and the geoElement list will be updated later
          // if it's already in the o-filter, we increase the size, the geoElement list will also be updated later

        	//TODO change the way of using freq
            map< Prefix, GeoElementList >::iterator pos = omp->find(ancestorPrefixes[i]);
            if (pos == omp->end())
            {
                (*omp)[ancestorPrefixes[i]].freq = oFilterMapIterator->second.geoElementOffsets.size() + oFilterMapIterator->second.freq;
            }
            else
            {
                pos->second.freq += oFilterMapIterator->second.geoElementOffsets.size() + oFilterMapIterator->second.freq;
            }
        }
    }
}

void OFilterMap::removePopularAndUselessDescendantsOFilters(vector<Prefix> &skipListToChildren, const vector<Prefix> &skipListFromParent)
{
    map< Prefix, GeoElementList >::iterator oFilterMapIterator;
    vector<Prefix>::const_iterator parentSkipListIterator;
    oFilterMapIterator = omp->begin();
    parentSkipListIterator = skipListFromParent.begin();
    while(oFilterMapIterator != omp->end())
    {
        if((oFilterMapIterator->second.geoElementOffsets.size() + oFilterMapIterator->second.freq) > OBJECT_SELECTIVITY_THRESHOLD)
        { // if the prefix is too popular, remove it from o-filter
            omp->erase(oFilterMapIterator++);
            continue;
        }
        else
        {
            //cout << "----> create o-filter for " << oFilterMapIterator->first.minId << " " << oFilterMapIterator->first.maxId << endl;
            // do binary search for the prefix on the parent skip list
            // if found, will return the correct position, otherwise will return the position of the "smallest" one that is "bigger" than it
            vector<Prefix>::const_iterator posInSkipList = lower_bound(parentSkipListIterator, skipListFromParent.end(), oFilterMapIterator->first);

            // do binary search for the last descendant of the prefix
            // will return the position of the "smallest" one that is "bigger" than it
            Prefix lastDescendant;
            lastDescendant.minId = oFilterMapIterator->first.maxId;
            lastDescendant.maxId = oFilterMapIterator->first.maxId;
            map< Prefix, GeoElementList >::iterator lastDescendantPos = omp->upper_bound(lastDescendant);

            // the position we will begin from in the next loop
            map< Prefix, GeoElementList >::iterator nextPos = lastDescendantPos;

            // the actual position of the last descendant
            lastDescendantPos--;

            // Jamshid :
            // At this point all the prefixes from parentSkipListIterator (insitialized to be begin) to posInSkipList in the skip list are
            // parents of all the prefixes from oFilterMapIterator to lastDescendantPos in the ofilter. the first mentioned range is passed
            // to skipListToChildren
            // add prefixes to the new skip list
            while(parentSkipListIterator != posInSkipList)
            {
                if(skipListToChildren.empty() || (!skipListToChildren[skipListToChildren.size()-1].isAncestor((*parentSkipListIterator))))
                    skipListToChildren.push_back((*parentSkipListIterator));
                parentSkipListIterator++;
            }

            if ( (!skipListToChildren.empty() && skipListToChildren[skipListToChildren.size()-1].isAncestor(oFilterMapIterator->first))
                    || (posInSkipList != skipListFromParent.end() && (*posInSkipList) == oFilterMapIterator->first))
            { // is on or covered by the parent's skip list

                //remove current prefix and all its descendant
                omp->erase(oFilterMapIterator, lastDescendantPos);
                omp->erase(lastDescendantPos); // the erase above doesn't erase the last one
            }
            else
            { // is NOT in parent's skip list
                
                skipListToChildren.push_back(oFilterMapIterator->first);

                // clear the freq counter, because we'll union current prefix's all descendants geoElement list
                oFilterMapIterator->second.freq = 0;

                // union current prefix's descendants geoElement list and remove those descendants, from oFilter
                map< Prefix, GeoElementList >::iterator iter = oFilterMapIterator;
                iter++;
                while(iter != nextPos)
                {
                    // The reason we don't count freq here is that if this prefixs' desendant's freq > 0,
                    // then this prefix must have already exceeded the threashold and we shouldn't even be here.
                    if(iter->second.geoElementOffsets.size() > 0)
                    {
                        oFilterMapIterator->second.add(iter->second);
                    }
                    iter++;
                }
                iter = oFilterMapIterator;
                if(iter != lastDescendantPos)//if has any descendants, remove them
                {
                    iter++;
                    omp->erase(iter, lastDescendantPos); // will be no-op if iter == lastDescendantPos
                    omp->erase(lastDescendantPos); // the erase above doesn't erase the last one
                }
            }

            oFilterMapIterator = nextPos;
        }
    }

    // add the rest of the old skip list to the new skip list
    while(parentSkipListIterator != skipListFromParent.end())
    {
        if(skipListToChildren.empty() || (!skipListToChildren[skipListToChildren.size()-1].isAncestor((*parentSkipListIterator))))
            skipListToChildren.push_back((*parentSkipListIterator));
        parentSkipListIterator++;
    }
}

// Search the prefix on the o-filter and get the corresponding geo element list
GeoElementList* OFilterMap::getGeoElementList(const Prefix &prefix) const
{
    Prefix matchingPrefix;
    return searchOFilter(prefix, matchingPrefix);
}

// earch the prefix on the o-filter and get the corresponding geo element list and the matching prefix
GeoElementList* OFilterMap::searchOFilter(const Prefix &prefix, Prefix &matchingPrefix) const
{
    // search the expansion on o-filter
    // don't use lower_bound instead, it's not the same
    // e.g. we have (12, 48) (48, 64) on o-filter and we search for (12, 24)
    //      lower_bound will give (48, 64), which is wrong
    //      upper_bound and decrement will give (12, 48), the ancestor of (12, 24), which is what we want
    map<Prefix , GeoElementList >::iterator oFilterPosition = omp->upper_bound(prefix);

    if(oFilterPosition != omp->begin())
    {
         oFilterPosition--; // the important decrement, explained above
         if ( oFilterPosition->first.isAncestor(prefix) )
         {
             matchingPrefix = oFilterPosition->first;
             return &(oFilterPosition->second);
         }
    }

    return NULL;
}


void OFilterMap::getAllDescendantsOnOFilter(const Prefix &prefix, set<Prefix> &descendantPrefixes) const
{
    const Prefix lastDescendant = Prefix(prefix.maxId, prefix.maxId);
    map< Prefix, GeoElementList >::iterator lastDescendantPos = omp->upper_bound(lastDescendant); // will return the position of the "smallest" one that "bigger" than it
    if (lastDescendantPos != omp->begin())
    {
        lastDescendantPos--; // the actual position of the last descendant
        while(prefix.isAncestor(lastDescendantPos->first))
        {
            descendantPrefixes.insert(lastDescendantPos->first); 

            if (lastDescendantPos == omp->begin())
                break;

            lastDescendantPos--;
        }
    }
}

void CFilterMap::gatherFreqOfAllAncestorPrefixesForCFilters(const Trie *trie)
{
    map< Prefix, ChildBitSet >::iterator cFilterMapIterator;
    for (cFilterMapIterator = cmp->begin(); cFilterMapIterator != cmp->end(); ++cFilterMapIterator)
    {
        vector<Prefix> ancestorPrefixes;
        // for each prefix in the c-filter, get ancestor prefixes from the trie
        trie->getAncestorPrefixes(cFilterMapIterator->first, ancestorPrefixes);
        for(unsigned i = 0; i < ancestorPrefixes.size(); i++)
        {
            // for each ancestor prefix, we search it in the c-filter
            // if it's not in the c-filter, we add it into the c-filter, and set the bits
            // if it's already in the c-filter, we do an OR operation
            map< Prefix, ChildBitSet >::iterator pos = cmp->find(ancestorPrefixes[i]);
            if (pos == cmp->end())
                (*cmp)[ancestorPrefixes[i]] = cFilterMapIterator->second;
            else
                pos->second |= cFilterMapIterator->second;
        }
    }
}


void CFilterMap::removePopularAndUselessDescendantsCFilters(const vector<Prefix> &skipListToChildren)
{
    map< Prefix, ChildBitSet >::iterator cFilterMapIterator;
    vector<Prefix>::const_iterator skipListIterator;
    cFilterMapIterator = cmp->begin();
    skipListIterator = skipListToChildren.begin();
    while(cFilterMapIterator != cmp->end())
    {
    	// TODO currently we don't do this pruning
    	//      CHILD_SELECTIVITY_THRESHOLD is set to 17, considering the current max number of children QuadNode is 16,
    	//      this condition will never be meet
    	//      maybe we should just remove this part, since we don't want to depend on hard coding numbers
        if(cFilterMapIterator->second.count() > CHILD_SELECTIVITY_THRESHOLD)
        { // if the prefix is too popular, remove it from c-filter
            cmp->erase(cFilterMapIterator++);
            continue;
        }
        else
        {
            // do binary search for the prefix on the skip list
            // will return the position of the "smallest" one that is "bigger" than it
            vector<Prefix>::const_iterator posInSkipList = upper_bound(skipListIterator, skipListToChildren.end(), cFilterMapIterator->first);
            if(posInSkipList != skipListToChildren.begin())
            {// if its ancestor or itself is possibly on the skip list
                posInSkipList--; // go to the possible position 
                if(posInSkipList->isAncestor(cFilterMapIterator->first)) // check if it really is
                {
                    cmp->erase(cFilterMapIterator++);
                    continue;
                }
            }
            cFilterMapIterator++;
        }
    }
}

//Get the childrenBitVector directly from a position in the keyword-children map
ChildBitSet* CFilterMap::getChildrenBitVector(const Prefix &prefix) const
{
    // the search here is different from the search on o-filter
    // because on c-filter we don't do ancestor prefix pruning
    // e.g. we can have "foo" and "football" on c-filter at the same time
    std::map<Prefix , ChildBitSet >::iterator vectorIterator = cmp->lower_bound(prefix);

    if(vectorIterator != cmp->end() && vectorIterator->first.isAncestor(prefix))
    {
        return &(vectorIterator->second);
    }
    else
        return NULL;
}


// Search the prefix on the c-filter, update the bit vector if we found
// If we the prefix is not on c-filter, create it and set the bit
void CFilterMap::updateCFilter(const Prefix &prefix, unsigned child)
{
    if (cmp->find(prefix) == cmp->end())
        (*cmp)[prefix].reset();

    (*cmp)[prefix].set(child);

    return;
}

}}
