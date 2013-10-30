
#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/IndexSearcher.h>
#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include "IntegrationTestHelper.h"
#include "analyzer/StandardAnalyzer.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2is;

Indexer *buildGeoIndex(string data_file, string index_dir)
{
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::LocationIndex);
    schema->setPrimaryKey("id");
    schema->setSearchableAttribute("name", 2);
    schema->setSearchableAttribute("category", 1);

    /// Create an Analyzer
    Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
    		"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "", srch2::instantsearch::STANDARD_ANALYZER);

    /// Create an index writer
    unsigned mergeEveryNSeconds = 3;
    unsigned mergeEveryMWrites = 5;
    unsigned updateHistogramEveryPMerges = 1;
    unsigned updateHistogramEveryQWrites = 5;
    IndexMetaData *indexMetaData = new IndexMetaData( new Cache(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,50000,
    		index_dir, "");
    Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

    Record *record = new Record(schema);

    unsigned docsCounter = 0;
    string line;

    ifstream data(data_file.c_str());

    /// Read records from file
    /// the file should have two fields, seperated by '^'
    /// the first field is the primary key, the second field is a searchable attribute
    while(getline(data,line))
    {
        unsigned cellCounter = 0;
        stringstream  lineStream(line);
        string cell;
        float lat=0.0, lng=0.0;

        while(getline(lineStream,cell,'^') && cellCounter < 6 )
        {
            if (cellCounter == 0)
            {
                record->setPrimaryKey(cell.c_str());
            }
            else if (cellCounter == 1)
            {
                record->setSearchableAttributeValue(0, cell);
            }
            else if (cellCounter == 2)
            {
                record->setSearchableAttributeValue(1, cell);
            }
            else if (cellCounter == 3)
            {
                record->setRecordBoost(atof(cell.c_str()));
            }
            else if (cellCounter == 4)
            {
                lat = atof(cell.c_str());
            }
            else if (cellCounter == 5)
            {
                lng = atof(cell.c_str());
            }

            cellCounter++;
        }

        record->setLocationAttributeValue(lat, lng);

        indexer->addRecord(record, analyzer);

        docsCounter++;

        record->clear();
    }

    cout << "#Docs Read:" << docsCounter << endl;

    indexer->commit();

    indexer->save();

    data.close();

    return indexer;
}

void query(Indexer *indexer, const string &keyword, double lb_lat, double lb_lng, double rt_lat, double rt_lng, TermType termType, unsigned ed, const vector<string> &expected)
{

    IndexSearcher *indexSearcher = IndexSearcher::create(indexer);

    Query *query = new Query(srch2::instantsearch::SearchTypeMapQuery);


    Term *term = NULL;
    if (ed>0)
        term = FuzzyTerm::create(keyword, termType, 1, 0.5, ed);
    else
        term = ExactTerm::create(keyword, termType, 1, 0.5);
    term->addAttributeToFilterTermHits(-1);
    query->setPrefixMatchPenalty(0.95);
    query->add(term);

    query->setRange(lb_lat, lb_lng, rt_lat, rt_lng);

    // for each keyword in the user input, add a term to the query
	QueryResults *queryResults = new QueryResults(new QueryResultFactory(), indexSearcher, query);

    indexSearcher->search(query, queryResults);

    ASSERT(expected.size() == queryResults->getNumberOfResults());

    for (unsigned i = 0; i < queryResults->getNumberOfResults(); i++)
    {
        ASSERT( queryResults->getRecordId(i) == expected[i] );
    }

    delete queryResults;
    delete query;
    delete indexSearcher;
}
//  for testing a  range query with a  rectangle
void query(Indexer *indexer,double lb_lat, double lb_lng, double rt_lat, double rt_lng, const vector<string> &expected)
{

    IndexSearcher *indexSearcher = IndexSearcher::create(indexer);
    Query *query = new Query(srch2::instantsearch::SearchTypeMapQuery);
    query->setRange(lb_lat,lb_lng,rt_lat,rt_lng);
    Rectangle *rectangleRange = new Rectangle();
    rectangleRange->min.x = lb_lat;
    rectangleRange->min.y = lb_lng;
    rectangleRange->max.x = rt_lat;
    rectangleRange->max.y = rt_lng;
	QueryResults *queryResults = new QueryResults(new QueryResultFactory(), indexSearcher, query);

    indexSearcher->search(*rectangleRange, queryResults);
    ASSERT(expected.size() == queryResults->getNumberOfResults());
    for (unsigned i = 0; i < queryResults->getNumberOfResults(); i++)
	{
	   ASSERT( queryResults->getRecordId(i) == expected[i] );
	}
    delete queryResults;
    delete query;
    delete rectangleRange;
    delete indexSearcher;
}
//test a  range query  with a circle
void query(Indexer *indexer,double lb_lat, double lb_lng, double radius, const vector<string> &expected)
{

	IndexSearcher *indexSearcher = IndexSearcher::create(indexer);
	Query *query = new Query(srch2::instantsearch::SearchTypeMapQuery);
	query->setRange(lb_lat,lb_lng,radius);
	Point pnt;
	pnt.x=lb_lat;
	pnt.y=lb_lng;
	Circle *circleRange = new Circle(pnt,radius);

	QueryResults *queryResults = new QueryResults(new QueryResultFactory(), indexSearcher, query);
	indexSearcher->search(*circleRange,queryResults);
	ASSERT(expected.size() == queryResults->getNumberOfResults());
	for (unsigned i = 0; i < queryResults->getNumberOfResults(); i++)
	{
		ASSERT( queryResults->getRecordId(i) == expected[i] );
	}
	delete circleRange;
	delete queryResults;
	delete query;
	delete indexSearcher;
}

