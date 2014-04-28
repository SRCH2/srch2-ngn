#ifndef __SERIALIZED_QUERY_RESULTS_H__
#define __SERIALIZED_QUERY_RESULTS_H__

#include <string>
#include "instantsearch/Term.h"
#include "instantsearch/QueryResults.h"
#include "instantsearch/TypedValue.h"
#include "instantsearch/Constants.h"

using srch2::instantsearch::TermType;

struct QueryResultKeyword {
  union {
    struct {
      unsigned attributeBitmap;
      unsigned editDistance;
      unsigned keyword;
      TermType termType;
    };
  long value;
  };
};

class SerializedQueryResults {
  char *buffer;
  unsigned maxMatchingKeywords;
  unsigned numResults;

  float *scores;
  unsigned *externalIds;
  QueryResultKeyword *fixedKeywords;
  unsigned *keywords;

  std::string getStringByOffset(const unsigned* const) const;
  QueryResultKeyword getKeyword(unsigned, unsigned)  const;
 public: 
  SerializedQueryResults(std::allocator<char>, const QueryResults&);
  unsigned getNumberOfResults() const;
  std::string getRecordId(const unsigned) const;
  TypedValue getResultsScore(const unsigned) const;
  std::string getResultsScoreString(const unsigned) const;
  void getMatchingKeywords(const unsigned, std::vector<std::string>&) const;
  void getEditDistances(const unsigned, std::vector<unsigned>&) const;
  void 
    getMatchedAttributeBitmaps(const unsigned, std::vector<unsigned>&) const;
  void getTermTypes(const unsigned, std::vector<TermType>& tt) const;
};


#endif /* __SERIALIZED_QUERY_RESULTS_H__ */
