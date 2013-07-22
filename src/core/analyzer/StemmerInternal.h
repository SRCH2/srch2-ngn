// $Id: StemmerInternal.h 3074 2012-12-06 22:26:36Z oliverax $

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

#ifndef __CORE_ANALYZER__STEMMER_H__
#define __CORE_ANALYZER__STEMMER_H__

#define TRUE 1
#define FALSE 0


#include <instantsearch/platform.h>
#include <instantsearch/Analyzer.h>

#include <string>
#include <map>

#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>

using std::string;

namespace srch2
{
namespace instantsearch
{

struct token_details {
    char * b;       /* buffer for word to be stemmed */
    int k;          /* offset to the end of the string */
    int j;          /* a general offset into the string */
};

class StemmerInternal
{
public:
    static struct token_details * create_stemmer(void)
    {
        return (struct token_details *) new struct token_details;
    }

    static void free_stemmer(struct token_details * z)
    {
        delete z;
    }

    static int  stem(struct token_details * z, std::string token, int k);

private:


    /* cons(i) is TRUE <=> b[i] is a consonant. */
    static int cons(struct token_details * z, int i);


    /* m() measures the number of consonant sequences between k0 and j. if c is
       a consonant sequence and v a vowel sequence, and <..> indicates arbitrary
       presence,

          <c><v>       gives 0
          <c>vc<v>     gives 1
          <c>vcvc<v>   gives 2
          <c>vcvcvc<v> gives 3
          ....
    */
    static int m(struct token_details * z);


    /* vowelinstem() is TRUE <=> k0,...j contains a vowel */
    static int vowelinstem(struct token_details * z);


    /* doublec(j) is TRUE <=> j,(j-1) contain a double consonant. */
    static int doublec(struct token_details * z, int j);


    /* cvc(i) is TRUE <=> i-2,i-1,i has the form consonant - vowel - consonant
       and also if the second c is not w,x or y. this is used when trying to
       restore an e at the end of a short word. e.g.

          cav(e), lov(e), hop(e), crim(e), but
          snow, box, tray.

    */
    static int cvc(struct token_details * z, int i);

    /* ends(s) is TRUE <=> k0,...k ends with the string s. */
    static int ends(struct token_details * z, const char * s);


    /* setto(s) sets (j+1),...k to the characters in the string s, readjusting k. */
    static void setto(struct token_details * z, const char * s);


    static void r(struct token_details * z, const char * s);


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
    static void step1ab(struct token_details * z);

    /* step1c() turns terminal y to i when there is another vowel in the stem. */
    static void step1c(struct token_details * z);


    /* step2() maps double suffices to single ones. so -ization ( = -ize plus
       -ation) maps to -ize etc. note that the string before the suffix must give
       m() > 0.
    */
    static void step2(struct token_details * z) ;


    /* step3() deals with -ic-, -full, -ness etc. similar strategy to step2. */
    static void step3(struct token_details * z);

    /* step4() takes off -ant, -ence etc., in context <c>vcvc<v>. */
    static void step4(struct token_details * z);


    /* step5() removes a final -e if m() > 1, and changes -ll to -l if m() > 1. */
    static void step5(struct token_details * z);

};


class Stemmer
{
public:

    Stemmer() {}; // Default constructor for boost serialization

    Stemmer(StemmerNormalizerFlagType stemmerFlag, const std::string &indexDirectory);

    /* searchHeadWords() search if the given string is present in the HeadWordsDictionary */
    int searchHeadWords(const std::string &search) const;

    /* input : Token to be stemmed  Output : StemmedToken */
    std::string stemToken(const std::string &token) const;

    virtual ~Stemmer() ;

private:

    /* create the HeadWordsMap from the file of headwords.
     */
    int createHeadWordsMap(const std::string &indexDirectory);
    std::map<std::string,int> headwords;

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & headwords;
    }
};


}}


#endif /* __CORE_ANALYZER__STEMMER_H__ */