int main(int argc, char **argv)
{
    cout << "Test begins." << endl;
    cout << "-------------------------------------------------------" << endl;

    string index_dir = getenv("index_dir");
    string data_file = index_dir + "data";

    cout << "Read init data from " << data_file << endl;
    cout << "Save index to " << index_dir << endl;

   buildGeoIndex(data_file, index_dir);
   unsigned mergeEveryNSeconds = 10;
   unsigned mergeEveryMWrites = 5;
   unsigned updateHistogramEveryPMerges = 1;
   unsigned updateHistogramEveryQWrites = 5;

    IndexMetaData *indexMetaData = new IndexMetaData( new Cache(),
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,50000,
    		index_dir, "");
    Indexer *indexer = Indexer::load(indexMetaData);

    // Three records:
    // 1^prefix^dummy^0.441715770673^44.968654^-89.635963
    // 2^prefax^dummy^3.50400619173^31.478081^-100.477646
    // 3^dummy^dummy^4.49093064352^46.715284^-122.954735
    
    vector<string> expected;

    // for testing a range query but without keyword information
    expected.push_back("1");
    expected.push_back("2");
    query(indexer,0,-102,50,0,expected); // for testing a  range query with a  rectangle
    expected.clear();

    expected.push_back("3");
    expected.push_back("2");
    expected.push_back("1");
    query(indexer,50,-120,50,expected); //test a  range query  with a circle
    expected.clear();

    expected.push_back("1");
    query(indexer, "prefix", -200.0, -200.0, 200.0, 200.0, srch2is::TERM_TYPE_COMPLETE, 0, expected);
    expected.clear();


    query(indexer, "prefi", -200.0, -200.0, 200.0, 200.0, srch2is::TERM_TYPE_COMPLETE, 0, expected);

    // TODO revive this test case after the bug in active node is fixed
    /*
    expected.push_back("1");
    query(indexer, "prefi", -200.0, -200.0, 200.0, 200.0, srch2is::COMPLETE, 1, expected);
    expected.clear();
    */

    expected.push_back("1");
    query(indexer, "prefi", -200.0, -200.0, 200.0, 200.0, srch2is::TERM_TYPE_PREFIX, 0, expected);
    expected.clear();

    expected.push_back("1");
    expected.push_back("2");
    query(indexer, "prefi", -200.0, -200.0, 200.0, 200.0, srch2is::TERM_TYPE_PREFIX, 1, expected);
    expected.clear();

    delete indexer;

    return 0;
}
