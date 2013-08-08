
// $Id: InvertedIndex.cpp 3480 2013-06-19 08:00:34Z jiaying $

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

#include "index/InvertedIndex.h"
#include "util/Assert.h"
#include "util/Logger.h"
#include <math.h>

#include <algorithm>
#include <vector>
#include <iostream>

#include <cassert>

using std::endl;
using std::vector;
using srch2::util::Logger;

namespace srch2
{
namespace instantsearch
{

void InvertedListContainer::sortAndMergeBeforeCommit(const unsigned keywordId, const ForwardIndex *forwardIndex, bool needToSortEachInvertedList)
{
	// sort this inverted list only if the flag is true.
	// if the flag is false, we only need to commit.
	if (needToSortEachInvertedList) {
		ts_shared_ptr<vectorview<unsigned> > writeView;
		this->invList->getWriteView(writeView);

		vector<InvertedListElement> elem(writeView->size());
		for(unsigned i = 0; i< writeView->size(); i++)
		{
			elem[i].recordId = writeView->getElement(i);
			elem[i].positionIndexOffset = forwardIndex->getKeywordOffset(elem[i].recordId, keywordId);
		}
		std::sort(elem.begin(), elem.end(), InvertedListContainer::InvertedListElementGreaterThan(forwardIndex));

		for(unsigned i = 0; i< writeView->size(); i++)
		{
			writeView->at(i) = elem[i].recordId;
		}
	}

	this->invList->commit();
}

void InvertedListContainer::sortAndMerge(const unsigned keywordId, const ForwardIndex *forwardIndex)
{
	ts_shared_ptr<vectorview<unsigned> > readView;
	this->invList->getReadView(readView);
	unsigned readViewListSize = readView->size();

	ts_shared_ptr<vectorview<unsigned> > writeView;
	this->invList->getWriteView(writeView);

	unsigned writeViewListSize = writeView->size();
	vector<InvertedListElement> elem(writeView->size());

	for(unsigned i = 0; i< writeView->size(); i++)
	{
		elem[i].recordId = writeView->getElement(i);
		elem[i].positionIndexOffset = forwardIndex->getKeywordOffset(elem[i].recordId, keywordId);
	}

    Logger::debug("SortnMerge: | %d | %d ", readViewListSize, writeViewListSize);

	std::sort( elem.begin() + readViewListSize, elem.begin() + writeViewListSize, InvertedListContainer::InvertedListElementGreaterThan(forwardIndex) );

	std::inplace_merge (elem.begin(), elem.begin() + readViewListSize, elem.begin() + writeViewListSize, InvertedListContainer::InvertedListElementGreaterThan(forwardIndex));

	for(unsigned i = 0; i< writeView->size(); i++)
	{
		writeView->at(i) = elem[i].recordId;
	}
	this->invList->commit();
}


InvertedIndex::InvertedIndex(ForwardIndex *forwardIndex)
{
    //this->invertedIndexVector = new cowvector<InvertedListContainerPtr>(100);
    this->invertedIndexVector = NULL;
    this->keywordIds = NULL;
    this->forwardIndex = forwardIndex;
    this->commited_WriteView = false;
}

InvertedIndex::~InvertedIndex()
{
    if (this->invertedIndexVector != NULL) {
        this->invertedIndexVector->commit();
        this->keywordIds->commit();
        for (unsigned invertedIndexIter = 0; invertedIndexIter < this->getTotalNumberOfInvertedLists_ReadView(); ++invertedIndexIter)
        {
            ts_shared_ptr<vectorview<InvertedListContainerPtr> > writeView;
            this->invertedIndexVector->getWriteView(writeView);
            delete writeView->getElement(invertedIndexIter);
        }
        this->invertedListSizeDirectory.clear();
        delete this->invertedIndexVector;
        delete this->keywordIds;
    }
}

bool InvertedIndex::isValidTermPositionHit(unsigned forwardListId, unsigned keywordOffset,
                       unsigned searchableAttributeId, unsigned& termAttributeBitmap, float &termRecordStaticScore) const
{
    return this->forwardIndex->isValidRecordTermHit(forwardListId, keywordOffset,
                            searchableAttributeId, termAttributeBitmap, termRecordStaticScore);
}

// given a forworListId and invertedList offset, return the keyword offset
unsigned InvertedIndex::getKeywordOffset(unsigned forwardListId, unsigned invertedListOffset) const
{
	ts_shared_ptr<vectorview<unsigned> > readView;
	this->keywordIds->getReadView(readView);
	//transfer the invertedList offset to keywordId
	return this->forwardIndex->getKeywordOffset(forwardListId, readView->getElement(invertedListOffset));
}

// For Trie bootstrap
void InvertedIndex::incrementDummyHitCount(unsigned invertedIndexDirectoryIndex)
{
    if ( this->commited_WriteView == false)
    {
        if ( invertedIndexDirectoryIndex == this->invertedListSizeDirectory.size() )
        {
            this->invertedListSizeDirectory.push_back(0);
        }
        else  if (invertedIndexDirectoryIndex < this->invertedListSizeDirectory.size())
        {
            // do nothing
        }
    }
}
void InvertedIndex::incrementHitCount(unsigned invertedIndexDirectoryIndex)
{
    if ( this->commited_WriteView == false)
    {
        if ( invertedIndexDirectoryIndex == this->invertedListSizeDirectory.size() )
        {
            this->invertedListSizeDirectory.push_back(1);
        }
        else  if (invertedIndexDirectoryIndex < this->invertedListSizeDirectory.size())
        {
            this->invertedListSizeDirectory.at(invertedIndexDirectoryIndex) += 1;
        }
    }
    else
    {
        ts_shared_ptr<vectorview<InvertedListContainerPtr> > writeView;
        this->invertedIndexVector->getWriteView(writeView);
        if (invertedIndexDirectoryIndex == writeView->size())
        {
            writeView->push_back(new InvertedListContainer(1));
        }
    }
}

float InvertedIndex::getIdf(const unsigned totalNumberOfDocuments, const unsigned keywordId) const
{
    float idf = 0.0;
    if ( this->commited_WriteView == false)
    {
        ASSERT(keywordId < this->invertedListSizeDirectory.size());
        idf = log (totalNumberOfDocuments / ((float)(this->invertedListSizeDirectory.at(keywordId) )));
    }
    else
    {
        ts_shared_ptr<vectorview<InvertedListContainerPtr> > writeView;
        this->invertedIndexVector->getWriteView(writeView);
        
        ASSERT(keywordId < writeView->size());
        
        idf = log (totalNumberOfDocuments / ((float)(writeView->getElement(keywordId)->getWriteViewSize())) );
    }
    return idf;
}
    
float InvertedIndex::computeRecordStaticScore(RankerExpression *rankerExpression, const float recordBoost, 
                          const float recordLength, const float tf, const float idf, 
                          const float sumOfFieldBoosts) const
{
    // recordScoreType == srch2::instantsearch::LUCENESCORE:
    float textRelevance =  Ranker::computeRecordTfIdfScore(tf, idf, sumOfFieldBoosts);
    return rankerExpression->applyExpression(recordLength, recordBoost, textRelevance);
}

void InvertedIndex::initialiseInvertedIndexCommit()
{
    ASSERT(this->commited_WriteView != true); // TODO: can be removed for optimization.

    //delete this->invertedIndexVector;
    this->invertedIndexVector = new cowvector<InvertedListContainerPtr>(this->invertedListSizeDirectory.size());
    this->keywordIds = new cowvector<unsigned> (this->invertedListSizeDirectory.size());
    ts_shared_ptr<vectorview<InvertedListContainerPtr> > writeView;
    this->invertedIndexVector->getWriteView(writeView);
    for (vector<unsigned>::iterator vectorIterator = this->invertedListSizeDirectory.begin();
            vectorIterator != this->invertedListSizeDirectory.end();
            vectorIterator++)
    {
        InvertedListContainerPtr invListContainerPtr;
        invListContainerPtr = new InvertedListContainer(*vectorIterator);
        writeView->push_back(invListContainerPtr);
    }
    this->invertedIndexVector->commit();
}
/* COMMIT LOGIC
 * 1. Get all the records from the forwardIndex
 * 2. For each record
 *         2.1 Get the forwardList corresponding to the record
 *         2.2 Get the recordBoost from the forwardList
 *         2.2 For each keyword in the forwardList
 *             2.2.1 Calculate the score
 *          2.2.2 Add the keyword,rid to the InvertedIndex
 *          2.2.3 Add the score in the forwardIndex
 */

void InvertedIndex::commit( ForwardList *forwardList,
        RankerExpression *rankerExpression,
        const unsigned forwardListOffset, const unsigned totalNumberOfDocuments,
        const Schema *schema, const vector<NewKeywordIdKeywordOffsetTriple> &newKeywordIdKeywordOffsetTriple)
{
    if (this->commited_WriteView == false)
    {
        //unsigned sumOfOccurancesOfAllKeywordsInRecord = 0;
        float recordBoost = forwardList->getRecordBoost();

        for (unsigned counter = 0; counter < forwardList->getNumberOfKeywords(); counter++)
        {
            //unsigned keywordId = forwardIndex->getForwardListElementByDirectory(forwardListOffset , counter);//->keywordId;
        	unsigned keywordId = newKeywordIdKeywordOffsetTriple.at(counter).first;
        	unsigned invertedListId = newKeywordIdKeywordOffsetTriple.at(counter).second.second;
            float idf = this->getIdf(totalNumberOfDocuments, invertedListId); // Uses invertedListSizeDirectory at commit stage

            ///Use a positionIndexDirectory to get the size of records for calculating tf
            //unsigned numberOfOccurancesOfGivenKeywordInRecord = forwardList->getNumberOfPositionHitsForAllKeywords(schema);
            //sumOfOccurancesOfAllKeywordsInRecord += numberOfOccurancesOfGivenKeywordInRecord;

            unsigned tf = 1; // TODO: get the right TF value from the positional index
            float sumOfFieldBoost = forwardList->getKeywordRecordStaticScore(counter);
            float recordLength = forwardList->getNumberOfKeywords();
            float score = this->computeRecordStaticScore(rankerExpression, recordBoost, recordLength, tf, idf, sumOfFieldBoost);

            //assign keywordId for the invertedListId
            ts_shared_ptr<vectorview<unsigned> > writeView;
            this->keywordIds->getWriteView(writeView);
            writeView->at(invertedListId) = keywordId;
            this->addInvertedListElement(invertedListId, forwardListOffset);
            forwardList->setKeywordRecordStaticScore(counter, score);
        }
    }
}

void InvertedIndex::finalCommit(bool needToSortEachInvertedList)
{
    ts_shared_ptr<vectorview<InvertedListContainerPtr> > writeView;
    this->invertedIndexVector->getWriteView(writeView);
    unsigned sizeOfList = writeView->size();
    ts_shared_ptr<vectorview<unsigned> > keywordIdsWriteView;
    this->keywordIds->getWriteView(keywordIdsWriteView);

    for (unsigned iter = 0; iter < sizeOfList; ++iter)
    {
    	writeView->at(iter)->sortAndMergeBeforeCommit(keywordIdsWriteView->getElement(iter), this->forwardIndex, needToSortEachInvertedList);
    }

    this->invertedIndexVector->commit();
    this->keywordIds->commit();
    this->commited_WriteView = true;
    this->invertedListSizeDirectory.clear();
}

void InvertedIndex::merge()
{
    this->invertedIndexVector->commit();
    this->keywordIds->commit();
    ts_shared_ptr<vectorview<InvertedListContainerPtr> > writeView;
    this->invertedIndexVector->getWriteView(writeView);
    // get keywordIds writeView
    ts_shared_ptr<vectorview<unsigned> > keywordIdsWriteView;
    this->keywordIds->getWriteView(keywordIdsWriteView);

    for ( set<unsigned>::const_iterator iter = this->invertedListSetToMerge.begin(); iter != this->invertedListSetToMerge.end(); ++iter)
    {
        writeView->at(*iter)->sortAndMerge(keywordIdsWriteView->getElement(*iter), this->forwardIndex);
    }
    this->invertedListSetToMerge.clear();
}

// recordInternalId is same as forwardIndeOffset
void InvertedIndex::addRecord(ForwardList* forwardList,
        RankerExpression *rankerExpression,
        const unsigned forwardListOffset, const SchemaInternal *schema,
        const Record *record, const unsigned totalNumberOfDocuments,
        const KeywordIdKeywordStringInvertedListIdTriple &keywordIdList)
{
    if (this->commited_WriteView == true)
    {
        //unsigned sumOfOccurancesOfAllKeywordsInRecord = 0;
        float recordBoost = record->getRecordBoost();


        for (unsigned counter = 0; counter < keywordIdList.size(); counter++)
        {
        	unsigned keywordId = keywordIdList.at(counter).first;
            unsigned invertedListId = keywordIdList.at(counter).second.second;

            ///Use a positionIndexDirectory to get the size of  for calculating tf
            //unsigned numberOfOccurancesOfGivenKeywordInRecord = forwardList->getNumberOfPositionHitsForAllKeywords(schema);
            //sumOfOccurancesOfAllKeywordsInRecord += numberOfOccurancesOfGivenKeywordInRecord;

            unsigned tf = 1; ///TODO
            float sumOfFieldBoost = forwardList->getKeywordRecordStaticScore(counter);
            float recordLength = forwardList->getNumberOfKeywords();

            ts_shared_ptr<vectorview<unsigned> > writeView;
            this->keywordIds->getWriteView(writeView);

            writeView->at(invertedListId) = keywordId;
            this->addInvertedListElement(invertedListId, forwardListOffset);
            this->invertedListSetToMerge.insert(invertedListId);

            float idf = this->getIdf(totalNumberOfDocuments, invertedListId);
            float score = this->computeRecordStaticScore(rankerExpression, recordBoost, recordLength, tf, idf, sumOfFieldBoost);

            forwardList->setKeywordRecordStaticScore(counter, score);
        }
    }
}


void InvertedIndex::addInvertedListElement(unsigned keywordId, unsigned recordId)
{
    ts_shared_ptr<vectorview<InvertedListContainerPtr> > writeView;
    this->invertedIndexVector->getWriteView(writeView);

    ASSERT( keywordId < writeView->size());

    writeView->at(keywordId)->addInvertedListElement(recordId);
}

const unsigned InvertedIndex::getInvertedListElementByDirectory(const unsigned invertedListId, const unsigned cursor) const
{
    /*if(invertedListId > this->getTotalNumberOfInvertedLists_ReadView() )
    {
        return NULL;
    }*/
	assert(invertedListId < this->getTotalNumberOfInvertedLists_ReadView());
    ts_shared_ptr<vectorview<InvertedListContainerPtr> > readView;
    this->invertedIndexVector->getReadView(readView);
    return readView->getElement(invertedListId)->getInvertedListElement(cursor);
}

//ReadView InvertedListSize
unsigned InvertedIndex::getInvertedListSize_ReadView(const unsigned invertedListId) const
{
    // A valid record ID is in the range [0, 1, ..., directorySize - 1]
    if(invertedListId >= this->getTotalNumberOfInvertedLists_ReadView() )
        return 0;

    ts_shared_ptr<vectorview<InvertedListContainerPtr> > readView;
    this->invertedIndexVector->getReadView(readView);
    return readView->getElement(invertedListId)->getReadViewSize();
}

unsigned InvertedIndex::getTotalNumberOfInvertedLists_ReadView() const
{
    ts_shared_ptr<vectorview<InvertedListContainerPtr> > readView;
    this->invertedIndexVector->getReadView(readView);
    return readView->size();
}

int InvertedIndex::getNumberOfBytes() const
{

    //return (sizeof(InvertedListElement) * this->totalSizeOfInvertedIndex) + sizeof(invertedIndexVector) + sizeof(this->totalSizeOfInvertedIndex) + sizeof(this->forwardIndex);
    return ~0;
}

void InvertedIndex::print_test() const
{
    Logger::debug("InvertedIndex is:");
    ts_shared_ptr<vectorview<InvertedListContainerPtr> > readView;
    this->invertedIndexVector->getReadView(readView);
    ts_shared_ptr<vectorview<unsigned> > keywordIdsReadView;
    this->keywordIds->getReadView(keywordIdsReadView);
    for (unsigned vectorIterator =0;
            vectorIterator != readView->size();
            vectorIterator++)
    {
        Logger::debug("Inverted List: %d, KeywordId: %d", vectorIterator , keywordIdsReadView->at(vectorIterator));
    	this->printInvList(vectorIterator);
    }
}

}}

