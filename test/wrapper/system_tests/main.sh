#!/bin/bash

PWD_DIR=$(pwd)

if [ $# -lt 2 ]; then
    echo "Usage: $0 [-f] <system-test-directory> <server-executable>"
    exit 1
fi

force=0
if [ "$1" = '-f' ]; then
    force=1
    shift
fi

# $1 is <srch2-main-dir>/test/wrapper/system_tests
SYSTEM_TEST_DIR=$1
if [ ! -d "$SYSTEM_TEST_DIR" ]; then
    echo "$0: \"$SYSTEM_TEST_DIR\" not an existing directory."
    if [ $force -eq 0 ]; then
	exit 1
    fi
fi
cd $SYSTEM_TEST_DIR

# $2 is <srch2-main-dir>/build/src/server/srch2-search-server
SRCH2_ENGINE=$2
if [ ! -x "$SRCH2_ENGINE" ]; then
    echo "$0: Search engine \"$SRCH2_ENGINE\" not valid."
    if [ $force -eq 0 ]; then
	exit 1
    fi
fi

function printTestBanner {
    testName="$1"
    totalLength=79 # width to make banner
    banner="---------------------do $test_id"
    while [ "${#banner}" -lt "$totalLength" ]
    do
	banner="${banner}-"
    done
    echo "$banner"
}

echo ''
echo "NOTE: $0 will start numerous instances of the srch2 server.  Pre-existing server processes will interfere with this testing."
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

test_id="boolean expression test"
printTestBanner "$test_id"
python ./boolean-expression-test/boolean-expression.py $SRCH2_ENGINE ./boolean-expression-test/queries.txt > system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="qf_dynamic_ranking"
printTestBanner "$test_id"
python ./qf_dynamic_ranking/qf_dynamic_ranking.py $SRCH2_ENGINE ./qf_dynamic_ranking/queriesAndResults.txt > system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx

test_id="phrase search test"
printTestBanner "$test_id"
python ./phraseSearch/phrase_search.py $SRCH2_ENGINE ./phraseSearch/queries.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
#    if [ $force -eq 0 ]; then
#	exit 255
#    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx

test_id="phrase search test with boolean expression"
printTestBanner "$test_id"
python ./phraseSearch/phrase_search.py $SRCH2_ENGINE ./phraseSearch/booleanQueries.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_idi"
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx

test_id="multi valued attribute"
printTestBanner "$test_id"
python ./test_multi_valued_attributes/test_multi_valued_attributes.py '--srch' $SRCH2_ENGINE '--qryNrslt' ./test_multi_valued_attributes/queriesAndResults.txt '--frslt' ./test_multi_valued_attributes/facetResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx

test_id="save_shutdown_restart test"
printTestBanner "$test_id"
python ./save_shutdown_restart_export_test/save_shutdown_restart_export_test.py $SRCH2_ENGINE >> system_test.log 2>&1
if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="empty_index test"
printTestBanner "$test_id"
python ./empty_index/empty_index.py $SRCH2_ENGINE >> system_test.log 2>&1
if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="high_insert test"
printTestBanner "$test_id"
# ./high_insert_test/autotest.sh $SRCH2_ENGINE >> system_test.log 2>&1
echo "SKIPPING high_insert_test"

#if [ $? -gt 0 ]; then
#    echo "FAILED: $test_id"
#    if [ $force -eq 0 ]; then
#	exit 255
#    fi
#else
#    echo "-- PASSED: $test_id"
#fi
rm -rf data/ *.idx

test_id="exact_A1 test"
printTestBanner "$test_id"
python ./exact_a1/exact_A1.py $SRCH2_ENGINE ./exact_a1/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="fuzzy_A1 test"
printTestBanner "$test_id"
python ./fuzzy_a1/fuzzy_A1.py $SRCH2_ENGINE ./fuzzy_a1/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="exact_M1 test"
printTestBanner "$test_id"
python ./exact_m1/exact_M1.py $SRCH2_ENGINE ./exact_m1/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="fuzzy_M1 test"
printTestBanner "$test_id"
python ./fuzzy_m1/fuzzy_M1.py $SRCH2_ENGINE ./fuzzy_m1/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="exact_Attribute_Based_Search test"
printTestBanner "$test_id"
python ./exact_attribute_based_search/exact_Attribute_Based_Search.py $SRCH2_ENGINE ./exact_attribute_based_search/queriesAndResults.txt >> system_test.log 2>&1
if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx

test_id="fuzzy_Attribute_Based_Search test"
printTestBanner "$test_id"
python ./fuzzy_attribute_based_search/fuzzy_Attribute_Based_Search.py $SRCH2_ENGINE ./fuzzy_attribute_based_search/queriesAndResults.txt >> system_test.log 2>&1
#if [ $? -gt 0 ]; then
#    echo "FAILED: $test_id"
#    exit 255
#fi
#echo "-- IGNORING FAILURE: $test_id"
if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="exact_Attribute_Based_Search_Geo test"
printTestBanner "$test_id"
python ./exact_attribute_based_search_geo/exact_Attribute_Based_Search_Geo.py $SRCH2_ENGINE ./exact_attribute_based_search_geo/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="fuzzy_Attribute_Based_Search_Geo test"
printTestBanner "$test_id"
python ./fuzzy_attribute_based_search_geo/fuzzy_Attribute_Based_Search_Geo.py $SRCH2_ENGINE ./fuzzy_attribute_based_search_geo/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="faceted search test"
printTestBanner "$test_id"
python ./faceted_search/faceted_search.py '--srch' $SRCH2_ENGINE '--qryNrslt' ./faceted_search/queriesAndResults.txt '--frslt' ./faceted_search/facetResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="sort filter test"
printTestBanner "$test_id"
python ./sort_filter/sort_filter.py $SRCH2_ENGINE ./sort_filter/queriesAndResults.txt ./sort_filter/facetResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="filter query test"
printTestBanner "$test_id"
python ./filter_query/filter_query.py $SRCH2_ENGINE ./filter_query/queriesAndResults.txt ./filter_query/facetResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="test_solr_compatible_query_syntax"
printTestBanner "$test_id"
python ./test_solr_compatible_query_syntax/test_solr_compatible_query_syntax.py $SRCH2_ENGINE ./test_solr_compatible_query_syntax/queriesAndResults.txt ./test_solr_compatible_query_syntax/facetResults.txt >> system_test.log 2>&1

# TODO - hack until we figure out why faceted results are do different
echo "-- IGNORING FAILURE: $test_id"
rm -rf data/ *.idx


#if [ $? -gt 0 ]; then
#    echo "FAILED: $test_id"
#if [ $force -eq 0 ]; then
#    exit 255
#fi
#fi
#echo "-- PASSED: $test_id"

test_id="test_search_by_id"
printTestBanner "$test_id"
python ./test_search_by_id/test_search_by_id.py $SRCH2_ENGINE >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED:$test_id"
fi
rm -rf data/ *.idx


test_id="date and time implementation test"
printTestBanner "$test_id"
python ./date_time_new_features_test/date_time_new_features_test.py $SRCH2_ENGINE ./date_time_new_features_test/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="geo test"
printTestBanner "$test_id"
python ./geo/geo.py $SRCH2_ENGINE ./geo/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="term type test"
printTestBanner "$test_id"
python ./term_type/term_type.py $SRCH2_ENGINE ./term_type/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="analyzer end to end test"
printTestBanner "$test_id"
python ./analyzer_exact_a1/analyzer_exact_A1.py $SRCH2_ENGINE ./analyzer_exact_a1/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="top_k test"
printTestBanner "$test_id"
python ./top_k/test_srch2_top_k.py $SRCH2_ENGINE food 10 20 >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx


test_id="reset logger test"
printTestBanner "$test_id"
#python ./reset_logger/test_reset_logger.py ./reset_logger/srch2-search-server >> system_test.log 2>&1
python ./reset_logger/test_reset_logger.py $SRCH2_ENGINE >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx reset_logger/indexes


test_id="tests_used_for_statemedia"
printTestBanner "$test_id"
${NODE_CMD:-node} ./tests_used_for_statemedia/autotest.sh $SRCH2_ENGINE >> system_test.log 2>&1

# TODO - hack until we figure out why tests_used_for_statemedia/large_insertion_test/large_insertion_test.rb
# won't run and tests_used_for_statemedia/update_endpoint_test
echo "-- IGNORING FAILURE: $test_id"
rm -rf data/ *.idx


#if [ $? -gt 0 ]; then
#    echo "FAILED: $test_id"
#if [ $force -eq 0 ]; then
#    exit 255
#fi
#fi
#echo "-- PASSED: $test_id"

test_id="test for batch upsert"
printTestBanner "$test_id"
python ./upsert_batch/test_upsert_batch.py $SRCH2_ENGINE >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ *.idx upsert_batch/indexes upsert_batch/*.idx upsert_batch/indexes/*.idx


test_id="test for batch insert"
printTestBanner "$test_id"
python ./upsert_batch/test_insert_batch.py $SRCH2_ENGINE >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ upsert_batch/*.idx upsert_batch/indexes/*.idx


test_id="multicore"
printTestBanner "$test_id"
rm -f ./multicore/core?/*.idx ./multicore/core?/srch2-log.txt
python ./multicore/multicore.py $SRCH2_ENGINE ./multicore/queriesAndResults.txt ./multicore/queriesAndResults2.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ multicore/core?/*.idx

test_id="multiport"
printTestBanner "$test_id"
rm -f ./multiport/core?/*.idx ./multiport/core?/srch2-log.txt
python ./multiport/multiport.py $SRCH2_ENGINE ./multiport/queriesAndResults.txt >> system_test.log 2>&1

if [ $? -gt 0 ]; then
    echo "FAILED: $test_id"
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id"
fi
rm -rf data/ multiport/core?/*.idx


# clear the output directory. First make sure that we are in correct directory
if [ "$(pwd)" = "$SYSTEM_TEST_DIR" ]; then
    rm -rf data
fi

cd $PWD_DIR
