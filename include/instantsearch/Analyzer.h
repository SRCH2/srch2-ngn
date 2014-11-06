//$Id: Analyzer.h 3456 2013-07-29 02:11:13Z iman $

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

#ifndef __INCLUDE_INSTANTSEARCH__ANALYZER_H__
#define __INCLUDE_INSTANTSEARCH__ANALYZER_H__

#include <instantsearch/platform.h>
#include <instantsearch/Constants.h>
#include <vector>
#include <string>
#include <map>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

using namespace std;

namespace srch2 {
namespace instantsearch {

class AnalyzerInternal;
enum AnalyzedTokenType {
	ANALYZED_ORIGINAL_TOKEN,
	ANALYZED_SYNONYM_TOKEN
};
struct AnalyzedTermInfo {
	string term;
	unsigned position;
	unsigned charOffset;
	unsigned charLength;
	AnalyzedTokenType analyzedTokenType;
};

struct TokenAttributeHits {
	// list of attributes this token belongs to
    vector<unsigned> attributeIdList;
    // list of word positions of a term in all attributes
    vector<unsigned> positionsOfTermInAttribute;
    // list of character offsets of a term in all attributes
    vector<unsigned> charOffsetsOfTermInAttribute;
    // list of original term's length of a synonym term in all attributes
    vector<unsigned> charLensOfTermInAttribute;
    // list of flag indicating whether a current position/offset of a term is synonym or not.
    vector<AnalyzedTokenType> typesOfTermInAttribute;
};


class Record;
class StemmerContainer;
class StopWordContainer;
class ProtectedWordsContainer;
class SynonymContainer;
class ChineseDictionaryContainer;
/**
 * An Analyzer is used at query time to tokenize a queryString into
 * a vector of query keywords and also prevents very common words from
 * being passed into the search engine. */
class MYLIB_EXPORT Analyzer {
public:
    Analyzer(Analyzer& analyzerObject);

    Analyzer(const StemmerContainer *stemmer,
             const StopWordContainer *stopWords,
             const ProtectedWordsContainer *protectedWords,
             const SynonymContainer *synonyms,
             const std::string &allowedSpecialCharacters,
             const AnalyzerType &analyzerType = STANDARD_ANALYZER,
             const ChineseDictionaryContainer* chineseDict = NULL);


	void setRecordAllowedSpecialCharacters(const std::string &allowedSpecialCharacters);

	const std::string& getRecordAllowedSpecialCharacters() const ;

    /*
     *  isPrefix is a way to inform the analyzer that stop filter should not be applied
     *  the passed string.
     */
	void applyFilters(const std::string & input, std::vector<std::string> & queryTokens, bool isPrefix);

	// clear the initial states of the filters in the analyzer, e.g.,
	// for those filters that have an internal buffer to keep tokens.
	// Such an internal buffer can have leftover tokens from
	// the previous query (possibly an invalid query)
	void clearFilterStates();

	void tokenizeQuery(const std::string &queryString,
			std::vector<AnalyzedTermInfo> &queryKeywords, bool isPrefix = false) const;

	void tokenizeRecord(const Record *record,
	        std::map<string, TokenAttributeHits> &tokenAttributeHitsMap) const;

	AnalyzerType getAnalyzerType() const;

    void load(boost::archive::binary_iarchive &ia);

    void save(boost::archive::binary_oarchive &oa);

	void fillInCharacters(const char * data);
	bool processToken();
	std::vector<CharType> & getProcessedToken();
	unsigned getProcessedTokenCharOffset();
	unsigned getProcessedTokenPosition();
	unsigned getProcessedTokenLen();
	AnalyzedTokenType getProcessedTokenType();
	/**
	 * Destructor to free persistent resources used by the Analyzer.
	 */
	~Analyzer();

private:
	AnalyzerInternal * analyzerInternal;

};

}
}

#endif //__INCLUDE_INSTANTSEARCH__ANALYZER_H__

