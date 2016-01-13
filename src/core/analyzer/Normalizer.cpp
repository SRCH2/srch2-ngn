/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Normalizer.h"
#include <cstdio>
#include <string>

using std::string;

namespace srch2
{
namespace instantsearch
{

Normalizer::Normalizer(StemmerNormalizerFlagType stemmerFlag, const std::string &indexDirectory)
{
    if (stemmerFlag != srch2::instantsearch::DISABLE_STEMMER_NORMALIZER)
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
    for(i=0; i<input_size-1; i++)
    {
        currentLookup = inputWords[i];
        MyMap::const_iterator it = walMartDictionary.find(currentLookup);

        if(it != walMartDictionary.end())//match - only for the current word
        {
            inputWords.push_back((*it).second);
        }

        else//for the current word and the following word
        {
            currentLookup = inputWords[i] + std::string("-") + inputWords[i+1];
            MyMap::const_iterator it = walMartDictionary.find(currentLookup);

            if(it != walMartDictionary.end())// match
            {
                inputWords.push_back(currentLookup);
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


