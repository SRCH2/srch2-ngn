// $Id: Normalizer.h 3294 2013-05-01 03:45:51Z jiaying $
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

#ifndef NORMALIZER_H_
#define NORMALIZER_H_

#include <instantsearch/platform.h>
#include <instantsearch/Analyzer.h>

#include <cstdlib>
#include <cstdio>
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

//typedef std::tr1::unordered_map<std::string, std::string> MyMap;
typedef std::map<std::string, std::string> MyMap;

class Normalizer
{
public:

    Normalizer() {};  // Default constructor for boost serialization
    Normalizer(StemmerNormalizerType stemNormType, const std::string &indexDirectory);
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

