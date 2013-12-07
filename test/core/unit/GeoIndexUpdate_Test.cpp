//$Id: GeoIndexUpdate_Test.cpp 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $
#include <sstream>
#include <iostream> 
#include <time.h>
#include <boost/algorithm/string.hpp>

#include "geo/QuadTree.h"
#include "util/Assert.h"

#include <instantsearch/Query.h>
#include <instantsearch/Term.h>
#include <instantsearch/QueryResults.h>
#include <instantsearch/QueryEvaluator.h>
#include <instantsearch/GlobalCache.h>

using namespace std;
using namespace srch2::instantsearch;

const unsigned mergeEveryNSeconds = 10;
const unsigned mergeEveryMWrites = 5;
const unsigned updateHistogramEveryPMerges = 1;
const unsigned updateHistogramEveryQWrites = 5;

// convert a string to an integer.  similar to "atoi()"
bool parseUnsigned(string &line, unsigned &pk)
{
	istringstream coordinatesStream(line);
	unsigned num;
	vector<unsigned> nums;
	while(coordinatesStream >> num)
	{
		nums.push_back(num);
	}
	if(nums.size() == 1)
	{
		pk = nums[0];
	}
	else
	{
		return false;
	}
	return true;
}

bool parsePoint(string &line, Point &pt)
{
	istringstream coordinatesStream(line);
	double coord;
	vector<double> coords;
	while(coordinatesStream >> coord)
	{
		coords.push_back(coord);
	}
	if(coords.size() == 2)
	{
		pt.x = coords[0];
		pt.y = coords[1];
	}
	else
	{
		return false;
	}
	return true;
}

void addLocationRecordWithSingleAttr(vector< pair<string, pair<string, Point> > > &recordsToSearch, Indexer *indexer, Schema *schema,Analyzer* analyzer, unsigned primaryKey, const string &firstAttribute, double pointX, double pointY)
{
    if (pointX > 200.0 || pointX < -200.0
        || pointY > 200.0 || pointY < -200.0)
        return;
	Point point;
	point.x = pointX;
	point.y = pointY;
	Record *record = new Record(schema);
    stringstream pkey_string;
    pkey_string << primaryKey;
	record->setPrimaryKey(pkey_string.str()); // give a value for the primary key
	record->setSearchableAttributeValue(0, firstAttribute);
	record->setLocationAttributeValue(point.x, point.y);

	indexer->addRecord(record, analyzer);

    // store the inserted record to search later
    pair<string, Point> recordToSearch (pkey_string.str(), point);
    recordsToSearch.push_back( make_pair(firstAttribute, recordToSearch) );

	delete record;
}

void readSingleAttrRecordsFromFile(vector< pair<string, pair<string, Point> > > &recordsToSearch, Indexer *indexer, Schema *schema, Analyzer* analyzer, const string &directoryName)
{
    string primaryKeysFile = directoryName + "/primaryKeys.txt";
    string pointsFile = directoryName + "/points.txt";
    string firstAttrsFile = directoryName + "/firstAttr.txt";
    //string secondAttrsFile = directoryName + "/secondAttr.txt";

    ifstream primaryKeys(primaryKeysFile.c_str());
    ASSERT ( primaryKeys ); // cannot open primaryKeys file

    ifstream points(pointsFile.c_str());
    ASSERT  ( points ); //cannot open points file

    ifstream firstAttrs(firstAttrsFile.c_str());
    ASSERT  ( firstAttrs ); // cannot open firstAttrs file

    //ifstream secondAttrs(secondAttrsFile.c_str());
    //ASSERT  ( secondAttrs ); //cannot open secondAttrs file

    string primaryKeyLine;
    string pointLine;
    string firstAttrLine;
    //string secondAttrLine;
    
    // inserting objects in the Quadtree
    while (getline(primaryKeys, primaryKeyLine)
            && getline(points, pointLine)
            && getline(firstAttrs, firstAttrLine))
      //      && getline(secondAttrs, secondAttrLine))
    {
        unsigned primaryKey = 0;
        Point point;
        point.x = 0.0;
        point.y = 0.0;

        bool retval = parseUnsigned(primaryKeyLine, primaryKey);
        ASSERT  ( retval ); //cannot parse primaryKeys file correctly
        retval = parsePoint(pointLine, point);
        ASSERT  ( retval ); //cannot parse points file correctly"
        (void)retval;

        //addLocationRecord(indexer, schema, primaryKey, firstAttrLine, secondAttrLine, point.x, point.y);
        addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, primaryKey, firstAttrLine, point.x, point.y);
    }
}

