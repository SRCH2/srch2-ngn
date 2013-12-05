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
  : numberOfAttributes(numberOfAttributes) {}

inline
void getQueryKeywordIds(const Trie& trie, const TrieNode* root,
    const std::vector<Term*>& keywords, unsigned* keywordID) {
  std::vector<Term*>::const_iterator keyword(keywords.begin());
  for(; keyword != keywords.end(); ++keywordID, ++keyword) {
     *keywordID=
       (trie.
        getTrieNodeFromUtf8String(root, *(*keyword)->getKeyword()))->getId();
  }
//  *keywordID=
//  (trie.getTrieNodeFromUtf8String(root, *(*keyword)->getKeyword()))->getId();
}

bool attributeBoostSearcher(const AttributeBoost&  lhs,
    unsigned rhs) {
  return lhs.attributeMask < rhs;
}

AttributeBoost* 
DynamicScoringFilter::getAttributeBoost(unsigned attributeID)  {
  return std::lower_bound(attribute, attribute+numberOfAttributes,
      (unsigned) (1 << attributeID),
      &attributeBoostSearcher);
}

void updateAttributeHitCount(DynamicScoringFilter& filter,
    unsigned attributeMask) {
  for(AttributeIterator attribute(attributeMask);
      attribute.hasNext();
      ++attribute) {
  ++filter.getAttributeBoost(*attribute)->hitCount;
  }
}

float newScore(const ForwardList& record, const SchemaInternal *const schema,
    DynamicScoringFilter& filter,
    const unsigned *const queryKeywordIDs, unsigned numberOfKeywords) {
  struct KeywordBoost keyword[numberOfKeywords+1];

  /* For each keyword in Query */
  {
    const unsigned *queryKeyword= queryKeywordIDs;
    unsigned i=0;
  for(; i < numberOfKeywords; ++i) {
    if(record.haveWordInRange(schema, *queryKeyword, *queryKeyword+1,
          filter.boostedAttributeMask,
          keyword[i].id, keyword[i].attributeMask, keyword[i].score)) {
      keyword[i].attributeMask&= filter.boostedAttributeMask;
      //records runtime score changed for this keyword
      updateAttributeHitCount(filter, keyword[i].attributeMask);
  /* end if score changed*/} /* end for keywords */ } /*end for scope*/}

  //TODO: handle matchingKeyword

  float rtn= srch2::instantsearch::DynamicScoringRanker::
    CalculateAndAggregrateDynamicScore(keyword, numberOfKeywords, filter);

  for(unsigned i=0; i < filter.numberOfAttributes; ++i) 
    filter.attribute[i].hitCount=0;

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
  unsigned queryKeywordIDs[query->getQueryTerms()->size()];


	((IndexSearcherInternal*) indexSearcher)->getTrie()
    ->getTrieRootNode_ReadView(root);
  getQueryKeywordIds(*((IndexSearcherInternal*) indexSearcher)->getTrie(),
      root->root,
      *query->getQueryTerms(),
      queryKeywordIDs);

  output->copyForPostProcessing(input); 

  std::vector<QueryResult*>& results= output->impl->sortedFinalResults;

  for(std::vector<QueryResult*>::iterator result= results.begin();
      result != results.end(); ++result) {
    forwardList= forwardIndex.getForwardList((*result)->internalRecordId,
        isValid);
    assert(isValid);
    dynamicScore= newScore(*forwardList, forwardIndex.getSchema(), *this,
        queryKeywordIDs, query->getQueryTerms()->size());
    dynamicScore= std::max(dynamicScore,
        (*result)->_score.getFloatTypedValue());

    (*result)->_score.setTypedValue(dynamicScore); 
  }
      
  std::sort( results.begin(), results.end(), QueryResultCompareScore);
}

}}
