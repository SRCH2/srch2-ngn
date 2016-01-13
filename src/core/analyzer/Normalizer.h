
#ifndef NORMALIZER_H_
#define NORMALIZER_H_

#include <instantsearch/platform.h>
#include <instantsearch/Analyzer.h>

#include <string>
#include <vector>
#include <map>

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

#include "util/encoding.h"

namespace srch2
{
namespace instantsearch
{

typedef std::map<std::string, std::string> MyMap;

class Normalizer
{
public:

    Normalizer() {};  // Default constructor for boost serialization
    Normalizer(StemmerNormalizerFlagType stemmerFlag, const std::string &indexDirectory);
    ~Normalizer();

    /*
     *normalize()
     *Input : vector of tokens
     *Output : vector of tokens with normalized tokens added at the end of the input vector
     */
    void normalize(std::vector<std::string> &input) const;

private:
    MyMap walMartDictionary;

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & walMartDictionary;
    }
};

}}
#endif

