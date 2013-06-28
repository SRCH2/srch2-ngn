// $Id: StemmerInternal.cpp 3074 2012-12-06 22:26:36Z oliverax $

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

#include "StemmerInternal.h"

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;

namespace srch2
{
namespace instantsearch
{

Stemmer::Stemmer(StemmerNormalizerType stemNormType, const std::string &indexDirectory)
{
    if ( stemNormType == srch2::instantsearch::STEMMER_NORMALIZER)
        this->createHeadWordsMap(indexDirectory);
}

/* create the HeadWordsMap from the file of headwords.
 */
int Stemmer::createHeadWordsMap(const std::string &indexDirectory)
{
    string filePath = indexDirectory + "StemmerHeadwords.txt";
    std::string str;
    std::ifstream input(filePath.c_str());
    if(input.fail())
    {
        cerr<<"\nThe file could not be opened.";
        return -1;
    }

    while(getline(input,str))
    {
        this->headwords.insert(make_pair(str,1));
    }
    return 0;
}


Stemmer::~Stemmer()
{

}

/* searchHeadWords() search if the given string is present in the HeadWordsDictionary */
int Stemmer::searchHeadWords(const std::string &search) const
{
    std::map<std::string,int>::const_iterator iter=this->headwords.begin();
    iter=this->headwords.find(search);
    if(iter!=this->headwords.end())
    {
        //cout<<"Value is"<<iter->second;
        return TRUE;
    }
    else
    {
        //cout<<"Key is not available";
        return FALSE;
    }

}

/* input : Token to be stemmed  Output : StemmedToken */
std::string Stemmer::stemToken(const std::string &token)const
{
    if (searchHeadWords(token))
        return token;

    struct token_details * z = StemmerInternal::create_stemmer();
    int tok_length = token.length();
    int result = StemmerInternal::stem(z,token,tok_length-1);
    std::string output_token = token.substr(0,result+1);
    StemmerInternal::free_stemmer(z);
    return output_token;
}


/***********************************************************************************
 * StemmerInternal Function http://tartarus.org/~martin/PorterStemmer/
 */

/* cons(i) is TRUE <=> b[i] is a consonant. */
int StemmerInternal::cons(struct token_details * z, int i)
{
    switch (z->b[i])

   {  case 'a': case 'e': case 'i': case 'o': case 'u': return FALSE;
      case 'y': return (i == 0) ? TRUE : !cons(z, i - 1);
      default: return TRUE;
   }
}

/* m() measures the number of consonant sequences between k0 and j. if c is
       a consonant sequence and v a vowel sequence, and <..> indicates arbitrary
       presence,

          <c><v>       gives 0
          <c>vc<v>     gives 1
          <c>vcvc<v>   gives 2
          <c>vcvcvc<v> gives 3
          ....
*/

int StemmerInternal::m(struct token_details * z)
{  int n = 0;
   int i = 0;
   int j = z->j;
   while(TRUE)
   {  if (i > j) return n;
      if (! cons(z, i)) break; i++;
   }
   i++;
   while(TRUE)
   {  while(TRUE)
      {  if (i > j) return n;
            if (cons(z, i)) break;
            i++;
      }
      i++;
      n++;
      while(TRUE)
      {  if (i > j) return n;
         if (! cons(z, i)) break;
         i++;
      }
      i++;
   }
}


/* vowelinstem() is TRUE <=> k0,...j contains a vowel */
int StemmerInternal::vowelinstem(struct token_details * z)
{
   int j = z->j;
   int i; for (i = 0; i <= j; i++) if (! cons(z, i)) return TRUE;
   return FALSE;
}


/* doublec(j) is TRUE <=> j,(j-1) contain a double consonant. */
int StemmerInternal::doublec(struct token_details * z, int j)
{
   char * b = z->b;
   if (j < 1) return FALSE;
   if (b[j] != b[j - 1]) return FALSE;
   return cons(z, j);
}



/* cvc(i) is TRUE <=> i-2,i-1,i has the form consonant - vowel - consonant
       and also if the second c is not w,x or y. this is used when trying to
       restore an e at the end of a short word. e.g.

          cav(e), lov(e), hop(e), crim(e), but
          snow, box, tray.

*/
int StemmerInternal::cvc(struct token_details * z, int i)
{  if (i < 2 || !cons(z, i) || cons(z, i - 1) || !cons(z, i - 2)) return FALSE;
   {  int ch = z->b[i];
      if (ch  == 'w' || ch == 'x' || ch == 'y') return FALSE;
   }
   return TRUE;
}


/* ends(s) is TRUE <=> k0,...k ends with the string s. */
int StemmerInternal::ends(struct token_details * z, const char * s)
{  int length = s[0];
   char * b = z->b;
   int k = z->k;
   if (s[length] != b[k]) return FALSE; /* tiny speed-up */
   if (length > k + 1) return FALSE;
   if (memcmp(b + k - length + 1, s + 1, length) != 0) return FALSE;
   z->j = k-length;
   return TRUE;
}


/* setto(s) sets (j+1),...k to the characters in the string s, readjusting k. */
void StemmerInternal::setto(struct token_details * z, const char * s)
{  int length = s[0];
   int j = z->j;
   memmove(z->b + j + 1, s + 1, length);
   z->k = j+length;
}

void StemmerInternal::r(struct token_details * z, const char * s)
{
    if (m(z) > 0)
        setto(z, s);
}




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

void StemmerInternal::step1ab(struct token_details * z)
{
   char * b = z->b;
   if (b[z->k] == 's')
   {
       if (ends(z, "\04" "sses"))
           z->k -= 2;
       else if (ends(z, "\03" "ies"))
           setto(z, "\01" "i");
       else if (b[z->k - 1] != 's')
           z->k--;
   }
   if (ends(z, "\03" "eed"))
   {
       if (m(z) > 0)
           z->k--;
   }
   else if ((ends(z, "\02" "ed") || ends(z, "\03" "ing")) && vowelinstem(z))
   {
       z->k = z->j;
      if (ends(z, "\02" "at"))
          setto(z, "\03" "ate");
      else if (ends(z, "\02" "bl")) setto(z, "\03" "ble");
      else if (ends(z, "\02" "iz")) setto(z, "\03" "ize");
      else if (doublec(z, z->k))
      {  z->k--;
         {  int ch = b[z->k];
            if (ch == 'l' || ch == 's' || ch == 'z') z->k++;
         }
      }
      else if (m(z) == 1 && cvc(z, z->k)) setto(z, "\01" "e");
   }
}


/* step1c() turns terminal y to i when there is another vowel in the stem. */
void StemmerInternal::step1c(struct token_details * z)
{
   if (ends(z, "\01" "y") && vowelinstem(z))
       z->b[z->k] = 'i';
}


/* step2() maps double suffices to single ones. so -ization ( = -ize plus
       -ation) maps to -ize etc. note that the string before the suffix must give
       m() > 0.
*/
void StemmerInternal::step2(struct token_details * z)
{
    switch (z->b[z->k-1])

    {
       case 'a': if (ends(z, "\07" "ational")) { r(z, "\03" "ate"); break; }
                 if (ends(z, "\06" "tional")) { r(z, "\04" "tion"); break; }
                 break;
       case 'c': if (ends(z, "\04" "enci")) { r(z, "\04" "ence"); break; }
                 if (ends(z, "\04" "anci")) { r(z, "\04" "ance"); break; }
                 break;
       case 'e': if (ends(z, "\04" "izer")) { r(z, "\03" "ize"); break; }
                 break;
       case 'l': if (ends(z, "\03" "bli")) { r(z, "\03" "ble"); break; } /*-DEPARTURE-*/

     /* To match the published algorithm, replace this line with
        case 'l': if (ends(z, "\04" "abli")) { r(z, "\04" "able"); break; } */

                 if (ends(z, "\04" "alli")) { r(z, "\02" "al"); break; }
                 if (ends(z, "\05" "entli")) { r(z, "\03" "ent"); break; }
                 if (ends(z, "\03" "eli")) { r(z, "\01" "e"); break; }
                 if (ends(z, "\05" "ousli")) { r(z, "\03" "ous"); break; }
                 break;
       case 'o': if (ends(z, "\07" "ization")) { r(z, "\03" "ize"); break; }
                 if (ends(z, "\05" "ation")) { r(z, "\03" "ate"); break; }
                 if (ends(z, "\04" "ator")) { r(z, "\03" "ate"); break; }
                 break;
       case 's': if (ends(z, "\05" "alism")) { r(z, "\02" "al"); break; }
                 if (ends(z, "\07" "iveness")) { r(z, "\03" "ive"); break; }
                 if (ends(z, "\07" "fulness")) { r(z, "\03" "ful"); break; }
                 if (ends(z, "\07" "ousness")) { r(z, "\03" "ous"); break; }
                 break;
       case 't': if (ends(z, "\05" "aliti")) { r(z, "\02" "al"); break; }
                 if (ends(z, "\05" "iviti")) { r(z, "\03" "ive"); break; }
                 if (ends(z, "\06" "biliti")) { r(z, "\03" "ble"); break; }
                 break;
       case 'g': if (ends(z, "\04" "logi")) { r(z, "\03" "log"); break; } /*-DEPARTURE-*/


    }
}


/* step3() deals with -ic-, -full, -ness etc. similar strategy to step2. */
void StemmerInternal::step3(struct token_details * z)
{
    switch (z->b[z->k])

    {
       case 'e': if (ends(z, "\05" "icate")) { r(z, "\02" "ic"); break; }
                 if (ends(z, "\05" "ative")) { r(z, "\00" ""); break; }
                 if (ends(z, "\05" "alize")) { r(z, "\02" "al"); break; }
                 break;
       case 'i': if (ends(z, "\05" "iciti")) { r(z, "\02" "ic"); break; }
                 break;
       case 'l': if (ends(z, "\04" "ical")) { r(z, "\02" "ic"); break; }
                 if (ends(z, "\03" "ful")) { r(z, "\00" ""); break; }
                 break;
       case 's': if (ends(z, "\04" "ness")) { r(z, "\00" ""); break; }
                 break;
    }
}


/* step4() takes off -ant, -ence etc., in context <c>vcvc<v>. */
void StemmerInternal::step4(struct token_details * z)
{
    switch (z->b[z->k-1])

  {
         case 'a': if (ends(z, "\02" "al")) break; return;

         case 'c': if (ends(z, "\04" "ance")) break;
                   if (ends(z, "\04" "ence")) break; return;
         case 'e': if (ends(z, "\02" "er")) break; return;
         case 'i': if (ends(z, "\02" "ic")) break; return;
         case 'l': if (ends(z, "\04" "able")) break;
                   if (ends(z, "\04" "ible")) break; return;
         case 'n': if (ends(z, "\03" "ant")) break;
                   if (ends(z, "\05" "ement")) break;
                   if (ends(z, "\04" "ment")) break;
                   if (ends(z, "\03" "ent")) break; return;
         case 'o': if (ends(z, "\03" "ion") && (z->b[z->j] == 's' || z->b[z->j] == 't')) break;
                   if (ends(z, "\02" "ou")) break; return;
                   /* takes care of -ous */
         case 's': if (ends(z, "\03" "ism")) break; return;
         case 't': if (ends(z, "\03" "ate")) break;
                   if (ends(z, "\03" "iti")) break; return;
         case 'u': if (ends(z, "\03" "ous")) break; return;
         case 'v': if (ends(z, "\03" "ive")) break; return;
         case 'z': if (ends(z, "\03" "ize")) break; return;
         default: return;
  }
      if (m(z) > 1)
          z->k = z->j;
}


/* step5() removes a final -e if m() > 1, and changes -ll to -l if m() > 1. */
void StemmerInternal::step5(struct token_details * z)
{
  char * b = z->b;
  z->j = z->k;
  if (b[z->k] == 'e')
  {
      int a = m(z);
      if ((a > 1 || a == 1) && (!cvc(z, z->k - 1)))
          z->k--;
  }
  if (b[z->k] == 'l' && doublec(z, z->k) && m(z) > 1)
      z->k--;
}

int StemmerInternal::stem(struct token_details * z, std::string token, int k)
{
    char *b=new char[token.size()+1];
    b[token.size()]=0;
    memcpy(b,token.c_str(),token.size());

//    const char * c = token.c_str();
//    char * b = NULL;
//    strcpy(b,c);

    if (k <= 1)
       return k; /*-DEPARTURE-*/
    z->b = b;
    z->k = k; /* copy the parameters into z */

   /* With this line, strings of length 1 or 2 don't go through the
      stemming process, although no mention is made of this in the
      published algorithm.
   */

   step1ab(z);
   step1c(z);
   step2(z);
   step3(z);
   step4(z);
   step5(z);
   return z->k;
}

}}
