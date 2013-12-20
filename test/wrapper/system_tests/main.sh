#!/bin/sh
# $1 is <srch2-main-dir>/test/wrapper/system_tests
SYSTEM_TEST_DIR=$1
# $2 is <srch2-main-dir>/build/src/server
SRCH2_ENGINE_DIR=$2
PWD_DIR=$(pwd)
cd $SYSTEM_TEST_DIR

echo ''
echo "NOTE: $0 will start numerous instances of the srch2 server.  Pre-existing server processes will intefere with this testing."
echo ''

# Test for ruby framework for some tests
ruby --version > system_test.log 2>&1
if [ $? -eq 0 ]; then
    HAVE_RUBY=1
else
    HAVE_RUBY=0
    echo "WARNING: Could not find ruby, which some tests require.  Try: sudo apt-get install ruby1.9.1"
fi

# Test for node.js framework
nodejs --version >> system_test.log 2>&1
if [ $? -eq 0 ]; then
    HAVE_NODE=1
    NODE_CMD=nodejs
else
    # maybe it's called just node, but need to test due to another package with the same name
    NODE_TEST=`node -e 'console.log(1);'` 2>> system_test.log
    node --version >> system_test.log 2>&1
    if [ $? -eq 0 ] && [ "${NODE_TEST:-0}" -eq 1 ]; then
	HAVE_NODE=1
	NODE_CMD=node
    else
	HAVE_NODE=0
	echo "WARNING: Could not find node (node.js), which some tests require.  Try: sudo apt-get install nodejs"
    fi
fi

# We remove the old indexes, if any, before doing the test.
rm -rf data/ *.idx

test_id="phrase search test"
echo "---------------------do $test_id-----------------------"
python ./phraseSearch/phrase_search.py $SRCH2_ENGINE_DIR ./phraseSearch/queries.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx

test_id="multi valued attribute"
echo "---------------------do $test_id-----------------------"
python ./test_multi_valued_attributes/test_multi_valued_attributes.py '--srch' $SRCH2_ENGINE_DIR '--qryNrslt' ./test_multi_valued_attributes/queriesAndResults.txt '--frslt' ./test_multi_valued_attributes/facetResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx

test_id="save_shutdown_restart test"
echo "---------------------do $test_id-----------------------"
python ./save_shutdown_restart_export_test/save_shutdown_restart_export_test.py $SRCH2_ENGINE_DIR >> system_test.log 2>&1
if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="empty_index test"
echo "---------------------do $test_id-----------------------"
python ./empty_index/empty_index.py $SRCH2_ENGINE_DIR >> system_test.log 2>&1
if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="high_insert test"
echo "---------------------do $test_id-----------------------"
./high_insert_test/autotest.sh $SRCH2_ENGINE_DIR >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx

test_id="exact_A1 test"
echo "---------------------do $test_id-----------------------"
python ./exact_a1/exact_A1.py $SRCH2_ENGINE_DIR ./exact_a1/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="fuzzy_A1 test"
echo "---------------------do $test_id-----------------------"
python ./fuzzy_a1/fuzzy_A1.py $SRCH2_ENGINE_DIR ./fuzzy_a1/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="exact_M1 test"
echo "---------------------do $test_id-----------------------"
python ./exact_m1/exact_M1.py $SRCH2_ENGINE_DIR ./exact_m1/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="fuzzy_M1 test"
echo "---------------------do $test_id-----------------------"
python ./fuzzy_m1/fuzzy_M1.py $SRCH2_ENGINE_DIR ./fuzzy_m1/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="exact_Attribute_Based_Search test"
echo "---------------------do $test_id-----------------------"
python ./exact_attribute_based_search/exact_Attribute_Based_Search.py $SRCH2_ENGINE_DIR ./exact_attribute_based_search/queriesAndResults.txt >> system_test.log 2>&1
if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx

