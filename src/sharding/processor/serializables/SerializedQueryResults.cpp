#include "SerializedQueryResults.h"
#include "QueryResultsInternal.h"


#define UNINIT_WORD 0

typedef vector<QueryResult*>::const_iterator Result;

void copyFixedPartOfResultsToPtrAndExtractMatchingKeywords(
    const std::vector<QueryResult*>& results,
    unsigned maxMatchingKeywords, QueryResultKeyword *fixedPartPtr, 
    vector<std::string>& queryMatchingStrings) {

  typedef vector<TrieNodePointer>::iterator TrieNodeIter;

  vector<TrieNodePointer> queryMatchingPtrs;
  for(Result qr = results.begin(); qr != results.end(); ++qr) {
    int i;
    for(i = 0; i < (*qr)->matchingKeywordTrieNodes.size(); ++i) {
      fixedPartPtr->attributeBitmap = (*qr)->attributeBitmaps[i];
      fixedPartPtr->editDistance = (*qr)->editDistances[i];
      fixedPartPtr->termType = (*qr)->termTypes[i];
 
      TrieNodeIter pos = std::find(queryMatchingPtrs.begin(), 
          queryMatchingPtrs.end(), (*qr)->matchingKeywordTrieNodes[i]);
      if(pos == queryMatchingPtrs.end()) {
        queryMatchingPtrs.push_back((*qr)->matchingKeywordTrieNodes[i]);
        queryMatchingStrings.push_back((*qr)->matchingKeywords[i]);
        fixedPartPtr->keyword = queryMatchingStrings.size();
      } else {
        fixedPartPtr->keyword = pos - queryMatchingPtrs.begin();
      } 
    }
    fixedPartPtr += maxMatchingKeywords - i;
  }
}

void
calculateMaxNumberOfMatchingKeywordPerRecordAndTotalExternalIdLength(
    const std::vector<QueryResult*>& results,unsigned& max,unsigned& length) {
  max = length = 0;
  for(Result qr = results.begin(); qr != results.end(); ++qr) {
    std::max(max, (unsigned) (*qr)->matchingKeywordTrieNodes.size());
    length += (*qr)->externalRecordId.length();
  }
}

unsigned 
calculateLengthOfAllUniqueMatchingKeywords(const 
    std::vector<std::string>& matches) {
  unsigned total = 0;
  for(vector<std::string>::const_iterator keyword = matches.begin(); 
      keyword != matches.end(); ++keyword) {
    total += keyword->length();
  }
  return total;
}

void* placeScoresInBuffer(const std::vector<QueryResult*>& results, 
    float *buffer) {
  for(Result qr = results.begin(); qr != results.end(); ++qr) {
    *(buffer++) = (*qr)->getResultScore().getFloatTypedValue();
  }
  return buffer;
}

void* placeMatchingKeywordsInBuffer(const std::vector<std::string> keywords,
   char *const buffer, unsigned offsetOffsets, 
    unsigned offsetWords) {
  unsigned *offset = (unsigned*) (buffer + offsetOffsets);
  char *word = buffer + offsetWords;

  for(std::vector<std::string>::const_iterator i = keywords.begin();
    i  != keywords.end(); ++i) {
    *(offset++) = word - buffer;
    memcpy(word, (*i).data(), (*i).length());
    word += (*i).length();
  }
  *offset = word - buffer;

  return word;
}

void* placeExternalIdsInBuffer(const std::vector<QueryResult*> results,
    char *const buffer, unsigned offsetOffsets, 
    unsigned offsetWords) {
  unsigned *offset = (unsigned*) buffer + offsetOffsets;
  char *word = buffer + offsetWords;

  for(Result i = results.begin(); i != results.end(); ++i) {
    *(offset++) = word - buffer;
    memcpy(word,(*i)->externalRecordId.data(),(*i)->externalRecordId.length());
    word += (*i)->externalRecordId.length();
  }
  *offset = word - buffer;

  return word;
}

void serialize(const std::vector<QueryResult*>& results, void *buffer,
    const void *fixedPart, unsigned sizeofFixedPart, 
    const std::vector<string>& queryMatchingStrings, 
    unsigned totalExternalIdLength, unsigned totalMatchingKeyword) {
  void *currentPlaceInSerializedBuffer = buffer;

  currentPlaceInSerializedBuffer = 
    placeScoresInBuffer(results, (float*) buffer);

  memcpy(currentPlaceInSerializedBuffer, fixedPart, sizeofFixedPart);
  currentPlaceInSerializedBuffer = 
  ((char*) currentPlaceInSerializedBuffer + sizeofFixedPart);

  unsigned offsetToExternalIdsOffsets =
    (char*) currentPlaceInSerializedBuffer - (char*) buffer;
  unsigned offsetToKeywordsOffsets = 
    offsetToExternalIdsOffsets + (results.size()+1) * sizeof(unsigned);
  unsigned offsetToExternalIds = offsetToExternalIdsOffsets + 
    (queryMatchingStrings.size()+1) * sizeof(int);

  currentPlaceInSerializedBuffer = 
    placeExternalIdsInBuffer(results, (char*) buffer,
        offsetToExternalIdsOffsets, offsetToExternalIds);

  unsigned offsetToKeywords =
    (char*) currentPlaceInSerializedBuffer - (char*) buffer;

  currentPlaceInSerializedBuffer = 
    placeMatchingKeywordsInBuffer(queryMatchingStrings, (char*) buffer, 
        offsetToKeywordsOffsets, offsetToKeywords);
}

