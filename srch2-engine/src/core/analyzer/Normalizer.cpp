// $Id: Normalizer.cpp 3074 2012-12-06 22:26:36Z oliverax $

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

#include "Normalizer.h"
#include <string>

using std::string;

namespace srch2
{
namespace instantsearch
{

Normalizer::Normalizer(StemmerNormalizerType stemNormType, const std::string &indexDirectory)
{
    if (stemNormType != srch2::instantsearch::NO_STEMMER_NORMALIZER)
    {
        FILE *f;
        string filePath = indexDirectory + "NormalizerRules.txt";
        f = fopen(filePath.c_str(), "r");
        char preNorm[50];
        char postNorm[50];
        while(fscanf(f, "%s %s\n", preNorm, postNorm) == 2)
            walMartDictionary[std::string(preNorm)] = std::string(postNorm);
        fclose(f);
    }
}

Normalizer::~Normalizer()
{
    this->walMartDictionary.clear();
}

/*
 *normalize()
 *Input : vector of tokens
 *Output : vector of tokens with normalized tokens added at the end of the input vector
 */
void Normalizer::normalize(std::vector<std::string> &inputWords) const
{
    std::string currentLookup;
    unsigned i;
    if(inputWords.size()==0)
        return;

    unsigned input_size = inputWords.size();
    //std::cout<<"input_size: "<<input_size<<std::endl;
    for(i=0; i<input_size-1; i++)
    {
        currentLookup = inputWords[i];
        MyMap::const_iterator it = walMartDictionary.find(currentLookup);

        if(it != walMartDictionary.end())//match - only for the current word
        {
            //std::cout<<(*it).second<<std::endl;
            inputWords.push_back((*it).second);
        }

        else//for the current word and the following word
        {
            currentLookup = inputWords[i] + std::string("-") + inputWords[i+1];
            MyMap::const_iterator it = walMartDictionary.find(currentLookup);

            if(it != walMartDictionary.end())// match
            {
                //std::cout<<currentLookup<<std::endl;
                inputWords.push_back(currentLookup);
                //std::cout<<(*it).second<<std::endl;
                inputWords.push_back((*it).second);

            }
        }
    }
    currentLookup = inputWords[input_size-1]; // for the last word
    MyMap::const_iterator it = walMartDictionary.find(currentLookup);
    if(it != walMartDictionary.end())//match - only for the current word
    {
        inputWords.push_back((*it).second);

    }
}


}}


