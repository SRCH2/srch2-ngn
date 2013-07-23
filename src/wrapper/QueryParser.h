// $Id$ 07/11/13 Jamshid


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

 * Copyright �� 2013 SRCH2 Inc. All rights reserved
 */


#ifndef _WRAPPER_QUERYPARSER_H__
#define _WRAPPER_QUERYPARSER_H__





#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <iostream>
#include <sys/queue.h>
#include <event.h>
#include <evhttp.h>
#include <event2/http.h>


#include "Srch2ServerConf.h"
#include "instantsearch/Analyzer.h"
#include "instantsearch/Schema.h"
#include "ParsedParameterContainer.h"

namespace srch2is = srch2::instantsearch;
using srch2is::Schema;
using srch2is::Analyzer;

namespace srch2
{
namespace httpwrapper
{

class QueryParser
{
public:

	QueryParser(const evkeyvalq &headers,
			ParsedParameterContainer * container);


	// parses the URL to a query object , fills out the container
	void parse();




private:


	ParsedParameterContainer * container;
	const evkeyvalq & headers;



	// constants used by this class
	static const char* const fieldListDelimiter;

	static const char* const fieldListParamName;
	static const char* const debugControlParamName;
	static const char* const debugParamName;
	static const char* const startParamName;
	static const char* const rowsParamName;
	static const char* const timeAllowedParamName;



	// TODO: change the prototypes to reflect input/outputs
	/*
	 *
	 * input:
	 * 		1. the key value map
	 * 		2. the query string: q={key=val key2=val2}field1:keyword1 AND keyword2~2.5
	 *
	 * output:
	 * 		1. It fills up the queryHelper object
	 */
	void mainQueryParser();


	/*
	 * this function looks to see if there is a debug key/value pairs in headers
	 * input:
	 * 		1. the key value map
	 * outpu:
	 * 		2. fills up the helper
	 */
	void debugQueryParser();

	/*
	 * it looks to see if we have a field list (fl=field1,field2,field3 or fl=*)
	 * if we have field list it fills up the helper accordingly
	 */
	void fieldListParser();



	/*
	 * it looks to see if we have a start offset (start=)
	 * if we have start offset it fills up the helper accordingly
	 */
	void startOffsetParameterParser();

	/*
	 * it looks to see if we have a number of results (start=)
	 * if we have number of results it fills up the helper accordingly
	 */
	void numberOfResultsParser();



	/*
	 * it looks to see if we have a time limitation
	 * if we have time limitation it fills up the helper accordingly
	 */
	void timeAllowedParameterParser();




	/*
	 * it looks to see if we have a omit header
	 * if we have omit header it fills up the helper accordingly.
	 */
	void omitHeaderParameterParser();


	/*
	 * it looks to see if we have a responce type
	 * if we have reponce type it fills up the helper accordingly.
	 */
	void responseWriteTypeParameterParser();



	/*
	 * it looks to see if there is any post processing filter
	 * if there is then it fills up the helper accordingly
	 */
	void filterQueryParameterParser();


	/*
	 * parses the parameters facet=true/false , and it is true it parses the rest of
	 * parameters which are related to faceted search.
	 */
	void facetParser();


	/*
	 * looks for the parameter sort which defines the post processing sort job
	 */
	void sortParser();



	/*
	 * this function parses the local parameters section of all parts
	 * input:
	 * 		1. local parameters string : {key=val key2=val2}
	 * output:
	 * 		1. it fills up the metadata of the queryHelper object
	 */
	void localParameterParser();



	/*
	 * this function parses the keyword string for the boolean operators, boost information, fuzzy flags ...
	 * input: keyword string : keyword1 AND keyword2~2.5
	 * output: fills up the helper
	 */
	void keywordParser();



	/*
	 * this function parsers only the parameters which are specific to Top-K
	 */
	void topKParameterParser();

	/*
	 * this function parsers only the parameters which are specific to get all results search type
	 */
	void getAllResultsParser();


	/*
	 * this function parsers the parameters related to geo search like latitude and longitude .
	 */
	void getGeoParser();

};



}
}

#endif //_WRAPPER_QUERYPARSER_H__