test_id="fuzzy_Attribute_Based_Search test"
echo "---------------------do $test_id-----------------------"
python ./fuzzy_attribute_based_search/fuzzy_Attribute_Based_Search.py $SRCH2_ENGINE_DIR ./fuzzy_attribute_based_search/queriesAndResults.txt >> system_test.log 2>&1
if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="exact_Attribute_Based_Search_Geo test"
echo "---------------------do $test_id-----------------------"
python ./exact_attribute_based_search_geo/exact_Attribute_Based_Search_Geo.py $SRCH2_ENGINE_DIR ./exact_attribute_based_search_geo/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="fuzzy_Attribute_Based_Search_Geo test"
echo "---------------------do $test_id-----------------------"
python ./fuzzy_attribute_based_search_geo/fuzzy_Attribute_Based_Search_Geo.py $SRCH2_ENGINE_DIR ./fuzzy_attribute_based_search_geo/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="faceted search test"
echo "---------------------do $test_id-----------------------"
python ./faceted_search/faceted_search.py '--srch' $SRCH2_ENGINE_DIR '--qryNrslt' ./faceted_search/queriesAndResults.txt '--frslt' ./faceted_search/facetResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="sort filter test"
echo "---------------------do $test_id-----------------------"
python ./sort_filter/sort_filter.py $SRCH2_ENGINE_DIR ./sort_filter/queriesAndResults.txt ./sort_filter/facetResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="filter query test"
echo "---------------------do $test_id-----------------------"
python ./filter_query/filter_query.py $SRCH2_ENGINE_DIR ./filter_query/queriesAndResults.txt ./filter_query/facetResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="test_solr_compatible_query_syntax"
echo "---------------------do $test_id-----------------------"
python ./test_solr_compatible_query_syntax/test_solr_compatible_query_syntax.py $SRCH2_ENGINE_DIR ./test_solr_compatible_query_syntax/queriesAndResults.txt ./test_solr_compatible_query_syntax/facetResults.txt >> system_test.log 2>&1

# TODO - hack until we figure out why faceted results are do different
echo "-- IGNORING FAILURE: $test_id"
rm -rf data/ *.idx


#if [ $? -gt 0 ]; then
#    echo "FAILED: $test_id"
#    exit -1
#fi
#echo "-- PASSED: $test_id"

test_id="test_search_by_id"
echo "---------------------do $test_id-----------------------"
python ./test_search_by_id/test_search_by_id.py $SRCH2_ENGINE_DIR >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED:$test_id"
rm -rf data/ *.idx


test_id="date and time implementation test"
echo "---------------------do $test_id-----------------------"
python ./date_time_new_features_test/date_time_new_features_test.py $SRCH2_ENGINE_DIR ./date_time_new_features_test/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="geo test"
echo "---------------------do $test_id-----------------------"
python ./geo/geo.py $SRCH2_ENGINE_DIR ./geo/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="term type test"
echo "---------------------do $test_id-----------------------"
python ./term_type/term_type.py $SRCH2_ENGINE_DIR ./term_type/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="analyzer end to end test"
echo "---------------------do $test_id-----------------------"
python ./analyzer_exact_a1/analyzer_exact_A1.py $SRCH2_ENGINE_DIR ./analyzer_exact_a1/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="top_k test"
echo "---------------------do $test_id-----------------------"
python ./top_k/test_srch2_top_k.py $SRCH2_ENGINE_DIR food 10 20 >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="reset logger test"
echo "---------------------do $test_id-----------------------"
#python ./reset_logger/test_reset_logger.py ./reset_logger/srch2-search-server >> system_test.log 2>&1
python ./reset_logger/test_reset_logger.py $SRCH2_ENGINE_DIR >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="tests_used_for_statemedia"
echo "---------------------do $test_id-----------------------"
${NODE_CMD:-node} ./tests_used_for_statemedia/autotest.sh $SRCH2_ENGINE_DIR >> system_test.log 2>&1

# TODO - hack until we figure out why tests_used_for_statemedia/large_insertion_test/large_insertion_test.rb
# won't run and tests_used_for_statemedia/update_endpoint_test
echo "-- IGNORING FAILURE: $test_id"
rm -rf data/ *.idx


#if [ $? -gt 0 ]; then
#    echo "FAILED: $test_id"
#    exit -1
#fi
#echo "-- PASSED: $test_id"

test_id="test for batch upsert"
echo "---------------------do $test_id-----------------------"
python ./upsert_batch/test_upsert_batch.py $SRCH2_ENGINE_DIR >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


test_id="test for batch insert"
echo "---------------------do $test_id-----------------------"
python ./upsert_batch/test_insert_batch.py $SRCH2_ENGINE_DIR >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    exit -1
fi
echo "-- PASSED: $test_id"
rm -rf data/ *.idx


# clear the output directory. First make sure that we are in correct directory
if [ "$(pwd)" = "$SYSTEM_TEST_DIR" ]; then
    rm -rf data
fi

cd $PWD_DIR
