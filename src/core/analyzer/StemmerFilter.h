/*
 * StemmerFilter.h
 *
 *  Created on: Jun 7, 2013
 *      Author: iman
 */

#ifndef __CORE_ANALYZER_STEMMERFILTER_H__
#define __CORE_ANALYZER_STEMMERFILTER_H__

// TODO: jamshid fix the macros all over the code
#define TRUE 1
#define FALSE 0

#include <instantsearch/platform.h>
#include <instantsearch/Analyzer.h>

#include <string>
#include <map>

#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>

#include "TokenStream.h"
#include "TokenFilter.h"

using std::string;

namespace srch2 {
namespace instantsearch {

struct TokenDetails {
	char * stemmedWordBuffer; /* buffder for word to be stemmed */
	int endStringOffset; /* offset to the end of the string */
	int stringOffset; /* a general offset into the string */
};
class StemmerContainer;
class StemmerFilter: public TokenFilter {
public:
	/*
	 * Constructor of the Stemmer Filter.
	 * It is the second constructor of the StemmerFilter. This calls a function (creatHeadWordsMap()) to construct the dictionary.
	 * */
	StemmerFilter(TokenStream* tokenStream, const string &stemmerFilePath);

	/*
	 * searchHeadWords() search if the given string is present in the HeadWordsDictionary \
     * */
	bool searchWord(const std::string &search) const;

	/*
	 * input : Token to be stemmed  Output : StemmedToken
	 * */
	std::string stemToken(const std::string &token) const;


	/*
	 * incrementToken() is a virtual function of class TokenOperator. Here we have to implement it. It goes on all tokens.
	 * */
	bool processToken();

	virtual ~StemmerFilter();

private:
	StemmerContainer& stemmerContainer;
	// StemmerType type;
};

class StemmerFilterInternal {
public:
	/*
	 * This function stems and calls the functions related to step1 to t
	 */
	static std::string stemUsingPorterRules(std::string token);

private:
	/*
	 *  cons(i) is TRUE <=> b[i] is a consonant.
	 *  */
	static int ifConsonant(struct TokenDetails * tokenDetail, int index);

	/* m() measures the number of consonant sequences between k0 and j. if c is
	 a consonant sequence and v a vowel sequence, and <..> indicates arbitrary
	 presence,

	 <c><v>       gives 0
	 <c>vc<v>     gives 1
	 <c>vcvc<v>   gives 2
	 <c>vcvcvc<v> gives 3
	 ....
	 */
	static int numberOfConsonantSequences(struct TokenDetails * tokenDetail);

	/*
	 * vowelinstem() is TRUE <=> k0,...j contains a vowel */
	static int vowelInStem(struct TokenDetails * tokenDetail);

	/* doublec(j) is TRUE <=> j,(j-1) contain a double consonant. */
	static int doubleConsonant(struct TokenDetails * tokenDetail, int j);

	/* cvc(i) is TRUE <=> i-2,i-1,i has the form consonant - vowel - consonant
	 and also if the second c is not w,x or y. this is used when trying to
	 restore an e at the end of a short word. e.g.

	 cav(e), lov(e), hop(e), crim(e), but
	 snow, box, tray.

	 */
	static int ConsonantVowelConsonantStructure(
			struct TokenDetails * tokenDetail, int i);

	/*
	 * ends(s) is TRUE <=> k0,...k ends with the string s.
	 * */
	static int endStr(struct TokenDetails * tokenDetail, const char * s);

	/*
	 * replaceBufferWithString(s) sets (j+1),...k to the characters in the string s
	 * */
	static void replaceBufferWithString(struct TokenDetails * tokenDetail,
			const char * s);

	/*
	 * depending on if there is consonant sequences or not, calls setto function.
	 * */
	static void callReplaceBufferWithStringoIfConsonant(
			struct TokenDetails * tokenDetail, const char * s);

	/* step1ab() gets rid of plurals and -ed or -ing. e.g.

	 caresses  ->  caress
	 ponies    ->  poni
	 ties      ->  ti
	 caress    ->  caress
	 cats      ->  cat

	 feed      ->  feed
	 agreed    ->  agree
	 disabled  ->  disable

	 matting   ->  mat
	 mating    ->  mate
	 meeting   ->  meet
	 milling   ->  mill
	 messing   ->  mess

	 meetings  ->  meet

	 */

	static void stepOneAB(struct TokenDetails * tokenDetail);

	/*
	 *  step1c() turns terminal y to i when there is another vowel in the stem.
	 *  */
	static void stepOneC(struct TokenDetails * tokenDetail);

	/*
	 * step2() maps double suffices to single ones. so -ization ( = -ize plus
	 -ation) maps to -ize etc. note that the string before the suffix must give
	 m() > 0.
	 */
	static void stepTwo(struct TokenDetails * tokenDetail);

	/*
	 *  step3() deals with -ic-, -full, -ness etc. similar strategy to step2.
	 *  */
	static void stepThree(struct TokenDetails * tokenDetail);

	/*
	 *  step4() takes off -ant, -ence etc., in context <c>vcvc<v>.
	 *  */
	static void stepFour(struct TokenDetails * tokenDetail);

	/*
	 *  step5() removes a final -e if m() > 1, and changes -ll to -l if m() > 1.
	 *  */
	static void stepFive(struct TokenDetails * tokenDetail);

};

}
}

#endif /* __CORE_ANALYZER__STEMMERFILTER_H__ */
