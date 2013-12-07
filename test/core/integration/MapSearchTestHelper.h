
#ifndef __MAPSEARCHTESTHELPER_H__
#define __MAPSEARCHTESTHELPER_H__

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/QueryEvaluator.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include <instantsearch/QueryResults.h>

#include <query/QueryResultsInternal.h>
#include <operation/QueryEvaluatorInternal.h>

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>

using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace srch2is;


/**
 * This function computes a normalized edit-distance threshold of a
 * keyword based on its length.  This threshold can be used to support
 * instant search (search as you type) as a user types in a keyword
 * character by character.  The intuition is that we want to allow
 * more typos for longer keywords. The following is an example:
 * <table>
 * <TR> <TD> Prefix Term </TD>  <TD> Normalized Edit-Distance Threshold </TD> </TR>
 * <TR> <TD> ele </TD> <TD> 0 </TD> </TR>
 * <TR> <TD> elep </TD> <TD> 1 </TD> </TR>
 * <TR> <TD> elepha </TD> <TD> 2 </TD> </TR>
 * <TR> <TD> elephant </TD> <TD> 2 </TD> </TR>
 * </table>
 */
unsigned getNormalizedThresholdGeo(unsigned keywordLength)
{
	// Keyword length:             [1,3]        [4,5]        >= 6
	// Edit-distance threshold:      0            1           2
	if (keywordLength < 4)
		return 0;
	else if (keywordLength < 6)
		return 1;
	return 2;
}

bool parseLine(string &line, vector<string> &query, Rectangle &range)
{
	vector<string> fields;
	boost::split(fields, line, boost::is_any_of(","));

	if (fields.size() < 4)
	{
		return false;
	}

	range.max.x = atof(fields[0].c_str()) + 0.2;
	range.max.y = atof(fields[1].c_str()) + 0.2;
	range.min.x = atof(fields[0].c_str()) - 0.2;
	range.min.y = atof(fields[1].c_str()) - 0.2;

	size_t found;
	string tmp;

	found = fields[2].find_first_of(" ");
	if(found == string::npos)
		tmp = fields[2];
	else
		tmp = fields[2].substr(0, found);
	if(tmp.length() == 0)
		return false;
//	else if(tmp.length() > 1)
//		query.push_back(tmp.substr(1, tmp.length()));
//	else
		query.push_back(tmp);

	found = fields[3].find_last_of(" ");
	if(found == string::npos)
		tmp = fields[3];
	else
		tmp = fields[3].substr(found+1, fields[3].length());
	if(tmp.length() == 0)
		return false;
	else
		query.push_back(tmp);

	return true;
}

bool ifAllFoundResultsAreCorrect(const vector<unsigned> &expectedResults, const vector<unsigned> &results)
{
	for(unsigned i = 0; i < results.size(); i++)
	{
		bool found = false;
		unsigned resultRecordId = results[i];
		for(unsigned j = 0; j < expectedResults.size(); j++)
		{
			if( resultRecordId == expectedResults[j])
			{
				found = true;
				break;
			}
		}
		if(!found)
			return false;
	}

	return true;
}

bool ifAllExpectedResultsAreFound(const vector<unsigned> &expectedResults, const vector<unsigned> &results)
{
	for(unsigned i = 0; i < expectedResults.size(); i++)
	{
		bool found = false;
		for(unsigned j = 0; j < results.size(); j++)
		{
			unsigned resultRecordId = results[j];
			if( resultRecordId == expectedResults[i])
			{
				found = true;
				break;
			}
		}
		if(!found)
			return false;
	}

	return true;
}

void readGeoRecordsFromFile(string filepath, Indexer *index, Schema *schema, Analyzer* analyzer)
{
	ifstream data(filepath.c_str());

	std::string line;
	int cellCounter;
	while(std::getline(data,line))
	{
		Record *record = new Record(schema);

		cellCounter = 0;
		std::stringstream  lineStream(line);
		std::string        cell;

		Point p;
		while(std::getline(lineStream,cell,'^') && cellCounter < 8 )
		{
			//cell.assign(cell,1,cell.length()-2);
			if (cellCounter == 0)
			{
				unsigned primaryKey = atoi(cell.c_str());
				record->setPrimaryKey(primaryKey);
			}
			else if (cellCounter < 3)
			{
				if (cellCounter == 1)
				{
					p.x = atof(cell.c_str());
					if(p.x > 200.0 || p.x < -200.0)
					{
						cout << "bad x: " << p.x << ", set to 40.0 for testing purpose" << endl;
						p.x = 40.0;
					}
				}
				if (cellCounter == 2)
				{
					p.y	= atof(cell.c_str());
					if(p.y > 200.0 || p.y < -200.0)
					{
						cout << "bad y: " << p.y << ", set to -120.0 for testing purpose" << endl;
						p.y = -120.0;
					}
				}
			}
			else
			{
				if(cell.compare("") == 0)
				{
					record->setSearchableAttributeValue(cellCounter-3,"");
				}
				else
				{
					record->setSearchableAttributeValue(cellCounter-3,cell);
				}

			}
			cellCounter++;
		}
		record->setLocationAttributeValue(p.x, p.y);

		// add the record
		index->addRecord(record, analyzer);
		delete record;
	}

	data.close();

}

