//$Id: Analyzer.h 3456 2013-06-14 02:11:13Z jiaying $

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

namespace srch2 {
namespace instantsearch {



/**
 * An Analyzer is used at query time to tokenize a queryString into
 * a vector of query keywords and also prevents very common words from
 * being passed into the search engine. */
class MYLIB_EXPORT Analyzer {
public:

	/**
	 * Creates an Analyzer.
	 */
	/*static Analyzer *create();// Default is NO_STEMMER_NORMALIZER
	 static Analyzer *create( StemmerNormalizerType stemNormType);*/
	static Analyzer *create(const StemmerNormalizerFlagType &stemNormType,
			const std::string &stemmerFilePath,
			const std::string &stopWordFilePath,
			const std::string &synonymFilePath,
			const SynonymKeepOriginFlag &synonymKeepOriginFlag,
			const std::string &delimiters,
			const AnalyzerType &analyzerType = STANDARD_ANALYZER);

	virtual void setRecordAllowedSpecialCharacters(
			const std::string &delimiters) = 0;
	virtual const std::string& getRecordAllowedSpecialCharacters() const = 0;


	/*
	 * this function applies the filter chain on the input and returns only the first output of
	 * the chain. It's assumed the input is just one word and only the first output is of interest.
	 */
	virtual std::string applyFilters(std::string input) = 0 ;

	/**
	 * @param[in] queryString - The query string .
	 * @param[in, out] queryKeywords - A vector of query keywords, which is the result tokening queryString.
	 * @param[in] splitterCharacter - A delimiter used in the tokenization, such as "+" or space " ".
	 *
	 * Three operations are applied to the input parameter queryString:
	 * - All characters in queryString are converted to lowercase.
	 * - The following characters
	 *   "\r\n\t!%&*^@#,._-+=|/\\{}[]()?~`,<>;:\'\"
	 *   are replaced from the queryString with delimiterCharacter.
	 * - The queryString is tokenized using splitterCharacter as the
	 *   tokenizing character.
	 * Note that the input string queryString is a declared as const
	 * and the above changes are not applied directly to the string.
	 */
	virtual void tokenizeQuery(const std::string &queryString,
			std::vector<std::string> &queryKeywords) const = 0;

	/*
	 *   Example URL: http://localhost:8081/search?q=hospital:title,description+services:description
	 *
	 *   @param queryString: input query string = "hospital:title,description+services:description"
	 *   @param queryKeywords : hospital and services
	 *   @param splitterCharacter: '+'
	 *   @param filterSplitterCharacter: ':'
	 *   @param fieldsAndCharacter: ','
	 *   @param fieldsOrCharacter: '.'
	 *   @param searchableAttributesNameToId: map of searchable attributes from name to Id
	 *   @param filter: vector of attribute hits
	 *   Note that the searchableAttributesNameToId is used to map an attribute name to its Id, which will be used in Attribute-based search
	 */
	// TODO: Refactor the function and its arguments. Possibly move to wrapper
	virtual void tokenizeQueryWithFilter(const std::string &queryString,
			std::vector<std::string> &queryKeywords,
			const char &splitterCharacter, const char &filterSplitterCharacter,
			const char &fieldsAndCharacter, const char &fieldsOrCharacter,
			const std::map<std::string, unsigned> &searchableAttributesNameToId,
			std::vector<unsigned> &filter) const = 0;

	/**
	 * Destructor to free persistent resources used by the Analyzer.
	 */
	virtual ~Analyzer() {
	};

};

}
}

#endif //__INCLUDE_INSTANTSEARCH__ANALYZER_H__