void generateTermAddToQuery(const string &keyword, Query *query, bool isFuzzy)
{
    if(isFuzzy)
    {
        string fuzzy_keyword = "a" + keyword;
        Term *term = FuzzyTerm::create(fuzzy_keyword, TERM_TYPE_PREFIX, 1, 100.0, 1);
        query->add(term);
    }
    else
    {
        Term *term = ExactTerm::create(keyword, TERM_TYPE_PREFIX, 1, 100.0);
        query->add(term);
    }

}

void searchRecords(const vector< pair<string, pair<string, Point> > > &recordsToSearch, Indexer *indexer, Analyzer *analyzer, bool shouldExist = true, bool isFuzzy = true)
{
    QueryEvaluatorRuntimeParametersContainer runTimeParameters;
    QueryEvaluator * queryEvaluator = new QueryEvaluator(indexer,&runTimeParameters );

    vector<PositionalTerm> queryKeywords;

    // go through each record to verify
    for (unsigned i = 0; i < recordsToSearch.size(); i++)
    {
        analyzer->tokenizeQuery(recordsToSearch[i].first, queryKeywords);

        // for each prefix of the first keyword
        for(unsigned j = 1; j <= queryKeywords[0].term.size(); j++)
        {
            Query *query = new Query(SearchTypeMapQuery);

            // gernerate a term for the prefix, add it to the query
            string prefix = queryKeywords[0].term.substr(0, j);
            generateTermAddToQuery(prefix, query, isFuzzy);

            //cout << prefix << " ";
            // generate one term for each of the remaining keywords, if any, add all terms to the query
            for (unsigned k = 1; k < queryKeywords.size(); k++)
            {
                //cout << queryKeywords[k] << " ";
                generateTermAddToQuery(queryKeywords[k].term, query, isFuzzy);
            }
            //cout << endl;

            // set the query range
            query->setRange( recordsToSearch[i].second.second.x - 0.05,
                             recordsToSearch[i].second.second.y - 0.05,
                             recordsToSearch[i].second.second.x + 0.05,
                             recordsToSearch[i].second.second.y + 0.05 );

        	QueryResults *queryResults = new QueryResults(new QueryResultFactory(), queryEvaluator, query);

            unsigned expectedRecordId = atoi(recordsToSearch[i].second.first.c_str());
            queryEvaluator->search(query, queryResults);

            bool pass = true;
            if(shouldExist)
            {
                if ( queryResults->getNumberOfResults() == 0 || queryResults->getRecordId(0) != recordsToSearch[i].second.first)
                {
                    pass = false;
                    cout << "record: "<< expectedRecordId << " " << prefix << " " << recordsToSearch[i].second.second.x << " " << recordsToSearch[i].second.second.y << endl;
                    cout << "num of results: " << queryResults->getNumberOfResults() << endl;
                    if (queryResults->getNumberOfResults() != 0)
                    {
                        for (int r = 0; r < queryResults->getNumberOfResults(); r++)
                        {
                            if (queryResults->getRecordId(r) == recordsToSearch[i].second.first)
                            {
                                cout << "found as result NO." << r+1 << endl;
                                pass = true;
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                for (int r = 0; r < queryResults->getNumberOfResults(); r++)
                {
                    if (queryResults->getRecordId(r) == recordsToSearch[i].second.first)
                    {
                        cout << "found the deleted result NO." << r+1 << endl;
                        pass = false;
                        break;
                    }
                }

            }

            ASSERT( pass );
            if(!pass)
                cout << "FAIL!" << endl;

            delete queryResults;
            delete query;
        }

        queryKeywords.clear();
    }

    delete queryEvaluator;
}

/*
 * o-filter threshold: 16
 *
 * Test 1
 *
 * Initial "z" prefix on quad tree root's o-filter, 10 records:
 *
 *                                 |  keyword  |  internalRecordId  |  event  |
 *
 *                                    "zzzzzzz"        (8,6)
 *                                    "zei"            (89,6)
 *                                    "zandini"        (94,6)
 *                                    "zimmerman"      (200,6)
 *                                    "zet"            (307,6)
 *                                    "zet"            (361,6)
 *                                    "zenith"         (665,6)
 *                                    "znet"           (759,2)
 *                                    "zion"           (788,6)
 *                                    "zales"          (850,2)
 *
 * Add a record:                      "zooa"           (900,2)            add to o-filter "z" prefix, now 11 records, threshold is not exceeded
 */
void test1 (vector< pair<string, pair<string, Point> > > &recordsToSearch, Indexer *indexer, Schema *schema, Analyzer *analyzer)
{
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10001, "zooa", 44.968731, -189.636891); // 900 root
}

/*
 * Test 2
 *
 * Add 5 records:                     "zoob"           (901-905,6)        add to o-filter "z" prefix, now 16 records, threshold is not exceeded 
 * Add a record:                      "zoob"           (906,6)            add to o-filter "z" prefix, exceed "z" prefix's o-filter threshold, split it
 *
 * ----------------
 * Split "z" to "za", "ze", "zi", "zn" and "zo"
 * ----------------
 * "za" prefix on quad tree root's o-filter:
 *
 *                                    "zandini"          94
 *                                    "zales"            850
 *
 * "ze" prefix on quad tree root's o-filter:
 *
 *                                    "zei"              89
 *                                    "zet"              307, 361
 *                                    "zenith"           665
 *
 * "zi" prefix on quad tree root's o-filter:
 *
 *                                    "zimmerman"        200
 *                                    "zion"             788
 *
 * "zn" prefix on quad tree root's o-filter:
 *
 *                                    "znet"             759
 *
 * "zo" prefix on quad tree root's o-filter:
 *
 *                                    "zooa"             900
 *                                    "zoob"             901-906
 *
 * "zn" prefix on quad tree root's o-filter:
 *
 *                                    "zzzzzzz"          8
 * ----------------
 *
 * Add 9 records:                     "zoob"           (907-915,6)        add to o-filter "zo" prefix, now 16 records, threshold is not exceeded
 */
void test2 (vector< pair<string, pair<string, Point> > > &recordsToSearch, Indexer *indexer, Schema *schema, Analyzer* analyzer)
{
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10002, "zoob", 42.084911, -80.146126); //901 root
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10003, "zoob", 42.320222, -71.592598); //902 root
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10004, "zoob", 39.955758, -82.719177); //903 root
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10005, "zoob", 41.667911, -71.53612); //904 root
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10006, "zoob", 29.743723, -94.96186); //905 root
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10007, "zoob", 40.725395, -73.976924); //906 root "z" too many, split, recover "z" on root's children quad nodes o-filter
                                                                                                            //    root->6
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10008, "zoob", 41.857385, -69.970816); //907 root->6
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10009, "zoob", 38.230989, -85.696596); //908 root->6
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10010, "zoob", 35.939134, -80.056366); //909 root->6 "z" too many, split, recover "z" on 6's children quad nodes o-filter
                                                                                                            //            but because already has "z"'s descendent prefixes on root, we can't build any o-filter here
                                                                                                            //            other than "zo", also shouldn't build "za", "zb"...
                                                                                                            //    6->1    on o-filter of "z", which was just recovered
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10011, "zoob", 37.112809, -94.480568); //910 root->6->1
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10012, "zoob", 41.119993, -81.074922); //911 root->6->1
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10013, "zoob", 39.863316, -74.95829); //912 root->6->5
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10014, "zoob", 39.895954, -76.613581); //913 root->6->1
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10015, "zoob", 41.511911, -87.25581); //914 root->6->1
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10016, "zoob", 33.846493, -84.249827); //915 root->6->1 "z" too many, split, recover "z" on 1's children quad nodes o-filter
                                                                                                            //               but because already has "z"'s descendent prefixes on root, we can't build any o-filter here
                                                                                                            //    1->9 prefix "z" is new for 9, it's not recovered ealier, so we build new o-filter for "z" here
}

