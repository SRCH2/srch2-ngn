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

 * Copyright © 2013 SRCH2 Inc. All rights reserved
 */


#ifndef _WRAPPER_QUERYBUILDER_H__
#define _WRAPPER_QUERYBUILDER_H__





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
#include "instantsearch/Query.h"
#include "postprocessing/ResultsPostProcessor.h"



namespace srch2
{
namespace instantsearch
{

class QueryBuilder
{
public:


	QueryBuilder(const Analyzer *analyzer,
			const srch2::httpwrapper::Srch2ServerConf *indexDataContainerConf,
			const Schema *schema);


	// parses the URL to a query object
	void parse(const evkeyvalq &headers,Query * query);

	// creates a post processing plan based on information from Query
	ResultsPostProcessorPlan * createPostProcessingPlan(const evkeyvalq &headers,Query * query);



private:


	const Analyzer *analyzer;
	const srch2::httpwrapper::Srch2ServerConf *indexDataContainerConf;
	const Schema *schema;




};



}
}

#endif
