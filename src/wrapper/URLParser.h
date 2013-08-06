//$Id: URLParser.h 3456 2013-06-14 02:11:13Z jiaying $

#ifndef _URLPARSER_H_
#define _URLPARSER_H_

#include <vector>
#include <string>
#include <map>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <iostream>
#include <sys/queue.h>
#include <event.h>
#include <evhttp.h>
#include <event2/http.h>


#include <instantsearch/Term.h>
#include <instantsearch/Query.h>
#include <instantsearch/Analyzer.h>
#include "Srch2ServerConf.h"

using std::string;
using std::stringstream;

namespace srch2is = srch2::instantsearch;

namespace srch2
{
namespace httpwrapper
{

typedef std::map<std::string,std::string> URLParamsPair; //params;

struct URLParserHelper
{
    stringstream parserErrorMessage;
    bool parserSuccess;
    int searchType;
    int offset;
    int resultsToRetrieve;
    bool isFuzzy;

    // only applicable for searchType=1: GetAllResultsQuery
    unsigned sortby;
    srch2::instantsearch::SortOrder order;

    URLParserHelper()
    {
        parserSuccess = false;
        searchType = 0;
        isFuzzy = true;
    }

/*  void print() const
    {
        std::cout
            << "|parserSuccess|" << parserSuccess
            << "|searchType|" << searchType
            << "|offset|" << offset
            << "|resultsToRetrieve|" << resultsToRetrieve
            << "|isFuzzy|" << isFuzzy
            << std::endl;
    }*/
};

class URLParser
{
public:
    //static URLParamsPair* parse(const char *url, string &cleanURL, bool &urlParsingSucess);

    static const char queryDelimiter;
    static const char filterDelimiter;
    static const char fieldsAndDelimiter;
    static const char fieldsOrDelimiter;

    static const char* const searchTypeParamName;
    static const char* const keywordsParamName;
    static const char* const termTypesParamName;
    static const char* const termBoostsParamName;
    static const char* const fuzzyQueryParamName;
    static const char* const similarityBoostsParamName;
    static const char* const resultsToRetrieveStartParamName;
    static const char* const resultsToRetrieveLimitParamName;
    static const char* const attributeToSortParamName;
    static const char* const orderParamName;
    static const char* const lengthBoostParamName;
    static const char* const jsonpCallBackName;

    static const char* const leftBottomLatitudeParamName;
    static const char* const leftBottomLongitudeParamName;
    static const char* const rightTopLatitudeParamName;
    static const char* const rightTopLongitudeParamName;

    static const char* const centerLatitudeParamName;
    static const char* const centerLongitudeParamName;
    static const char* const radiusParamName;
/*
private:
    static char from_hex(char ch);
    static char *url_decode(const char *str);
    static void url_decode_string(const char* p, string &cleanURL);*/
};

class URLToDoubleQuery
{
    public:
        srch2is::Query *exactQuery;
        srch2is::Query *fuzzyQuery;

        URLToDoubleQuery(const evkeyvalq &headers, const srch2is::Analyzer *analyzer, const Srch2ServerConf *indexDataContainerConf, const srch2is::Schema *schema, URLParserHelper &urlParserHelper);

        ~URLToDoubleQuery();
    private:

};

}
}


#endif //_URLPARSER_H_