/*
 * Test 3
 *
 * Add a record:                      "zooa"           (916,6)            add to o-filter "zo" prefix, exceed "zo" prefix's o-filter threshold, split it
 *
 * ----------------
 * Split "zo" to "zooa" and "zoob"
 * ----------------
 * "zooa" prefix on quad tree root's o-filter:
 *
 *                                    "zooa"             900, 916
 *
 * "zoob" prefix on quad tree root's o-filter:
 *
 *                                    "zoob"             901-915
 * ----------------
 */
void test3 (vector< pair<string, pair<string, Point> > > &recordsToSearch, Indexer *indexer, Schema *schema, Analyzer* analyzer)
{
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10017, "zooa", 42.313758, -71.224739); //916 root    "zo" too many, split, recover "zo" on root's children quad nodes o-filter
                                                                                                            //    root->6 on o-filter of "zo"
                                                                                                            //    6->5 on o-filter of "z"
}

/*
 * Test 4
 *
 * Add a record:                      "zoob"           (917,6)            add to o-filter "zoob" prefix, now 16 records, threshold is not exceeded
 * Add a record:                      "zoob"           (918,2)            add to o-filter "zoob" prefix, exceed "zoob" prefix's o-filter threshold, split it
 *
 * ----------------
 * Since "zoob" is a complete keyword, we need to split corresponding records to the children quad nodes
 * ----------------
 *
 * ----------------
 */
