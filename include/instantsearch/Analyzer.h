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

struct PositionalTerm {
	string term;
	unsigned position;
	unsigned charPosition;
};

struct TokenAttributeHits {
    /** Each entry has position information as follows:
     *  Attribute -> First 8bits -> Attribute in which the token hit occurred
     *  Hits -> Last 24 bits -> Position within the attribute where the token hit occurred.
     *  The positions start from 1, this is because the positions in PositionIndex are ZERO terminated.
     *
     *  The maximum number of allowed Attributes is checked by the following assert
     *  ASSERT( attribute <  0xff);
     *
     *  i.e. 255
     *
     *  The maximum number of the positionHit is checked by the following assert
     *  ASSERT( position <  0xffffff);
     *
     * i.e. 4 294 967 295
     *
     */
    vector<unsigned> attributeList;
    vector<unsigned> offsetOfTermInOrigRecord;
};


class Record;
class StemmerContainer;
class StopWordContainer;
class  ProtectedWordsContainer;
class  SynonymContainer;
typedef unsigned CharType;
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
             const std::string &delimiters,
             const AnalyzerType &analyzerType = STANDARD_ANALYZER,
             const std::string &chineseDictFilePath = "");


	void setRecordAllowedSpecialCharacters(const std::string &delimiters);

	const std::string& getRecordAllowedSpecialCharacters() const ;

    /*
     *  isPrefix is a way to inform the analyzer that stop filter should not be applied
     *  the passed string.
     */
	std::string applyFilters(std::string input, bool isPrefix);

	// clear the initial states of the filters in the analyzer, e.g.,
	// for those filters that have an internal buffer to keep tokens.
	// Such an internal buffer can have leftover tokens from
	// the previous query (possibly an invalid query)
	void clearFilterStates();

	void tokenizeQuery(const std::string &queryString,
			std::vector<PositionalTerm> &queryKeywords) const;

	void tokenizeRecord(const Record *record,
	        std::map<string, TokenAttributeHits> &tokenAttributeHitsMap) const;

	AnalyzerType getAnalyzerType() const;

    void load(boost::archive::binary_iarchive &ia);

    void save(boost::archive::binary_oarchive &oa);

	// TODO: Refactor the function and its arguments. Possibly move to wrapper
	void tokenizeQueryWithFilter(const std::string &queryString,
			std::vector<PositionalTerm> &queryKeywords,
			const char &splitterCharacter,
			const char &filterSplitterCharacter,
			const char &fieldsAndCharacter,
			const char &fieldsOrCharacter,
			const std::map<std::string, unsigned> &searchableAttributesNameToId,
			std::vector<unsigned> &filter) const ;

	void fillInCharacters(const char * data);
	bool processToken();
	std::vector<CharType> & getProcessedToken();
	unsigned getProcessedTokenOffset();
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

