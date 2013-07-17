
// $Id: QueryResults.cpp 3456 2013-06-14 02:11:13Z jiaying $

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

#include "QueryResultsInternal.h"
#include "operation/IndexSearcherInternal.h"

namespace srch2
{
namespace instantsearch
{







QueryResultFactory::QueryResultFactory(){
	impl = new QueryResultFactoryInternal();
}


/**
 * Creates a QueryResults object.
 * @param[in] indexSearcher the reference to an IndexSearcher object.
 * @param[in] query the reference to a Query object.
 */
QueryResults::QueryResults(QueryResultFactory * resultsFactory, const  IndexSearcher* indexSearcher, Query* query){
	impl = new QueryResultsInternal(resultsFactory,dynamic_cast<const IndexSearcherInternal *>(indexSearcher),query );
}

QueryResults::~QueryResults(){
	delete impl;
}

/**
 * Checks if the iterator reaches the end. @return false if
 * there is no next item in the QueryResults.
 */
/*virtual bool next() = 0;*/

/**
 * Gets the number of result items in the QueryResults object.
 */
unsigned QueryResults::getNumberOfResults() const{
	 return impl->sortedFinalResults.size();
}

/**
 * Gets the current record id while iterating through the QueryResults object.
 */
/*virtual int getRecordId() = 0;*/

/**
 * Gets the record id of the 'position'-th item in the QueryResults object.
 */
std::string QueryResults::getRecordId(unsigned position) const {
    ASSERT (position < this->getNumberOfResults());
    return impl->sortedFinalResults.at(position)->externalRecordId;
}

/**
 * Gets the record id of the 'position'-th item in the QueryResults object.
 * TODO: Move/remove getInternalRecordId to internal include files. There should be a mapping from external
 * Used to access the InMemoryData.
 */
unsigned QueryResults::getInternalRecordId(unsigned position) const {
    ASSERT (position < this->getNumberOfResults());
    return impl->sortedFinalResults.at(position)->internalRecordId;
}

std::string QueryResults::getInMemoryRecordString(unsigned position) const {
    unsigned internalRecordId = this->getInternalRecordId(position);
    return impl->indexSearcherInternal->getInMemoryData(internalRecordId);
}

/**
 * Gets the score of the 'position'-th item in the QueryResults object.
 */
std::string QueryResults::getResultScoreString(unsigned position) const {
    ASSERT (position < this->getNumberOfResults());
    return impl->sortedFinalResults.at(position)->_score.toString();
}
Score QueryResults::getResultScore(unsigned position) const {
    ASSERT (position < this->getNumberOfResults());
    return impl->sortedFinalResults.at(position)->_score;
}

/**
 *
 * Gets the matching keywords of the 'position'-th item in
 * the QueryResults object. For each query keyword return a matching
 * keyword in a record. If the query keyword is prefix then return a
 * matching prefix. In the case of multiple matching keywords for a
 * query keyword,the best match based on edit distance is returned.
 *
 * For example, for the query "ulman compilor", a result
 * record with keywords "ullman principles compiler" will
 * return ["ullman","compiler"] in the matchingKeywords
 * vector.  In particular, "compiler" is the best matching
 * keyword in the record for the second query keyword
 * "compilor".
 */
void QueryResults::getMatchingKeywords(const unsigned position, std::vector<std::string> &matchingKeywords) const {
    matchingKeywords.assign(impl->sortedFinalResults[position]->matchingKeywords.begin(),
    		impl->sortedFinalResults[position]->matchingKeywords.end());
}

/**
 * Gets the edit distances of the 'position'-th item in the
 * QueryResults object.
 * @param[in] position the position of a result in the returned
 * results (starting from 0).
 * @param[out] editDistances a vector of edit distances of the
 * best matching keywords in this result record with respect
 * to the query keywords.
 *
 * For example, for the query "ulman compilor", a result
 * recorprivate:
struct Impl;
Impl *impl;d with keywords "ullman principles conpiler" will
 * return [1,2] in the editDistances vector.  In particular,
 * "2" means that the best matching keyword ("conpiler") in
 * this record has an edit distance 2 to the second query
 * keyword "compilor".
 */
void QueryResults::getEditDistances(const unsigned position, std::vector<unsigned> &editDistances) const {
    editDistances.assign(impl->sortedFinalResults[position]->editDistances.begin(),
    		impl->sortedFinalResults[position]->editDistances.end());
}

// The following two functions only work for attribute based search
void QueryResults::getMatchedAttributeBitmaps(const unsigned position, std::vector<unsigned> &matchedAttributeBitmaps) const {
	matchedAttributeBitmaps.assign(impl->sortedFinalResults[position]->attributeBitmaps.begin(),
			impl->sortedFinalResults[position]->attributeBitmaps.end());
}

void QueryResults::getMatchedAttributes(const unsigned position, std::vector<std::vector<unsigned> > &matchedAttributes) const {
	//TODO opt
	const vector<unsigned> &matchedAttributeBitmaps = impl->sortedFinalResults[position]->attributeBitmaps;
	matchedAttributes.resize(matchedAttributeBitmaps.size());

	for(int i = 0; i < matchedAttributeBitmaps.size(); i++)
	{
		unsigned idx = 0;
		unsigned matchedAttributeBitmap = matchedAttributeBitmaps[i];
		matchedAttributes[i].clear();
		while(matchedAttributeBitmap)
		{
			if(matchedAttributeBitmap & 0x1)
			{
				matchedAttributes[i].push_back(idx);
			}
			matchedAttributeBitmap >>= 1;
			++idx;
		}
	}
}
/*
 *   In Geo search return distance between location of the result and center of the query rank.
 *   TODO: Change the name to getGeoDistance()
 *   // only the results of MapQuery have this
 */
double QueryResults::getPhysicalDistance(const unsigned position) const {
    ASSERT (position < this->getNumberOfResults());
    return impl->sortedFinalResults.at(position)->physicalDistance;
}

//TODO: These three functions for internal debugging. remove from the header
void QueryResults::printStats() const {
    impl->stat->print();
}

void QueryResults::printResult() const {
	// show attributeBitmaps
		vector<unsigned> attributeBitmaps;
		vector<vector<unsigned> > attributes;
		vector<string> matchedKeywords;
		cout << "Result count" <<": " << this->getNumberOfResults() << endl;;
		for(int i = 0; i < this->getNumberOfResults(); i++)
		{
			std::cout << "Result #" << i << ":" <<endl;
			this->getMatchedAttributeBitmaps(i, attributeBitmaps);
			this->getMatchingKeywords(i, matchedKeywords);
			this->getMatchedAttributes(i, attributes);
			for(int j = 0; j < attributeBitmaps.size(); j++)
			{
				cout << matchedKeywords[j] << " " << attributeBitmaps[j] << "{";
				for(int k = 0; k < attributes[j].size(); k++)
					cout << attributes[j][k] << " ";
				cout << "} | ";
			}

			cout << endl;
		}

}

void QueryResults::addMessage(const char* msg) {
	 impl->stat->addMessage(msg);
}


}}