void test4 (vector< pair<string, pair<string, Point> > > &recordsToSearch, Indexer *indexer, Schema *schema, Analyzer* analyzer)
{
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10018, "zoob", 41.032842, -85.112838); //917 root->6 "zo" too many, split, recover "zo" on 6's children quad nodes o-filter
                                                                                                            //            but because already has "zo"'s descendent prefixes on root, we can't build any o-filter here
                                                                                                            //    6->1    on o-filter of "zo"
                                                                                                            //    1->10   on o-filter of "z"
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10019, "zoob", 39.784264, -196.90046); //918 root    "zoob" too many, and doesn't have any descendent prefix to put on o-filter here
                                                                                                            //            recover "zoob" on root's children quad nodes o-filter (if no ancestor prefixes are already there)
                                                                                                            //    root->2 on o-filter of "z"
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10020, "zooa", 29.720766, -95.644299); //919 root->6->1->0
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10021, "zooa", 42.006142, -87.723387); //920 root->6->1->6
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10022, "zooa", 39.932171, -91.367813); //921 root->6->1->6
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10023, "zooa", 40.651611, -74.323787); //922 root->6->5
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10024, "zooa", 41.387675, -88.436038); //923 root->6->1->6
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10025, "zooa", 41.65234, -93.620104); //924 root->6->1->6
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10026, "zooa", 32.475117, -84.87687); //925 root->6->1 "zo" too many, split, has "zooa" on root's o-filter, so can only build o-filter for "zoob" here
                                                                                                           //               recover "zo" on 1's children quad nodes o-filter (if no ancestor prefixes are already there)
                                                                                                           //    1->9       on o-filter of "z"
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10027, "zooa", 41.55043, -88.102959);  //926 root->6->1->6
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10028, "zooa", 37.086221, -76.470469); //927 root->6->1->13
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10029, "zooa", 39.754002, -75.129142); //928 root->6->1->14
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10030, "zooa", 40.844204, -73.986604); //929 root->6->5
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10031, "zooa", 38.59078, -92.2489); //930 root->6->1->6
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10032, "zooa", 38.969364, -77.080132); //931 root->6->1->14
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10033, "zooa", 41.89024, -88.00116); //932 root->6->1->6
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10034, "zooa", 27.624649, -80.436362); //933 root "zooa" too many, and doesn't have any descendent prefix to put on o-filter here
                                                                                                            // recover "zooa" on root's children quad nodes o-filter (if no ancestor prefixes are already there)
                                                                                                            // root->6 "zooa" too many, and doesn't have any descendent prefix to put on o-filter here
                                                                                                            // recover "zooa" on 6's children quad nodes o-filter  (if no ancestor prefixes are already there)
                                                                                                            // 6->1 "zooa" is recovered on o-filter
                                                                                                            // 1->12
}

