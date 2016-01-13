
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


