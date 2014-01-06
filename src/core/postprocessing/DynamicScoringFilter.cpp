// $Id: DynamicScoringFilter.h 2013-12-01 RJ $

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

 * Copyright © 2013 SRCH2 Inc. All rights reserved
 */

#include <instantsearch/DynamicScoringFilter.h>
#include <util/AttributeIterator.h>
#include <index/ForwardIndex.h>
#include <instantsearch/QueryResults.h>
#include <query/QueryResultsInternal.h>
#include <operation/IndexSearcherInternal.h>
#include <algorithm>

namespace srch2 {
namespace instantsearch {

DynamicScoringFilter::DynamicScoringFilter(unsigned numberOfAttributes)
  : numberOfAttributes(numberOfAttributes), 
  attributeBoosts(new AttributeBoost[numberOfAttributes]) {} 

DynamicScoringFilter::~DynamicScoringFilter() {
  if(attributeBoosts != NULL)
    delete attributeBoosts;
}

inline
bool getQueryKeywordIds(const Trie& trie, const TrieNode* root,
    const std::vector<Term*>& terms, unsigned* keywordID) {
  const TrieNode *keywordNode;
  std::vector<Term*>::const_iterator term(terms.begin());
  for(; term != terms.end(); ++keywordID, ++term) {
    // It's possible for this function to return a NULL node,
    // i.e., when the keyword doesn't exist in the trie.
    if(!(keywordNode = 
          trie.getTrieNodeFromUtf8String(root, *(*term)->getKeyword()))) 
      return false;
    *keywordID= keywordNode->getId();
  }
  return true;
  //potential TODO: we might need to modify the matching term hit on and
  //or condition
}

bool attributeBoostSearchCondition(const AttributeBoost&  lhs,
    unsigned rhs) {
  return lhs.attributeMask < rhs;
}

/* This function returns the AttributeBoost structure associated with a given
   attribute id. Its behaviour is only well defined for Attribute ids with
   associated dynamic boosts in this query */
AttributeBoost* 
DynamicScoringFilter::getAttributeBoost(unsigned attributeID)  {
  return std::lower_bound(attributeBoosts, attributeBoosts+numberOfAttributes,
      (unsigned) (1 << attributeID),
      &attributeBoostSearchCondition);
}

void updateAttributeHitCount(DynamicScoringFilter& filter,
    unsigned attributeMask) {
  for(AttributeIterator attribute(attributeMask); 
      attribute.hasNext(); ++attribute) {
    ++filter.getAttributeBoost(*attribute)->hitCount;
  }
}

float dynamicRuntimeScore(const ForwardList& record,
    const SchemaInternal *const schema,
    DynamicScoringFilter& filter,
    const unsigned *const queryKeywordIDs, unsigned numberOfKeywords) {
  struct KeywordBoost *keywordBoosts = new KeywordBoost[numberOfKeywords];

  // For each keyword in Query 
  {
    const unsigned *queryKeyword= queryKeywordIDs;
    unsigned i=0;
  for(; i < numberOfKeywords; ++i, ++queryKeyword) {
    if(record.haveWordInRange(schema, *queryKeyword, *queryKeyword,
          filter.boostedAttributeMask, keywordBoosts[i].id, 
          keywordBoosts[i].attributeMask, keywordBoosts[i].score)) {
      keywordBoosts[i].attributeMask&= filter.boostedAttributeMask;
      //records runtime score changed for this keyword
      updateAttributeHitCount(filter, keywordBoosts[i].attributeMask);
    }
  }
  }

  //TODO: handle matchingKeyword

  float rtn= srch2::instantsearch::DynamicScoringRanker::
    CalculateAndAggregrateDynamicScore(keywordBoosts, 
        numberOfKeywords, filter);

  //Clear the counts for the next record
  for(unsigned i=0; i < filter.numberOfAttributes; ++i) 
    filter.attributeBoosts[i].hitCount=0;

  delete [] keywordBoosts;
  return rtn;
}

inline
bool QueryResultCompareScore(const QueryResult *const lhs,
    const QueryResult *const rhs) {
  return lhs->_score > rhs->_score;
}

void DynamicScoringFilter::doFilter(IndexSearcher *indexSearcher,
    const Query *query, QueryResults *input, QueryResults *output) {
  bool isValid;
  const ForwardList* forwardList;
  
  float dynamicScore;
  const ForwardIndex& 
    forwardIndex(*((IndexSearcherInternal*) indexSearcher)->getForwardIndex());
  //Get Read Lock on Trie
  boost::shared_ptr<TrieRootNodeAndFreeList> root;
  unsigned *queryKeywordIDs = new unsigned[query->getQueryTerms()->size()];

  output->copyForPostProcessing(input);

  ((IndexSearcherInternal*) indexSearcher)->getTrie()
    ->getTrieRootNode_ReadView(root);

  // If one query keyword doesn't exist on the trie,
  // the filter becomes a "no-op".
  if(!getQueryKeywordIds(*((IndexSearcherInternal*) indexSearcher)->getTrie(),
      root->root, *query->getQueryTerms(), queryKeywordIDs))
    return;


  std::vector<QueryResult*>& results= output->impl->sortedFinalResults;

  for(std::vector<QueryResult*>::iterator result= results.begin();
      result != results.end(); ++result) {
    forwardList= forwardIndex.getForwardList((*result)->internalRecordId,
        isValid);
    ASSERT(isValid);
    dynamicScore= dynamicRuntimeScore(*forwardList,
        forwardIndex.getSchema(), *this,
        queryKeywordIDs, query->getQueryTerms()->size());
    //We assume "qf" can only increase the score
    dynamicScore= std::max(dynamicScore,
        (*result)->_score.getFloatTypedValue());

    (*result)->_score.setTypedValue(dynamicScore); 
  }
      
  std::sort(results.begin(), results.end(), QueryResultCompareScore);
  delete [] queryKeywordIDs;
}

}}