void test5 (vector< pair<string, pair<string, Point> > > &recordsToSearch, Indexer *indexer, Schema *schema, Analyzer* analyzer)
{
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99901, "zoo", 46.725251, -122.954566); //934 root->2
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99902, "zoo", 46.735251, -122.954566); //935 root->2
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99903, "zoo", 46.745251, -122.954566); //936 root->2
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99904, "zoo", 46.755251, -122.954566); //937 root->2
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99905, "zoo", 46.765251, -122.954566); //938 root->2
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99906, "zoo", 46.775251, -122.954566); //939 root->2
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99907, "zoo", 46.785251, -122.954566); //940 root->2
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99908, "zoo", 46.795251, -122.954566); //941 root->2
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99909, "zoo", 46.705251, -122.954566); //942 root->2
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99910, "zoo", 46.715251, -122.964566); //943 root->2
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99911, "zoo", 46.715251, -122.974566); //944 root->2
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99912, "zoo", 46.715251, -122.984566); //945 root->2
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99913, "zoo", 46.715251, -122.994566); //946 root->2 "z" too many, split, recover "z" on 2's children quad nodes o-filter
                                                                                                            //    2->13 on o-filter of "z", which was just recovered
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99914, "zoo", 46.715251, -122.904566); //947 root->2->13
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99915, "zoo", 46.715251, -122.914566); //948 root->2 "zo" too many, split, recover
                                                                                                            //    2->13 "z"  too many, split, recover "z" on 13's children quad nodes o-filter
                                                                                                            //    13->3 on o-filter of "z", which was just recovered
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99916, "zoo", 46.715251, -122.924566); //949 root->2->13->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 99917, "zoo", 46.715251, -122.934566); //950 root->2->13 "zo" too many, "zoo" also too many
                                                                                                            //    13->3 "zo" too many, "zoo" also too many
                                                                                                            //    3->5(leaf) "zo" too many, "zoo" also too many
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10035, "zooa", 46.715251, -122.954566); //951 root->2 on o-filter of "zooa"
                                                                                                             //    2->13->3->5
}

void test6 (vector< pair<string, pair<string, Point> > > &recordsToSearch, Indexer *indexer, Schema *schema, Analyzer* analyzer)
{
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10036, "zooa", 34.004789, -117.328692); //952 root->2 on o-filter of "zooa"
                                                                                                             //    2->13->5 on o-filter of "z"
}

