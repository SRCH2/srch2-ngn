//$Id: URLParser.cpp 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

#include "URLParser.h"

using namespace srch2::instantsearch;
using srch2::instantsearch::Query;

#define SEARCH_TYPE_OF_RANGE_QUERY_WITHOUT_KEYWORDS 2
namespace srch2
{
namespace httpwrapper
{


//for search
const char URLParser::queryDelimiter = '+';
const char URLParser::filterDelimiter = ':';
const char URLParser::fieldsAndDelimiter = ',';
const char URLParser::fieldsOrDelimiter = '.';

const char* const URLParser::searchTypeParamName = "type";
const char* const URLParser::keywordsParamName = "q";
const char* const URLParser::termTypesParamName = "termtypes";
const char* const URLParser::termBoostsParamName = "termboosts";
const char* const URLParser::fuzzyQueryParamName = "fuzzy";
const char* const URLParser::similarityBoostsParamName = "simboost";
const char* const URLParser::resultsToRetrieveStartParamName = "start";
const char* const URLParser::resultsToRetrieveLimitParamName = "limit";
const char* const URLParser::attributeToSortParamName = "sortby";
const char* const URLParser::orderParamName = "order";
const char* const URLParser::lengthBoostParamName = "lengthboost";
const char* const URLParser::jsonpCallBackName = "callback";

const char* const URLParser::leftBottomLatitudeParamName = "lb_lat";
const char* const URLParser::leftBottomLongitudeParamName = "lb_lng";
const char* const URLParser::rightTopLatitudeParamName = "rt_lat";
const char* const URLParser::rightTopLongitudeParamName = "rt_lng";

const char* const URLParser::centerLatitudeParamName = "ct_lat";
const char* const URLParser::centerLongitudeParamName = "ct_lng";
const char* const URLParser::radiusParamName = "radius";

// Schema will be used in Attribute-based search to set attribute bitmap.
URLToDoubleQuery::URLToDoubleQuery(const evkeyvalq &headers, const Analyzer *analyzer, const Srch2ServerConf *indexDataContainerConf, const Schema *schema, URLParserHelper &urlParserHelper)
{
    this->exactQuery = NULL;
    this->fuzzyQuery = NULL;

    //first check range query without keywords: (1)searchType=SEARCH_TYPE_OF_RANGE_QUERY_WITHOUT_KEYWORDS;(2)keywords is null;(3)Attribute Latitude and Longitude are not "IGNORE"
    if (indexDataContainerConf->getSearchType() == SEARCH_TYPE_OF_RANGE_QUERY_WITHOUT_KEYWORDS){//check if the query is a range query but without information

		 const char *keywordsParamName = evhttp_find_header(&headers, URLParser::keywordsParamName);
		 if (!keywordsParamName){//check if keyword information is empty
			 if(indexDataContainerConf->getAttributeLatitude()!="IGNORE"&&indexDataContainerConf->getAttributeLongitude()!="IGNORE"){// if the Attribute Latitude and Longitude are not IGNORE, we say query contains the range
				urlParserHelper.parserSuccess=true;
				urlParserHelper.searchType = 2;
				//set the position which the results starts from to retrieve
				{
					const char *resultsToRetrieveStartParamName = evhttp_find_header(&headers, URLParser::resultsToRetrieveStartParamName);
					if (resultsToRetrieveStartParamName){
						size_t sz;
						char *resultsToRetrieveStartParamName_cstar = evhttp_uridecode(resultsToRetrieveStartParamName, 0, &sz);
						urlParserHelper.offset = atoi(resultsToRetrieveStartParamName_cstar);
						delete resultsToRetrieveStartParamName_cstar;
					}
					else{//default value is 0
						urlParserHelper.offset = 0;
					}
				}

				//get the limited number of results to retrieval
				{
					const char *resultsToRetrieveLimitParamName = evhttp_find_header(&headers, URLParser::resultsToRetrieveLimitParamName);
					if (resultsToRetrieveLimitParamName){
						size_t sz;
						char *resultsToRetrieveLimitParamName_cstar = evhttp_uridecode(resultsToRetrieveLimitParamName, 0, &sz);
						urlParserHelper.resultsToRetrieve = atoi(resultsToRetrieveLimitParamName_cstar);
						delete resultsToRetrieveLimitParamName_cstar;
					}
					else{
						urlParserHelper.resultsToRetrieve = indexDataContainerConf->getDefaultResultsToRetrieve();
					}
				}
				//use the exactQuery to construct the query for the range query without keywords
				//for this query, only need to set the range of query
				this->exactQuery = new Query(MapQuery);
				//set the rectangle range
				const char *paramIter_latLB = evhttp_find_header(&headers, URLParser::leftBottomLatitudeParamName);
				const char *paramIter_lngLB = evhttp_find_header(&headers, URLParser::leftBottomLongitudeParamName);
				const char *paramIter_latRT = evhttp_find_header(&headers, URLParser::rightTopLatitudeParamName);
				const char *paramIter_lngRT = evhttp_find_header(&headers, URLParser::rightTopLongitudeParamName);
				//set the circle range
				const char *paramIter_latCT = evhttp_find_header(&headers, URLParser::centerLatitudeParamName);
				const char *paramIter_lngCT = evhttp_find_header(&headers, URLParser::centerLongitudeParamName);
				const char *paramIter_radius= evhttp_find_header(&headers, URLParser::radiusParamName);

				//check the parameters of rectangle are not null and set rectangle range
				if ( paramIter_latLB && paramIter_lngLB
					 && paramIter_latRT && paramIter_lngRT ){
					size_t sz1;
					char *paramIter_latLB_cstar = evhttp_uridecode(paramIter_latLB, 0, &sz1);
					double range_min_x = atof(paramIter_latLB_cstar);
					delete paramIter_latLB_cstar;

					size_t sz2;
					char *paramIter_lngLB_cstar = evhttp_uridecode(paramIter_lngLB, 0, &sz2);
					double range_min_y = atof(paramIter_lngLB_cstar);
					delete paramIter_lngLB_cstar;

					size_t sz3;
					char *paramIter_latRT_cstar = evhttp_uridecode(paramIter_latRT, 0, &sz3);
					double range_max_x = atof(paramIter_latRT_cstar);
					delete paramIter_latRT_cstar;

					size_t sz4;
					char *paramIter_lngRT_cstar = evhttp_uridecode(paramIter_lngRT, 0, &sz4);
					double range_max_y = atof(paramIter_lngRT_cstar);
					delete paramIter_lngRT_cstar;

					exactQuery->setRange(range_min_x, range_min_y, range_max_x, range_max_y);

				}
				else if (paramIter_latCT && paramIter_lngCT && paramIter_radius){//check the parameters of circle are not null and set the circle range
					size_t sz1;
					char *paramIter_latCT_cstar = evhttp_uridecode(paramIter_latCT, 0, &sz1);
					double x = atof(paramIter_latCT_cstar);
					delete paramIter_latCT_cstar;

					size_t sz2;
					char *paramIter_lngCT_cstar = evhttp_uridecode(paramIter_lngCT, 0, &sz2);
					double y = atof(paramIter_lngCT_cstar);
					delete paramIter_lngCT_cstar;

					size_t sz3;
					char *paramIter_radius_cstar = evhttp_uridecode(paramIter_radius, 0, &sz3);
					double r = atof(paramIter_radius_cstar);
					delete paramIter_radius_cstar;

					exactQuery->setRange(x, y, r);
				}
				else{
					urlParserHelper.parserSuccess=false;
				}
				return;
			 }

		 }

	}

    // the following code is constructing the query:(1)keywords only search;(2)keywords+geo
    {
        /*if(indexDataContainerConf->getSearchResponseJSONFormat() == 1)
        {
            const char *jsonpCallBack = evhttp_find_header(&headers, URLParser::jsonpCallBackName);
            if (jsonpCallBack)
            {
                char *jsonpCallBack_cstar = evhttp_decode_uri(jsonpCallBack);
                urlParserHelper.jsonpCallback = std::string(jsonpCallBack_cstar);
                delete jsonpCallBack_cstar;
            }
            else
            {
                urlParserHelper.parserSuccess = false;
                urlParserHelper.parserErrorMessage << "{\"error\":\"jsonp callback not given\"}";
                return;
            }
        }*/

        urlParserHelper.searchType = indexDataContainerConf->getSearchType(); //TopK: 0; Advanced: 1
        string sep;
        sep += URLParser::queryDelimiter;
        vector<string> queryKeywordVector;
        vector<TermType> termTypeVector;
        vector<string> termBoostsVector;
        vector<string> similarityBoostsVector;
        float lengthBoost;

        vector<unsigned> filters;

        {
            const char *keywordsParamName = evhttp_find_header(&headers, URLParser::keywordsParamName);
            if (keywordsParamName){
            	//cout << string(keywordsParamName)<<endl;
                size_t sz;
                char *keywordsParamName_cstar = evhttp_uridecode(keywordsParamName, 0, &sz);

                if (indexDataContainerConf->getSupportAttributeBasedSearch()){
                	// get searchable attribute to get map from attribute name to Id
                    analyzer->tokenizeQueryWithFilter(keywordsParamName_cstar, queryKeywordVector, URLParser::queryDelimiter,
                            URLParser::filterDelimiter, URLParser::fieldsAndDelimiter, URLParser::fieldsOrDelimiter,
                            schema->getSearchableAttribute(), filters);
                }
                else{
                    analyzer->tokenizeQuery(keywordsParamName_cstar, queryKeywordVector);
                    //check whether or not the last character is a whitespace (which is transformed from "+")
                    //For example, for a query "q=trus+", we will take "trus" as a complete term.
                    string query=string(keywordsParamName_cstar);

                    if(query.substr(query.length()-1, 1) == " "){
                    	queryKeywordVector.push_back(" ");
                    }
                    filters.assign(queryKeywordVector.size(), 1);
                }
                delete keywordsParamName_cstar;
            }
            else{
            	urlParserHelper.parserSuccess = false;
            	urlParserHelper.parserErrorMessage << "{\"error\":\"Empty search query\"}";
            	return;
            }

            
            if (queryKeywordVector.size() == 0){
                urlParserHelper.parserSuccess = false;
                urlParserHelper.parserErrorMessage << "{\"error\":\"Query keywords are malformed\"}";
                return;
            }
        
        }


        {

            const char *fuzzyQueryParamName = evhttp_find_header(&headers, URLParser::fuzzyQueryParamName);
            if (fuzzyQueryParamName){
                size_t sz;
                char *fuzzyQueryParamName_cstar = evhttp_uridecode(fuzzyQueryParamName, 0, &sz);

                std::string fuzzyQueryParamName_str = string(fuzzyQueryParamName_cstar);
                delete fuzzyQueryParamName_cstar;

                if (fuzzyQueryParamName_str.compare("1") == 0){
                    urlParserHelper.isFuzzy = true;
                }
                else if(fuzzyQueryParamName_str.compare("0") == 0){
                    urlParserHelper.isFuzzy = false;
                }
                else{
                    urlParserHelper.isFuzzy = indexDataContainerConf->getIsFuzzyTermsQuery();
                }

            }
            else{
                urlParserHelper.isFuzzy = indexDataContainerConf->getIsFuzzyTermsQuery();
            }
        }



        unsigned numberOfKeywords = queryKeywordVector.size();

        {
            const char *termTypesParamName = evhttp_find_header(&headers, URLParser::termTypesParamName);
            if (termTypesParamName){
                size_t sz;
                char *termTypesParamName_cstar = evhttp_uridecode(termTypesParamName, 0, &sz);

                std::string termTypesParamName_str = string(termTypesParamName_cstar);
                delete termTypesParamName_cstar;

                for (unsigned idx=0; idx<termTypesParamName_str.size(); idx++){
					if (termTypesParamName_str[idx] == '0'){
						termTypeVector.push_back(TERM_TYPE_PREFIX);
					}
					else if (termTypesParamName_str[idx] == '1')
						termTypeVector.push_back(TERM_TYPE_COMPLETE);
				}

                if (termTypeVector.size() != numberOfKeywords){
                    urlParserHelper.parserSuccess = false;
                    urlParserHelper.parserErrorMessage << "{\"error\":\"The number of TermTypes doesn't match the number of keywords\"}";
                    return;
                }


            }
            else{
                if (numberOfKeywords >= 1){
                    for (unsigned iter = 0; iter < numberOfKeywords - 1; iter++){
                        termTypeVector.push_back(TERM_TYPE_COMPLETE);
                    }
                    if(queryKeywordVector[numberOfKeywords - 1] != " "){
                    	if (!indexDataContainerConf->getQueryTermType())
                    		//The QueryTermType in the configure file decided the type of the last keyword only.
                    		termTypeVector.push_back(TERM_TYPE_PREFIX);
                    	else
                    		termTypeVector.push_back(TERM_TYPE_COMPLETE);
                    }
                    else{
                    	queryKeywordVector.pop_back();
                    	numberOfKeywords = numberOfKeywords - 1;
                    }

                }
            }
        }



        {
            const char *termBoostsParamName = evhttp_find_header(&headers, URLParser::termBoostsParamName);
            if (termBoostsParamName){
                size_t sz;
                char *termBoostsParamName_cstar = evhttp_uridecode(termBoostsParamName, 0, &sz);

                std::string termBoostsParamName_str = string(termBoostsParamName_cstar);
                delete termBoostsParamName_cstar;

                boost::split(termBoostsVector, termBoostsParamName_str, boost::is_any_of(sep));
                if (termBoostsVector.size() != numberOfKeywords){
                    urlParserHelper.parserSuccess = false;
                    urlParserHelper.parserErrorMessage << "{\"error\":\"The number of termBoosts doesn't match the number of keywords\"}";
                    return;
                }
            }
            else{
                //Raise error; //needed? ax
            }
        }



        {
            const char *lengthBoostParamName = evhttp_find_header(&headers, URLParser::lengthBoostParamName);
            if (lengthBoostParamName){
                size_t sz;
                char *lengthBoostParamName_cstar = evhttp_uridecode(lengthBoostParamName, 0, &sz);

                lengthBoost = atof(lengthBoostParamName_cstar);

                delete lengthBoostParamName_cstar;

            }
            else{
                lengthBoost = indexDataContainerConf->getQueryTermLengthBoost();
            }
        }



        {
            const char *similarityBoostsParamName = evhttp_find_header(&headers, URLParser::similarityBoostsParamName);
            if (similarityBoostsParamName){
                size_t sz;
                char *similarityBoostsParamName_cstar = evhttp_uridecode(similarityBoostsParamName, 0, &sz);
                string similarityBoostsParamName_str = string(similarityBoostsParamName_cstar);

                delete similarityBoostsParamName_cstar;

                boost::split(similarityBoostsVector, similarityBoostsParamName_str, boost::is_any_of(sep));
                if (similarityBoostsVector.size() != numberOfKeywords){
                    urlParserHelper.parserSuccess = false;
                    urlParserHelper.parserErrorMessage << "{\"error\":\"The number of similarityBoosts doesn't match the number of keywords\"}";
                    return;
                }
            }
            else
            {
                //Raise error; //needed? ax
            }
        }

        urlParserHelper.offset = 0; //default
        urlParserHelper.resultsToRetrieve = indexDataContainerConf->getDefaultResultsToRetrieve(); //atoi(values[5].c_str());
        int sortAttribute = indexDataContainerConf->getAttributeToSort(); //atoi(values[6].c_str());
        SortOrder order = (indexDataContainerConf->getOrdering()== 0) ? Ascending : Descending; //(atoi(values[7].c_str()) == 0) ? Ascending : Descending;

        {
            const char *searchTypeParamName = evhttp_find_header(&headers, URLParser::searchTypeParamName);
            if (searchTypeParamName){
                size_t sz;
                char *searchTypeParamName_cstar = evhttp_uridecode(searchTypeParamName, 0, &sz);
                string searchTypeParamName_str = string(searchTypeParamName_cstar);

                delete searchTypeParamName_cstar;

                if (searchTypeParamName_str.compare("0") != 0 && searchTypeParamName_str.compare("1") != 0 && searchTypeParamName_str.compare("2") != 0){
                    urlParserHelper.searchType = indexDataContainerConf->getSearchType();
                }
                else{
                    urlParserHelper.searchType = atoi(searchTypeParamName_str.c_str());

                    if (indexDataContainerConf->getNonSearchableAttributes()->size()==0 && urlParserHelper.searchType==1)
                        urlParserHelper.searchType = 0;
                }
            }
            else{
                urlParserHelper.searchType = indexDataContainerConf->getSearchType();
            }

            {
                const char *resultsToRetrieveStartParamName = evhttp_find_header(&headers, URLParser::resultsToRetrieveStartParamName);
                if (resultsToRetrieveStartParamName){
                    size_t sz;
                    char *resultsToRetrieveStartParamName_cstar = evhttp_uridecode(resultsToRetrieveStartParamName, 0, &sz);
                    urlParserHelper.offset = atoi(resultsToRetrieveStartParamName_cstar);

                    delete resultsToRetrieveStartParamName_cstar;
                }
                else{
                    urlParserHelper.offset = 0;
                }
            }


            {
                const char *resultsToRetrieveLimitParamName = evhttp_find_header(&headers, URLParser::resultsToRetrieveLimitParamName);
                if (resultsToRetrieveLimitParamName){
                    size_t sz;
                    char *resultsToRetrieveLimitParamName_cstar = evhttp_uridecode(resultsToRetrieveLimitParamName, 0, &sz);
                    urlParserHelper.resultsToRetrieve = atoi(resultsToRetrieveLimitParamName_cstar);

                    delete resultsToRetrieveLimitParamName_cstar;
                }
                else{
                    urlParserHelper.resultsToRetrieve = indexDataContainerConf->getDefaultResultsToRetrieve();
                }
            }
        }

        if (indexDataContainerConf->getSearchType() == 2){
            urlParserHelper.searchType = 2;
        }

        switch (urlParserHelper.searchType)
        {
            case 0:
                this->exactQuery = new Query(TopKQuery);
                this->fuzzyQuery = new Query(TopKQuery);
                break;

            case 1:
                {
                    const char *attributeToSortParamName = evhttp_find_header(&headers, URLParser::attributeToSortParamName);
                    if (attributeToSortParamName){
                        size_t sz;
                        char *attributeToSortParamName_cstar = evhttp_uridecode(attributeToSortParamName, 0, &sz);
                        sortAttribute = atoi(attributeToSortParamName_cstar);
                        
                        if (sortAttribute < 0 || sortAttribute > indexDataContainerConf->getNonSearchableAttributes()->size()-1)
                        {
                            sortAttribute = indexDataContainerConf->getAttributeToSort();
                        }

                        delete attributeToSortParamName_cstar;
                    }
                    else{
                        sortAttribute = indexDataContainerConf->getAttributeToSort();
                    }
                }

                {
                    const char *orderParamName = evhttp_find_header(&headers, URLParser::orderParamName);
                    if (orderParamName){
                        size_t sz;
                        char *orderParamName_cstar = evhttp_uridecode(orderParamName, 0, &sz);
                        order = (atoi(orderParamName_cstar) == 0) ? Ascending : Descending;
                        delete orderParamName_cstar;
                    }
                    else{
                        order = (indexDataContainerConf->getOrdering() == 0) ? Ascending : Descending;
                    }
                }

                urlParserHelper.sortby = sortAttribute;
                urlParserHelper.order = order;

                this->exactQuery = new Query(GetAllResultsQuery);
                this->fuzzyQuery = new Query(GetAllResultsQuery);

                this->exactQuery->setSortableAttribute(sortAttribute, order);
                this->fuzzyQuery->setSortableAttribute(sortAttribute, order);

                break;

            case 2:
                this->exactQuery = new Query(MapQuery);
                this->fuzzyQuery = new Query(MapQuery);

                {
                    const char *orderParamName = evhttp_find_header(&headers, URLParser::orderParamName);
                    if (orderParamName){
                        size_t sz;
                        char *orderParamName_cstar = evhttp_uridecode(orderParamName, 0, &sz);
                        order = (atoi(orderParamName_cstar) == 0) ? Ascending : Descending;
                        delete orderParamName_cstar;
                    }

                    const char *paramIter_latLB = evhttp_find_header(&headers, URLParser::leftBottomLatitudeParamName);
                    const char *paramIter_lngLB = evhttp_find_header(&headers, URLParser::leftBottomLongitudeParamName);
                    const char *paramIter_latRT = evhttp_find_header(&headers, URLParser::rightTopLatitudeParamName);
                    const char *paramIter_lngRT = evhttp_find_header(&headers, URLParser::rightTopLongitudeParamName);

                    const char *paramIter_latCT = evhttp_find_header(&headers, URLParser::centerLatitudeParamName);
                    const char *paramIter_lngCT = evhttp_find_header(&headers, URLParser::centerLongitudeParamName);
                    const char *paramIter_radius= evhttp_find_header(&headers, URLParser::radiusParamName);

                    if ( paramIter_latLB && paramIter_lngLB
                            && paramIter_latRT && paramIter_lngRT ){
                        size_t sz1;
                        char *paramIter_latLB_cstar = evhttp_uridecode(paramIter_latLB, 0, &sz1);
                        double range_min_x = atof(paramIter_latLB_cstar);
                        delete paramIter_latLB_cstar;

                        size_t sz2;
                        char *paramIter_lngLB_cstar = evhttp_uridecode(paramIter_lngLB, 0, &sz2);
                        double range_min_y = atof(paramIter_lngLB_cstar);
                        delete paramIter_lngLB_cstar;

                        size_t sz3;
                        char *paramIter_latRT_cstar = evhttp_uridecode(paramIter_latRT, 0, &sz3);
                        double range_max_x = atof(paramIter_latRT_cstar);
                        delete paramIter_latRT_cstar;

                        size_t sz4;
                        char *paramIter_lngRT_cstar = evhttp_uridecode(paramIter_lngRT, 0, &sz4);
                        double range_max_y = atof(paramIter_lngRT_cstar);
                        delete paramIter_lngRT_cstar;

                        exactQuery->setRange(range_min_x, range_min_y, range_max_x, range_max_y);
                        fuzzyQuery->setRange(range_min_x, range_min_y, range_max_x, range_max_y);
                    }
                    else if (paramIter_latCT && paramIter_lngCT && paramIter_radius){
                        size_t sz1;
                        char *paramIter_latCT_cstar = evhttp_uridecode(paramIter_latCT, 0, &sz1);
                        double x = atof(paramIter_latCT_cstar);
                        delete paramIter_latCT_cstar;

                        size_t sz2;
                        char *paramIter_lngCT_cstar = evhttp_uridecode(paramIter_lngCT, 0, &sz2);
                        double y = atof(paramIter_lngCT_cstar);
                        delete paramIter_lngCT_cstar;

                        size_t sz3;
                        char *paramIter_radius_cstar = evhttp_uridecode(paramIter_radius, 0, &sz3);
                        double r = atof(paramIter_radius_cstar);
                        delete paramIter_radius_cstar;

                        exactQuery->setRange(x, y, r);
                        fuzzyQuery->setRange(x, y, r);
                    }
                    else{
                        urlParserHelper.parserSuccess = false;
                        urlParserHelper.parserErrorMessage << "{\"error\":\"latlong for mapquery is invalid.\"}";
                        return;
                    }
                }


                break;

            default:
                urlParserHelper.parserSuccess = false;
                urlParserHelper.parserErrorMessage << "{\"error\":\"Invalid value for Searcher Type\"}";
                return;
        }

        // Construct the query
        bool termBoostsVectorValid = (termBoostsVector.size() == numberOfKeywords);
        bool similarityBoostsVectorValid = (similarityBoostsVector.size() == numberOfKeywords);

        exactQuery->setLengthBoost(lengthBoost);
        fuzzyQuery->setLengthBoost(lengthBoost);

        // set the prefix match penalty
        exactQuery->setPrefixMatchPenalty(indexDataContainerConf->getPrefixMatchPenalty());
        fuzzyQuery->setPrefixMatchPenalty(indexDataContainerConf->getPrefixMatchPenalty());

        //for each keyword, build the exact terms

        for (unsigned i = 0; i < numberOfKeywords; ++i){
            TermType termType = termTypeVector[i];

            unsigned termBoost;
            if (termBoostsVectorValid){
                termBoost = atoi(termBoostsVector[i].c_str());
                if (termBoost == 0 && termBoostsVector[i].compare("0") != 0){
                    urlParserHelper.parserSuccess = false;
                    urlParserHelper.parserErrorMessage << "{\"error\":\"Invalid value for termBoost option\"}";
                    return;
                }
            }
            else{
                termBoost = indexDataContainerConf->getQueryTermBoost();
            }

            float similarityBoost;
            if (similarityBoostsVectorValid){
                similarityBoost =  atof(similarityBoostsVector[i].c_str());
                //if(similarityBoost == 0 && similarityBoostsVector[i].compare("0") != 0){
                //    mg_printf(conn, "%sInvalid value for similarityBoost option\r\n", ajax_search_fail);
                //    return;
                //}
            }
            else{
                similarityBoost = indexDataContainerConf->getQueryTermSimilarityBoost();
            }

            Term *exactTerm;
            Term *fuzzyTerm;

            if(urlParserHelper.isFuzzy){
            	exactTerm = new Term(queryKeywordVector[i],
            			termType,
            			termBoost,
            			similarityBoost,
            			0);

                fuzzyTerm = new Term(queryKeywordVector[i],
                		termType,
                		termBoost,
                		similarityBoost,
                		Term::getNormalizedThreshold(getUtf8StringCharacterNumber(queryKeywordVector[i])));

                exactTerm->addAttributeToFilterTermHits(filters[i]);
                fuzzyTerm->addAttributeToFilterTermHits(filters[i]);

                this->exactQuery->add(exactTerm);
                this->fuzzyQuery->add(fuzzyTerm);

            }else{
                exactTerm = new Term(queryKeywordVector[i],
                		termType,
                		termBoost,
                		similarityBoost,
                		0);

                exactTerm->addAttributeToFilterTermHits(filters[i]);
                this->exactQuery->add(exactTerm);
            }

        }
    }
    urlParserHelper.parserSuccess = true;
}

URLToDoubleQuery::~URLToDoubleQuery()
{
    if(this->exactQuery != NULL){
        delete this->exactQuery;
    }

    if(this->fuzzyQuery != NULL){
        delete this->fuzzyQuery;
    }

}

}}
