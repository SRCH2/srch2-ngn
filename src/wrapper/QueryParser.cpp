
#include "QueryParser.h"
#include "ParserUtility.h"


namespace srch2
{
namespace instantsearch
{


QueryParser::QueryParser(const Analyzer *analyzer,
		const srch2::httpwrapper::Srch2ServerConf *indexDataContainerConf,
		const Schema *schema,
		const evkeyvalq &headers,
		ParsedParameterContainer * container){

	this->analyzer = analyzer;
	this->indexDataContainerConf = indexDataContainerConf;
	this->schema = schema;
	this->container = container;

}


// parses the URL to a query object
void parse(){


	/*
	 *  We have all the header information here which is the pairs of query parameters
	 * 1. call the main query parser : mainQueryParser();
	 * 2. call the debug parser: debugQueryParser();
	 * 3. call the field list parser : fieldListParser();
	 * 4. call the start parser: startOffsetParameterParser();
	 * 5. call the row parser: numberOfResultsParser();
	 * 6. call the time allowed parser: timeAllowedParameterParser();
	 * 7. call the omit header parser : omitHeaderParameterParser();
	 * 8. call the response writer parser : responseWriteTypeParameterParser();
	 * 9. call the filter query parser : filterQueryParameterParser();
	 * TODO : see what we should do about similarityBoost and lengthBoost
	 * 10. based on the value of search type (if it's defined in local parameters we take it
	 *    otherwise we get it from conf file) leave the rest of parsing to one of the parsers
	 * 10.1: search type : Top-K
	 * 		call topKParameterParser();
	 * 10.2: search type : All results
	 * 		call getAllResultsParser();
	 * 10.3: search type : GEO
	 * 		call getGeoParser();
	 */

	// do some parsing

}


void QueryParser::mainQueryParser(){ // TODO: change the prototype to reflect input/outputs

	/*
	 * 1. Parse the string and find
	 * 		1.1 local parameters
	 * 		1.2 keyword string
	 * 2. calls localParameterParser()
	 * 3. calls the keywordParser();
	 */
}


void QueryParser::debugQueryParser(){

}



void QueryParser::fieldListParser(){
	/*
	 * if there is a field list query parameter
	 * then parse it and fill the helper up
	 */
}

void QueryParser::debugQueryParser(){

	/*
	 * if there is a debug query parameter
	 * then parse it and fill the helper up
	 */
}


void QueryParser::startOffsetParameterParser(){
	/*
	 * if there is a start offset
	 * fills the helper up
	 */
}


void QueryParser::numberOfResultsParser(){
	/*
	 * if there is a number of results
	 * fills the helper up
	 */
}


void QueryParser::timeAllowedParameterParser(){
	/*
	 * it looks to see if we have a time limitation
	 * if we have time limitation it fills up the helper accordingly
	 */
}


void QueryParser::omitHeaderParameterParser(){
	/*
	 * it looks to see if we have a omit header
	 * if we have omit header it fills up the helper accordingly.
	 */
}


void QueryParser::responseWriteTypeParameterParser(){
	/*
	 * it looks to see if we have a responce type
	 * if we have reponce type it fills up the helper accordingly.
	 */
}



void QueryParser::filterQueryParameterParser(){
	/*
	 * it looks to see if there is any post processing filter
	 * if there is then it fills up the helper accordingly
	 */
}


void QueryParser::facetParser(){
	/*
	 * parses the parameters facet=true/false , and it is true it parses the rest of
	 * parameters which are related to faceted search.
	 */

}


void QueryParser::sortParser(){
	/*
	 * looks for the parameter sort which defines the post processing sort job
	 */
}

void QueryParser::localParameterParser(){
	/*
	 * based on context it extract the key/value pairs and puts them in the helper.
	 */
}


void QueryParser::keywordParser(){
	/*
	 * parses the keyword string and fills up the helper by term information
	 */
}


void QueryParser::topKParameterParser(){
	/*
	 * this function parsers only the parameters which are specific to Top-K
	 */
}


void QueryParser::getAllResultsParser(){
	/*
	 * this function parsers only the parameters which are specific to get all results search type
	 * 1. also calls the facet parser. : facetParser();
	 * 2. also calls the sort parser: sortParser();
	 * TODO: do we need to get any other parameters here ?
	 */

}



void QueryParser::getGeoParser(){
	/*
	 * this function parsers the parameters related to geo search like latitude and longitude .
	 * 1. also calls the facet parser. : facetParser();
	 * 2. also calls the sort parser: sortParser();
	 * 3. parses the geo parameters like leftBottomLatitude,leftBottomLongitude,rightTopLatitude,rightTopLongitude
	 * 		centerLatitude,centerLongitude,radius
	 * 4. Based on what group of geo parameters are present it sets geoType to CIRCULAR or RECTANGULAR

}

// creates a post processing plan based on information from Query
// TODO : query should be removed from the inputs of this function. This function should only return plan based on header
ResultsPostProcessorPlan * QueryParser::createPostProcessingPlan(const evkeyvalq &headers,Query * query){

	ResultsPostProcessorPlan * result = NULL;
	if(query->getPostProcessingFilter() == RANGE_CHECK){
		result = new ResultsPostProcessorPlan();
		result->addFilterToPlan(new RangeQueryFilter());
	}else if(query->getPostProcessingFilter() == NO_FILTER){
		// TODO : must create plan based on query
		result = new ResultsPostProcessorPlan();
	}

	return result;


}



}
}