SerializedQueryResults::SerializedQueryResults(std::allocator<char> alloc, 
    const QueryResults& queryResults) {
  
  vector<std::string> queryMatchingStrings;
  const vector<QueryResult*>& results = queryResults.impl->sortedFinalResults;

  maxMatchingKeywords = 0;
  unsigned totalExternalIdLength = 0;
  numResults = queryResults.getNumberOfResults();
  unsigned totalMatchingKeywordLength = 0;

  calculateMaxNumberOfMatchingKeywordPerRecordAndTotalExternalIdLength(
      results, maxMatchingKeywords, totalExternalIdLength);


  size_t sizeofFixedPart = 
    sizeof(QueryResultKeyword) * maxMatchingKeywords * numResults;
  QueryResultKeyword *fixedPart = (QueryResultKeyword*) 
    calloc(sizeof(QueryResultKeyword), maxMatchingKeywords * numResults);

  copyFixedPartOfResultsToPtrAndExtractMatchingKeywords(results, 
      maxMatchingKeywords, fixedPart, queryMatchingStrings);
 
  totalMatchingKeywordLength = 
    calculateLengthOfAllUniqueMatchingKeywords(queryMatchingStrings);

  buffer = 
    alloc.allocate( sizeof(float) + sizeof(unsigned) * numResults +
        sizeofFixedPart + 
        sizeof(unsigned) * (numResults+1) + totalExternalIdLength +
        sizeof(unsigned) * (queryMatchingStrings.size()+1) + 
        totalMatchingKeywordLength);

  serialize(results, buffer, fixedPart, sizeofFixedPart, queryMatchingStrings,
      totalExternalIdLength, totalMatchingKeywordLength);

  scores = (float*) buffer;
  fixedKeywords = (QueryResultKeyword*) (scores + numResults);
  externalIds = (unsigned*) (fixedKeywords + numResults*maxMatchingKeywords);
  keywords = (unsigned*) (externalIds + numResults);
}

unsigned SerializedQueryResults::getNumberOfResults() const {
  return numResults;
}

std::string 
SerializedQueryResults::getStringByOffset(const unsigned* const offset) const {
  return std::string(
      *(buffer + *offset), *(buffer + *(offset + 1)));
}

std::string SerializedQueryResults::getRecordId(const unsigned position) const {
  if(position > numResults - 1) return std::string();
  return getStringByOffset(externalIds + position);
}

TypedValue 
SerializedQueryResults::getResultsScore(const unsigned position) const {
  TypedValue rtn;
  rtn.setTypedValue(scores[position]);

  return rtn;
}

std::string 
SerializedQueryResults::getResultsScoreString(const unsigned i) const {
  return getResultsScore(i).toString();
}

QueryResultKeyword 
SerializedQueryResults::getKeyword(unsigned p, unsigned w) const {
  return fixedKeywords[p * maxMatchingKeywords + w];
}

void SerializedQueryResults::getMatchingKeywords(const unsigned position, 
    std::vector<std::string> &matchingKeywords) const {
  for(int w=0; w < maxMatchingKeywords; ++w) {
    if(getKeyword(position, w).value == UNINIT_WORD) break;
    
    unsigned offset = getKeyword(position, w).keyword;
    matchingKeywords.push_back(getStringByOffset(keywords + offset));
  }
}

void SerializedQueryResults::getEditDistances(const unsigned position, 
    std::vector<unsigned>& editDistances) const {
  for(int w=0; w < maxMatchingKeywords; ++w) {
    if(getKeyword(position, w).value == UNINIT_WORD) break;
    
    editDistances.push_back(getKeyword(position, w).editDistance);
  }
}

void SerializedQueryResults::getMatchedAttributeBitmaps(
    const unsigned position, 
    std::vector<unsigned>& matchedAttributeBitmap) const {
  for(int w=0; w < maxMatchingKeywords; ++w) {
    if(getKeyword(position, w).value == UNINIT_WORD) break;
    
    matchedAttributeBitmap.push_back(getKeyword(position, w).attributeBitmap);
  }
}

void SerializedQueryResults::getTermTypes(const unsigned position, 
    std::vector<TermType>& tt) const {
  for(int w=0; w < maxMatchingKeywords; ++w) {
    if(getKeyword(position, w).value == UNINIT_WORD) break;
    
    tt.push_back(getKeyword(position, w).termType);
  }
}