void test7 (vector< pair<string, pair<string, Point> > > &recordsToSearch, Indexer *indexer, Schema *schema, Analyzer* analyzer)
{
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10037, "zooa", -150.0, 150.0); //953 root->12
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10038, "zooa", -50.0, 150.0); //954 root->13
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10039, "zooa", 50.0, 150.0); //955 root->14
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10040, "zooa", 150.0, 150.0); //956 root->15
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10041, "zooa", -150.0, 50.0); //957 root->8
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10042, "zooa", -50.0, 50.0); //958 root->9
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10043, "zooa", 50.0, 50.0); //959 root->10
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10044, "zooa", 150.0, 50.0); //960 root->11
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10045, "zooa", -150.0, -50.0); //961 root->4
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10046, "zooa", -50.0, -50.0); //962 root->5
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10047, "zooa", 150.0, -50.0); //963 root->7
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10048, "zooa", -150.0, -150.0); //964 root->0
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10049, "zooa", -50.0, -150.0);  //965 root->1
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10050, "zooa", 150.0, -150.0); //966 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10051, "zooa", 151.0, -151.0); //967 root->3
}

void test8 (vector< pair<string, pair<string, Point> > > &recordsToSearch, Indexer *indexer, Schema *schema, Analyzer* analyzer)
{
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10052, "zooa", 150.1, -150.0); //968 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10053, "zooa", 150.2, -150.0); //969 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10054, "zooa", 150.3, -150.0); //970 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10055, "zooa", 150.4, -150.0); //971 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10056, "zooa", 150.5, -150.0); //972 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10057, "zooa", 150.0, -150.1); //973 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10058, "zooa", 150.0, -150.2); //974 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10059, "zooa", 151.0, -151.3); //975 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10060, "zooa", 151.0, -151.4); //976 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10061, "zooa", 151.0, -151.5); //977 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10062, "zooa", 151.6, -151.0); //978 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10063, "zooa", 151.7, -151.0); //979 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10064, "zooa", 151.8, -151.0); //980 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10065, "zooa", 151.9, -151.0); //981 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10066, "zooa", 151.0, -151.6); //982 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10067, "zooa", 151.0, -151.7); //983 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10068, "zooa", 151.0, -151.8); //984 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10069, "zooa", 151.0, -151.9); //985 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10070, "zooa", 151.0, -150.1); //986 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10071, "zooa", 151.0, -150.2); //987 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10072, "zooa", 151.0, -150.3); //988 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10073, "zooa", 151.0, -150.4); //989 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10074, "zooa", 151.0, -150.5); //990 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10075, "zooa", 151.0, -150.6); //991 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10076, "zooa", 151.0, -150.7); //992 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10077, "zooa", 151.0, -150.8); //993 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10078, "zooa", 151.0, -150.9); //994 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10079, "zooa", 150.6, -151.6); //995 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10080, "zooa", 150.7, -151.6); //996 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10081, "zooa", 150.8, -151.6); //997 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10082, "zooa", 150.9, -151.6); //998 root->3
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10083, "zooa", 150.9, -150.6); //999 root->3 split the leaf node
                                                                                                    //    "z", "zo", "zooa" all too many, don't build any o-filter
                                                                                                    //    3->6 (leaf)
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10084, "zoob", 150.7, -150.6); //1000 root->3 create o-filter for "zoob"
                                                                                                    //    3->6 (leaf)
}

// start to test the assign-new-keyword-ids functionality
void test9 (vector< pair<string, pair<string, Point> > > &recordsToSearch, Indexer *indexer, Schema *schema, Analyzer* analyzer)
{
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10085, "zzzzzzzzzzzzzzz", 150.9, -151.7); //1001 root create o-filter for new keyword prefix "zzzzzzzzzzzzzzz"
                                                                                                               //     root->3->6 (leaf)
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10086, "zzzzzzzzzzzzzzz1", 150.9, -152.0); //1002
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10087, "zzzzzzzzzzzzzzz2", 150.9, -152.1); //1003
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10088, "zzzzzzzzzzzzzzz3", 150.9, -152.2); //1004
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10089, "zzzzzzzzzzzzzzz4", 150.9, -152.3); //1005
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10090, "zzzzzzzzzzzzzzz5", 150.9, -152.4); //1006
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10091, "zzzzzzzzzzzzzzz6", 150.9, -152.5); //1007
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10092, "zzzzzzzzzzzzzzz7", 150.9, -152.6); //1008
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10093, "zzzzzzzzzzzzzzz8", 150.9, -152.7); //1009
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10094, "zzzzzzzzzzzzzzz9", 150.9, -152.8); //1010
   addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10095, "zzzzzzzzzzzzzzz0", 150.9, -152.9); //1011

}