void printGeoResults(srch2is::QueryResults *queryResults, unsigned offset = 0)
{
	cout << "Number of hits:" << queryResults->getNumberOfResults() << endl;
	for (unsigned resultIter = offset;
			resultIter < queryResults->getNumberOfResults() ; resultIter++)
	{
		vector<string> matchingKeywords;
		vector<unsigned> editDistances;

		// For each result, get the matching keywords and their edit distances
		queryResults->getMatchingKeywords(resultIter, matchingKeywords);
		queryResults->getEditDistances(resultIter, editDistances);

		// Output the result information
		cout << "\nResult-(" << resultIter << ") RecordId:"
				<< queryResults->getRecordId(resultIter)
				<< "\tScore:" << queryResults->getResultScoreString(resultIter)
				<< "\nDistance:" << queryResults->getPhysicalDistance(resultIter);

		cout << "\nMatching Keywords:" << endl;
		unsigned editDistancesIter = 0;
		for(vector<string>::iterator iter = matchingKeywords.begin();
				iter != matchingKeywords.end(); iter ++, editDistancesIter ++)
		{
			cout << "\t"
					<< *iter << " "
					<< editDistances.at(editDistancesIter);
		}
		cout<<endl;
	}
}

float pingToGetTopScoreGeo(const Analyzer *analyzer, QueryEvaluator * queryEvaluator, string queryString, float lb_lat, float lb_lng, float rt_lat, float rt_lng)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeMapQuery);

    vector<PositionalTerm> queryKeywords;
    analyzer->tokenizeQuery(queryString,queryKeywords);
    // for each keyword in the user input, add a term to the querygetThreshold(queryKeywords[i].size())
    //cout<<"Query:";
    for (unsigned i = 0; i < queryKeywords.size(); ++i)
    {
        //cout << "(" << queryKeywords[i] << ")("<< getNormalizedThreshold(queryKeywords[i].size()) << ")\t";
        TermType termType = TERM_TYPE_PREFIX;
        Term *term = FuzzyTerm::create(queryKeywords[i].term, termType, 1, 0.5, getNormalizedThresholdGeo(queryKeywords[i].term.size()));
        term->addAttributeToFilterTermHits(-1);
        query->setPrefixMatchPenalty(0.95);
        query->add(term);
    }

    query->setRange(lb_lat, lb_lng, rt_lat, rt_lng);

    //cout << "[" << queryString << "]" << endl;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    queryEvaluator->search(query, queryResults);
    //printGeoResults(queryResults);

    float resVal = queryResults->getResultScore(0).getFloatTypedValue();
    delete queryResults;
    delete query;
    return resVal;
}

int pingToCheckIfHasResults(const Analyzer *analyzer, QueryEvaluator * queryEvaluator, string queryString, float lb_lat, float lb_lng, float rt_lat, float rt_lng, int ed)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeMapQuery);

    vector<PositionalTerm> queryKeywords;
    analyzer->tokenizeQuery(queryString,queryKeywords);
    // for each keyword in the user input, add a term to the querygetThreshold(queryKeywords[i].size())
    //cout<<"Query:";
    srch2is::TermType termType = TERM_TYPE_COMPLETE;
    for (unsigned i = 0; i < queryKeywords.size(); ++i){
        //cout << "(" << queryKeywords[i] << ")("<< getNormalizedThreshold(queryKeywords[i].size()) << ")\t";
        Term *term = NULL;
        if(i == (queryKeywords.size()-1)){
            termType = TERM_TYPE_PREFIX;
        }
        if (ed>0)
            term = FuzzyTerm::create(queryKeywords[i].term, termType, 1, 0.5, ed);
        else
            term = ExactTerm::create(queryKeywords[i].term, termType, 1, 0.5);
        term->addAttributeToFilterTermHits(-1);
        query->setPrefixMatchPenalty(0.95);
        query->add(term);
    }

    query->setRange(lb_lat, lb_lng, rt_lat, rt_lng);

    //cout << "[" << queryString << "]" << endl;

    // for each keyword in the user input, add a term to the query
    QueryResults *queryResults = new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    queryEvaluator->search(query, queryResults);
    //printGeoResults(queryResults);
    //cout << "num of res: " << queryResults->getNumberOfResults() << endl;

    int returnValue =  queryResults->getNumberOfResults();
    delete queryResults;
    delete query;
    return returnValue;
}

unsigned existsInTopKGeo(const Analyzer *analyzer, QueryEvaluator *  queryEvaluator, string queryString, string primaryKey, int k, float lb_lat, float lb_lng, float rt_lat, float rt_lng)
{
    Query *query = new Query(srch2::instantsearch::SearchTypeMapQuery);

    vector<PositionalTerm> queryKeywords;
    analyzer->tokenizeQuery(queryString,queryKeywords);
    // for each keyword in the user input, add a term to the querygetThreshold(queryKeywords[i].size())
    for (unsigned i = 0; i < queryKeywords.size(); ++i)
    {
        TermType termType = TERM_TYPE_PREFIX;
        Term *term = ExactTerm::create(queryKeywords[i].term, termType, 1, 0.5);
        term->addAttributeToFilterTermHits(-1);
        query->setPrefixMatchPenalty(0.95);
        query->add(term);
    }

    query->setRange(lb_lat, lb_lng, rt_lat, rt_lng);
    QueryResults *queryResultsK = new QueryResults(new QueryResultFactory(),queryEvaluator, query);

    queryEvaluator->search(query, queryResultsK);

    //printResults(queryResultsK1);

    bool inTopK = false;

    unsigned kResultNum =  queryResultsK->getNumberOfResults();
    for(unsigned i = 0; i < kResultNum; i++)
    {
        if(queryResultsK->getRecordId(i) == primaryKey)
        {
            inTopK = true;
            break;
        }
    }

    delete queryResultsK;
    delete query;

    return inTopK;
}

#endif /* __MAPSEARCHTESTHELPER_H__ */
