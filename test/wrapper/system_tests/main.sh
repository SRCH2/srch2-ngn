#!/bin/bash

PWD_DIR=$(pwd)

if [ $# -lt 2 ]; then
    echo "Usage: $0 [-f] <system-test-directory> <server-executable>"
    exit 1
fi

machine=`uname -m`
os=`uname -s`
macName='Darwin'

# process options
force=0
output='/dev/tty'
html=0
upload=''
while [ "$#" -gt 2 ]
do
    # -f or --force option: continue executing tests even after one fails
    if [ "$1" = '-f' ] || [ "$1" = '--force' ]; then
	force=1
	shift
	continue
    fi

    # --html option: Format output in HTML (for dashboard)
    if [ "$1" = '--html' ]; then
	html=1
	shift
	continue
    fi

    # --outfile option: Where to send output
    # required if also specifying --upload
    if [ "$1" = '-o' ] || [ "$1" = '--output' ] || [ "$1" = '--output-file' ]; then
	output="$2"
	shift 2
	continue
    fi

    # --upload option: Upload output file to dashboard server
    # This curl command uses Content-Type: multipart/form-data
    if [ "$1" = '--upload' ]; then
	if [ "$output" = '/dev/tty' ]; then
	    output="report-`date +%F-%R`-${machine}-${os}.html"
	fi
	upload="curl -F \"uploadedfile=@${output}\" \"http://dilbert.calit2.uci.edu/CDash-2-0-2/system_tests_submit.php\""
	shift
	continue
    fi

    echo "$0: Unrecognized option: $1"
    break;
done

# $1 is <srch2-main-dir>/test/wrapper/system_tests
SYSTEM_TEST_DIR=$1
if [ ! -d "$SYSTEM_TEST_DIR" ]; then
    echo "$0: \"$SYSTEM_TEST_DIR\" not an existing directory."
    exit 1
fi
cd $SYSTEM_TEST_DIR

# $2 is <srch2-main-dir>/build/src/server/srch2-search-server
SRCH2_ENGINE=$2
if [ ! -x "$SRCH2_ENGINE" ]; then
    echo "$0: Search engine \"$SRCH2_ENGINE\" not valid."
    exit 1
fi

# Turn off Python stdout buffering so we don't lose output messages
PYTHONUNBUFFERED=True
export PYTHONUNBUFFERED

function printTestBanner {
    testName="$1"
    totalLength=79 # width to make banner
    if [ "$html" -gt 0 ]; then
	html_pre='<h4>'
	html_post='</h4>'
	html_results_link_pre="<a href=\"#${testName}\">"
	html_results_link_post='</a>'
	html_results_anchor="<a name=\"${testName}\"></a>"
	totalLength=$((${totalLength} + ${#html_pre} + ${#html_post} + ${#html_results_link_pre} + ${#html_results_link_post}))
    else
	html_pre=''
	html_post=''
	html_results_link_pre=''
	html_results_link_post=''
	html_results_anchor_pre=''
	html_results_anchor_post=''
    fi
    banner="${html_pre}---------------------do ${html_results_link_pre}${test_id}${html_results_link_post} test"
    while [ "${#banner}" -lt "$totalLength" ]
    do
	banner="${banner}-"
    done
    banner="${banner}${html_post}"
    echo "$banner" >> ${output}
    echo "${html_results_anchor}${banner}" >> system_test.log
}

# Clear output file if file was specified
if [ -f "$output" ]; then
    rm -f "$output"
fi

# setup some common HTML highlighting
if [ "$html" -gt 0 ]; then
    html_fail_pre='<font color="#FF0000">'
    html_fail_post='</font>'

    rm -f system_test.log
    html_title="System Tests - `date` - ${machine} ${os}"
    echo "<html><head><title>${html_title}</title></head><body><h1>${html_title}</h1><h2>Test Summary</h2>" >> ${output}
    html_escape_command="sed -e 's/</\&lt;/g' | sed -e 's/>/\&gt;/g'"
else
    html_fail_pre=''
    html_fail_post=''
    html_escape_command='cat'
fi

echo ''
echo "NOTE: $0 will start numerous instances of the srch2 server.  Pre-existing server processes will interfere with this testing."
echo ''

# Test for ruby framework for some tests
ruby --version >> system_test.log 2>&1
if [ $? -eq 0 ]; then
    HAVE_RUBY=1
else
    HAVE_RUBY=0
    echo "WARNING: Could not find ruby, which some tests require.  Try: sudo apt-get install ruby1.9.1" >> ${output}
fi

# Test for node.js framework
echo 'Checking node nodejs executable' >> system_test.log 2>&1
nodejs --version >> system_test.log 2>&1
if [ $? -eq 0 ]; then
    HAVE_NODE=1
    NODE_CMD=nodejs
else
    # maybe it's called just node, but need to test due to another package with the same name
    echo 'Checking for nodejs executable as just node' >> system_test.log 2>&1
    NODE_TEST=`node -e 'console.log(1);'` 2>> system_test.log
    node --version >> system_test.log 2>&1
    if [ $? -eq 0 ] && [ "${NODE_TEST:-0}" -eq 1 ]; then
	HAVE_NODE=1
	NODE_CMD=node
    else
	HAVE_NODE=0
	echo "WARNING: Could not find node (node.js), which some tests require.  Try: sudo apt-get install nodejs" >> ${output}
    fi
fi

# We remove the old indexes, if any, before doing the test.
rm -rf data/ *.idx

###############################################################################################################
#
#  SYSTEM TEST CASES SHOULD BE ADDED BELOW THIS MESSAGE
#
#  NOTE:  If you decide to add your new test case as a first test case (i.e right after this message), then
#         please be sure to append output using ">> system_test.log".
#
###############################################################################################################
test_id="lot of attributes"
printTestBanner "$test_id"
rm ./attributes/indexes/*
python ./attributes/attributes.py $SRCH2_ENGINE | eval "${html_escape_command}" >> system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
    exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi

test_id="synonyms"
printTestBanner "$test_id"
python ./synonyms/synonyms.py $SRCH2_ENGINE | eval "${html_escape_command}" >> system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi

test_id="highlighter"
printTestBanner "$test_id"
python ./highlight/highlight.py $SRCH2_ENGINE ./highlight/queries.txt  | eval "${html_escape_command}" >> system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx

test_id="cache_A1"
printTestBanner "$test_id"
python ./cache_a1/cache_A1.py $SRCH2_ENGINE ./cache_a1/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="boolean expression"
printTestBanner "$test_id"
python ./boolean-expression-test/boolean-expression.py $SRCH2_ENGINE ./boolean-expression-test/queries.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="qf_dynamic_ranking"
# qf disabled for now - will fail until we integrate with Jamshid's boolean expression changes
#printTestBanner "$test_id"
#python ./qf_dynamic_ranking/qf_dynamic_ranking.py $SRCH2_ENGINE ./qf_dynamic_ranking/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

#if [ ${PIPESTATUS[0]} -gt 0 ]; then
#    echo "${html_fail_pre}IGNORING FAILED: $test_id${html_fail_post}" >> ${output}
#    if [ $force -eq 0 ]; then
#	exit 255
#    fi
#else
#    echo "-- PASSED: $test_id" >> ${output}
#fi
#rm -rf data/ *.idx

test_id="phrase search"
printTestBanner "$test_id"
python ./phraseSearch/phrase_search.py $SRCH2_ENGINE ./phraseSearch/queries.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
#    if [ $force -eq 0 ]; then
#	exit 255
#    fi
    echo "-- IGNORING FAILURE: $test_id" >> ${output}
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx

test_id="phrase search with boolean expression"
printTestBanner "$test_id"
python ./phraseSearch/phrase_search.py $SRCH2_ENGINE ./phraseSearch/booleanQueries.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_idi${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx

test_id="multi valued attribute"
printTestBanner "$test_id"
python ./test_multi_valued_attributes/test_multi_valued_attributes.py '--srch' $SRCH2_ENGINE '--qryNrslt' ./test_multi_valued_attributes/queriesAndResults.txt '--frslt' ./test_multi_valued_attributes/facetResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx

test_id="save_shutdown_restart"
printTestBanner "$test_id"
python ./save_shutdown_restart_export_test/save_shutdown_restart_export_test.py $SRCH2_ENGINE | eval "${html_escape_command}" >> system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="empty_index"
printTestBanner "$test_id"
python ./empty_index/empty_index.py $SRCH2_ENGINE | eval "${html_escape_command}" >> system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


if [ $os != "$macName" ];then
    test_id="high_insert"
    printTestBanner "$test_id"
    ./high_insert_test/autotest.sh $SRCH2_ENGINE | eval "${html_escape_command}" >> system_test.log 2>&1
    if [ ${PIPESTATUS[0]} -gt 0 ]; then
        echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
    else
        echo "-- PASSED: $test_id" >> ${output}
    fi
    rm -rf data/ *.idx
else
    echo "-- IGNORING high_insert test on $macName" >> ${output}
fi

test_id="exact_A1"
printTestBanner "$test_id"
python ./exact_a1/exact_A1.py $SRCH2_ENGINE ./exact_a1/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="fuzzy_A1"
printTestBanner "$test_id"
python ./fuzzy_a1/fuzzy_A1.py $SRCH2_ENGINE ./fuzzy_a1/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx

test_id="fuzzy_A1_swap test"
printTestBanner "$test_id"
python ./fuzzy_a1_swap/fuzzy_A1_swap.py $SRCH2_ENGINE ./fuzzy_a1_swap/queriesAndResults.txt >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "FAILED: $test_id" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx



test_id="exact_M1"
printTestBanner "$test_id"
python ./exact_m1/exact_M1.py $SRCH2_ENGINE ./exact_m1/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="fuzzy_M1"
printTestBanner "$test_id"
python ./fuzzy_m1/fuzzy_M1.py $SRCH2_ENGINE ./fuzzy_m1/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="exact_Attribute_Based_Search"
printTestBanner "$test_id"
python ./exact_attribute_based_search/exact_Attribute_Based_Search.py $SRCH2_ENGINE ./exact_attribute_based_search/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx

test_id="fuzzy_Attribute_Based_Search"
printTestBanner "$test_id"
python ./fuzzy_attribute_based_search/fuzzy_Attribute_Based_Search.py $SRCH2_ENGINE ./fuzzy_attribute_based_search/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="exact_Attribute_Based_Search_Geo"
printTestBanner "$test_id"
python ./exact_attribute_based_search_geo/exact_Attribute_Based_Search_Geo.py $SRCH2_ENGINE ./exact_attribute_based_search_geo/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="fuzzy_Attribute_Based_Search_Geo"
printTestBanner "$test_id"
python ./fuzzy_attribute_based_search_geo/fuzzy_Attribute_Based_Search_Geo.py $SRCH2_ENGINE ./fuzzy_attribute_based_search_geo/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="faceted search"
printTestBanner "$test_id"
python ./faceted_search/faceted_search.py '--srch' $SRCH2_ENGINE '--qryNrslt' ./faceted_search/queriesAndResults.txt '--frslt' ./faceted_search/facetResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="sort filter"
printTestBanner "$test_id"
python ./sort_filter/sort_filter.py $SRCH2_ENGINE ./sort_filter/queriesAndResults.txt ./sort_filter/facetResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="filter query"
printTestBanner "$test_id"
python ./filter_query/filter_query.py $SRCH2_ENGINE ./filter_query/queriesAndResults.txt ./filter_query/facetResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="test_solr_compatible_query_syntax"
printTestBanner "$test_id"
python ./test_solr_compatible_query_syntax/test_solr_compatible_query_syntax.py $SRCH2_ENGINE ./test_solr_compatible_query_syntax/queriesAndResults.txt ./test_solr_compatible_query_syntax/facetResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


#if [ ${PIPESTATUS[0]} -gt 0 ]; then
#    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
#if [ $force -eq 0 ]; then
#    exit 255
#fi
#fi
#echo "-- PASSED: $test_id" >> ${output}

test_id="test_search_by_id"
printTestBanner "$test_id"
python ./test_search_by_id/test_search_by_id.py $SRCH2_ENGINE | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED:$test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="date and time implementation"
printTestBanner "$test_id"
python ./date_time_new_features_test/date_time_new_features_test.py $SRCH2_ENGINE ./date_time_new_features_test/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="geo"
printTestBanner "$test_id"
python ./geo/geo.py $SRCH2_ENGINE ./geo/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="term type"
printTestBanner "$test_id"
python ./term_type/term_type.py $SRCH2_ENGINE ./term_type/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="analyzer end to end"
printTestBanner "$test_id"
python ./analyzer_exact_a1/analyzer_exact_A1.py $SRCH2_ENGINE ./analyzer_exact_a1/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="top_k"
printTestBanner "$test_id"
python ./top_k/test_srch2_top_k.py $SRCH2_ENGINE food 10 20 | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


test_id="reset logger"
printTestBanner "$test_id"
python ./reset_logger/test_reset_logger.py $SRCH2_ENGINE | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
#    if [ $force -eq 0 ]; then
#	exit 255
#    fi
    echo "-- IGNORING FAILURE: $test_id" >> ${output}
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx reset_logger/indexes


test_id="tests_used_for_statemedia"
printTestBanner "$test_id"
# server is a little slow to exit for reset_logger, causing the server in statemedia's first test (write_correctness)
# to fail to bind the port, hanging the test script, so wait just a sec here
sleep 1
rm -rf data/tests_used_for_statemedia
if [ $HAVE_NODE -gt 0 ]; then

    if [ $HAVE_RUBY -eq 0 ]; then
	echo "-- ruby NOT INSTALLED - SKIPPING large_insertion component of ${test_id}" >> ${output}
    fi

    if [ `uname -s` != 'Darwin' ]; then
        NODECMD=${NODE_CMD:-node} ./tests_used_for_statemedia/autotest.sh $SRCH2_ENGINE | eval "${html_escape_command}" >> system_test.log 2>&1

        if [ ${PIPESTATUS[0]} -gt 0 ]; then
	    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
            if [ $force -eq 0 ]; then
                exit 255
	    fi
        else
	    echo "-- PASSED: $test_id" >> ${output}
        fi
    else
        echo "-- IGNORING $test_id on MacOS"
    fi
else
    echo "-- node.js NOT INSTALLED - SKIPPING: ${test_id}" >> ${output}
fi

# TODO - hack until we figure out why tests_used_for_statemedia/large_insertion_test/large_insertion_test.rb
# won't run and tests_used_for_statemedia/update_endpoint_test
#echo "-- IGNORING FAILURE: $test_id" >> ${output}

rm -rf data/ *.idx


test_id="batch upsert"
printTestBanner "$test_id"
python ./upsert_batch/test_upsert_batch.py $SRCH2_ENGINE | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx upsert_batch/indexes upsert_batch/*.idx upsert_batch/indexes/*.idx


test_id="batch insert"
printTestBanner "$test_id"
python ./upsert_batch/test_insert_batch.py $SRCH2_ENGINE | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ upsert_batch/*.idx upsert_batch/indexes/*.idx


test_id="multicore"
printTestBanner "$test_id"
rm -f ./multicore/core?/*.idx ./multicore/core?/srch2-log.txt
python ./multicore/multicore.py $SRCH2_ENGINE ./multicore/queriesAndResults.txt ./multicore/queriesAndResults2.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ multicore/core?/*.idx

test_id="multiport"
printTestBanner "$test_id"
rm -f ./multiport/core?/*.idx ./multiport/core?/srch2-log.txt
python ./multiport/multiport.py $SRCH2_ENGINE ./multiport/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
	exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ multiport/core?/*.idx

test_id="authentication"
printTestBanner "$test_id"
python ./authorization/authorization.py $SRCH2_ENGINE ./authorization/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx

test_id="test loading different schema"
printTestBanner "$test_id"
python ./test_load_diff_schema/test_load_diff_schema.py $SRCH2_ENGINE  | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx

test_id="refining attribute type"
printTestBanner "$test_id"
python ./refining_attr_type/refining_attr_type.py $SRCH2_ENGINE  | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx

test_id="primary key - refining field"
printTestBanner "$test_id"
python ./refining_field_primary_key/testPrimaryKey.py $SRCH2_ENGINE ./refining_field_primary_key/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx

test_id="run engine with missing parameters from config file"
printTestBanner "$test_id"
python ./missing_parameters_from_cm/missingParameters_config.py $SRCH2_ENGINE ./missing_parameters_from_cm/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx

test_id="empty record boost field"
printTestBanner "$test_id"
python ./empty_recordBoostField/empty_recordBoostField.py $SRCH2_ENGINE ./empty_recordBoostField/queriesAndResults.txt | eval "${html_escape_command}" >> system_test.log 2>&1

if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/ *.idx


# clear the output directory. First make sure that we are in correct directory
if [ "$(pwd)" = "$SYSTEM_TEST_DIR" ]; then
    rm -rf data
fi

if [ "$html" -gt 0 ]; then
    echo '<h2>Individual Test Results - Log Output</h2>' >> ${output}
    echo '<pre>' >> ${output}
    cat system_test.log >> ${output}
    echo '</pre>' >> ${output}
    echo '</body></html>' >> ${output}
fi

if [ "$upload" != '' ]; then
    eval $upload
fi

cd $PWD_DIR