void testInsertion(vector< pair<string, pair<string, Point> > > &recordsToSearch, Indexer *indexer, Schema *schema, Analyzer *analyzer)
{
    /// read, insert and search new records

    test1(recordsToSearch, indexer, schema, analyzer);

    test2(recordsToSearch, indexer, schema, analyzer);

    test3(recordsToSearch, indexer, schema, analyzer);

    test4(recordsToSearch, indexer, schema, analyzer);

    test5(recordsToSearch, indexer, schema, analyzer);

    test6(recordsToSearch, indexer, schema, analyzer);

    test7(recordsToSearch, indexer, schema, analyzer);

    test8(recordsToSearch, indexer, schema, analyzer);

    test9(recordsToSearch, indexer, schema, analyzer);

    sleep(mergeEveryNSeconds + 1);
    searchRecords(recordsToSearch, indexer, analyzer);
    //cout<<"case 9 passes" << endl;
}

void testDeletion(vector< pair<string, pair<string, Point> > > &recordsToSearch, Indexer *indexer, Analyzer *analyzer)
{
    vector< pair<string, pair<string, Point> > > deletedRecords;

    for (int i = 0; i < recordsToSearch.size(); i+=2)
    {
        indexer->deleteRecord(recordsToSearch[i].second.first);
        deletedRecords.push_back(recordsToSearch[i]);
        recordsToSearch.erase(recordsToSearch.begin() + i);
    }

    // Sleep to wait for the merging thread to take action
    sleep(mergeEveryNSeconds + 1);

    searchRecords(recordsToSearch, indexer, analyzer);
    searchRecords(deletedRecords, indexer, analyzer, false);
}

void testSmallInitLargeInsertion(const string directoryName)
{
    Cache *cache = new Cache(134217728,20000);
    IndexMetaData *indexMetaData = new IndexMetaData(cache,
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		directoryName, "");
    
    // Create a schema
    Schema *schema = Schema::create(LocationIndex);
    schema->setPrimaryKey("id"); // integer, by default not searchable
    schema->setSearchableAttribute("firstAttr", 2); // searchable text
    schema->setSearchableAttribute("secondAttr", 7); // searchable text

    // Create an analyzer
    Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
    		"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");
    Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

    vector< pair<string, pair<string, Point> > > recordsToSearch;
    addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10000001, "gravity paul", 29.743723, -94.96186);
    addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10000002, "god the devil and the world", 42.084911, -80.146126);
    addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10000003, "juliet", 42.320222, -71.592598);
    addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10000004, "lie to me", 39.955758, -82.719177);
    addLocationRecordWithSingleAttr(recordsToSearch, indexer, schema, analyzer, 10000005, "afro", 41.667911, -71.53612);
    indexer->commit();
    indexer->createAndStartMergeThreadLoop();
    searchRecords(recordsToSearch, indexer, analyzer);
    cout << "Small init index built correctly." << endl;
    readSingleAttrRecordsFromFile(recordsToSearch, indexer, schema, analyzer, directoryName+"/geo_update/aLotRecords");
    //readSingleAttrRecordsFromFile(recordsToSearch, indexer, schema, directoryName+"/geo_update/xrecords");
    sleep(mergeEveryNSeconds + 1);
    searchRecords(recordsToSearch, indexer, analyzer);

    // Test deleting half of the records that are just inserted
    testDeletion(recordsToSearch, indexer, analyzer);

    // Save the current index
    indexer->save();
    delete indexer;

    // Load the index again
    Indexer *loadedIndexer = Indexer::load(indexMetaData);
    loadedIndexer->createAndStartMergeThreadLoop();

    // Search the loaded index
    searchRecords(recordsToSearch, loadedIndexer, analyzer);

    cout << "Loaded data loaded correctly." << endl;

    // Delete records from the loaded index and search them
    testDeletion(recordsToSearch, loadedIndexer, analyzer);

    cout<<"Large insertion test case passes" << endl;

    // clear memory
    delete loadedIndexer;
    delete analyzer;
    delete schema;
    delete indexMetaData;
}

void testIncrementalUpdateGeoIndex(const string directoryName)
{
    Cache *cache = new Cache(134217728,20000);
    IndexMetaData *indexMetaData = new IndexMetaData(cache,
    		mergeEveryNSeconds, mergeEveryMWrites,
    		updateHistogramEveryPMerges, updateHistogramEveryQWrites,
    		directoryName, "");
    
    // Create a schema
    Schema *schema = Schema::create(LocationIndex);
    schema->setPrimaryKey("id"); // integer, by default not searchable
    schema->setSearchableAttribute("firstAttr", 2); // searchable text
    schema->setSearchableAttribute("secondAttr", 7); // searchable text

    // Create an analyzer
    Analyzer *analyzer = new Analyzer(srch2::instantsearch::DISABLE_STEMMER_NORMALIZER,
    		"", "", "", SYNONYM_DONOT_KEEP_ORIGIN, "");
    Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

    vector< pair<string, pair<string, Point> > > recordsToSearch;

    // Read init data
    readSingleAttrRecordsFromFile(recordsToSearch, indexer, schema, analyzer, directoryName+"/geo_update/900records");

    // Commit the index
    indexer->commit();
    indexer->createAndStartMergeThreadLoop();

    cout << "init data committed" << endl;

    // Search the init data
    searchRecords(recordsToSearch, indexer, analyzer);

    cout << "Init data loaded correctly." << endl;

    // Test inserting records
    testInsertion(recordsToSearch, indexer, schema, analyzer);

    // Test deleting half of the records that are just inserted
    testDeletion(recordsToSearch, indexer, analyzer);

    // Save the current index
    indexer->save();
    delete indexer;
    // Load the index again
    Indexer *loadedIndexer = Indexer::load(indexMetaData);
    loadedIndexer->createAndStartMergeThreadLoop();
    // Search the loaded index
    searchRecords(recordsToSearch, loadedIndexer, analyzer);

    cout << "Loaded data loaded correctly." << endl;

    // Delete records from the loaded index and search them
    testDeletion(recordsToSearch, loadedIndexer, analyzer);

    // Clear the memory
    delete loadedIndexer;
    delete analyzer;
    delete schema;
    delete indexMetaData;

    cout<<"Increment insertion and deletion test cases pass" << endl;

    return;
}

int main(int argc, char *argv[])
{

    bool verbose = false;
    if ( argc > 1 && strcmp(argv[1], "--verbose") == 0) {
        verbose = true;
    }

    // Environment variables
    // directoryName = $SRCH2ROOT/mario/trunk/srch2-engine/test/unit/test_data/geo_update
    // srch2_license_dir = $SRCH2ROOT/mario/trunk/srch2-engine/test/Developer_License
    const string directoryName = getenv("directoryName");

    // Read 900 geo records from a file. For each record, form a query using the first keyword of its "first" attribute and the
    // remaining keywords in this attribute. Run the search and check if the results contain the record ID. The test case passes
    // if each query can find the corresponding record ID.
    testIncrementalUpdateGeoIndex(directoryName);

    testSmallInitLargeInsertion(directoryName);

    return 0;
}
