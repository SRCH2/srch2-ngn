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

# This is the general test case function.
# Normally it takes 2 arguments, but sometimes it can also 4 arguments:
#  $1: the name of the test, corresponding to the previous test_id
#  $2: the full command line, like "python ./run_something.py args1, args2". 
#      Note , the full command line should be ONE string
#  $3: [optional], the skipped return code if it is not equal to 0. 
#      For some test cases (e.g. db_connector test cases)
#      we want to skip it if it failed by returning a certain code. 
#  $4: [optional], the skipped the message. 
#      This field works with the $3 argument, if the test case is allowed to skip,
#      we should provide the reason why it failed.
function test_case(){
    test_id="$1"
    printTestBanner "$test_id"
    eval $2 2>&1 | eval "${html_escape_command}" >> system_test.log 2>&1

    ret=${PIPESTATUS[0]}
    if [ $ret -gt 0 ]; then
        if [ $# -gt 2 ] && [ $ret -eq $3 ] ;then
            if [ $# -gt 3 ] ;then
                echo $4
            else
                echo "-- SKIPPED"
            fi
        else
            echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}
            if [ $force -eq 0 ]; then
                exit 255
            fi
        fi
    else
        echo "-- PASSED: $test_id" >> ${output}
    fi
    [ -d "./data" ] && rm -rf data/ 
    for idx in *.idx ; do 
        [ -f $idx ] && rm -f $idx
    done
}

###############################################################################################################
#
#  SYSTEM TEST CASES SHOULD BE ADDED BELOW THIS MESSAGE
#
#  NOTE:  If you decide to add your new test case as a first test case (i.e right after this message), then
#         please be sure to append output using ">> system_test.log".
#
###############################################################################################################

#TODO: disabled for now
#rm -fr SRCH2Cluster
#rm ./attributesAcl/stackoverflow/indexes/*
#rm ./attributesAcl/worldbank/indexes/*
#test_case "attributes ACL" "python ./attributesAcl/testProgram.py $SRCH2_ENGINE"

#TODO: Failed.  FOR SURREDNRA
#rm -fr SRCH2Cluster
#rm ./attributes/indexes/*
#test_case "lot of attributes" "python ./attributes/attributes.py $SRCH2_ENGINE" 

#PASSED
sleep 3
rm -fr SRCH2Cluster
test_case "positional ranking in phrase search" "python ./positionalRanking_phraseSearch/positionalRanking.py $SRCH2_ENGINE ./positionalRanking_phraseSearch/queries.txt"

#TODO: Failed.  Path of synonyms?  Prateek.
#sleep 3
#rm -fr SRCH2Cluster
#test_case "synonyms" "python ./synonyms/synonyms.py $SRCH2_ENGINE" 

#sleep 3

#TODO: disabled for now
#rm -fr SRCH2Cluster
#rm -rf access_control/*Data
#test_case "record-based-ACL" "python ./access_control/record-based-ACL.py $SRCH2_ENGINE ./access_control/queriesAndResults.txt"

#TODO: Failed
sleep 3
rm -fr SRCH2Cluster
test_case "highlighter" "python ./highlight/highlight.py $SRCH2_ENGINE ./highlight/queries.txt"

#passed
sleep 3
rm -fr SRCH2Cluster
test_case "cache_A1" "python ./cache_a1/cache_A1.py $SRCH2_ENGINE ./cache_a1/queriesAndResults.txt" 

#passed
sleep 3
rm -fr SRCH2Cluster
test_case "boolean expression" "python ./boolean-expression-test/boolean-expression.py $SRCH2_ENGINE ./boolean-expression-test/queries.txt" 

# qf disabled for now - will fail until we integrate with Jamshid's boolean expression changes
#test_case "qf_dynamic_ranking" "./qf_dynamic_ranking/qf_dynamic_ranking.py $SRCH2_ENGINE ./qf_dynamic_ranking/queriesAndResults.txt"
#printTestBanner "$test_id"

#passed
sleep 3
rm -fr SRCH2Cluster
test_case "phrase search" "python ./phraseSearch/phrase_search.py $SRCH2_ENGINE ./phraseSearch/queries.txt" 

#passed
sleep 3
rm -fr SRCH2Cluster
test_case "phrase search with boolean expression" "python ./phraseSearch/phrase_search.py $SRCH2_ENGINE ./phraseSearch/booleanQueries.txt" 


#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "multi valued attribute" "python ./test_multi_valued_attributes/test_multi_valued_attributes.py '--srch2' $SRCH2_ENGINE '--qryNrslt' \
./test_multi_valued_attributes/queriesAndResults.txt '--frslt' ./test_multi_valued_attributes/facetResults.txt"

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "save_shutdown_restart" "python ./save_shutdown_restart_export_test/save_shutdown_restart_export_test.py $SRCH2_ENGINE" 

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "empty_index" "python ./empty_index/empty_index.py $SRCH2_ENGINE"

#passed
sleep 3
rm -fr SRCH2Cluster
test_case "exact_A1" "python ./exact_a1/exact_A1.py $SRCH2_ENGINE ./exact_a1/queriesAndResults.txt" 

#passed
sleep 3
rm -fr SRCH2Cluster
test_case "fuzzy_A1" "python ./fuzzy_a1/fuzzy_A1.py $SRCH2_ENGINE ./fuzzy_a1/queriesAndResults.txt" 

#passed
sleep 3
rm -fr SRCH2Cluster
test_case "fuzzy_A1_swap test" "python ./fuzzy_a1_swap/fuzzy_A1_swap.py $SRCH2_ENGINE ./fuzzy_a1_swap/queriesAndResults.txt"

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "exact_M1" "python ./exact_m1/exact_M1.py $SRCH2_ENGINE ./exact_m1/queriesAndResults.txt" 

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "fuzzy_M1" "python ./fuzzy_m1/fuzzy_M1.py $SRCH2_ENGINE ./fuzzy_m1/queriesAndResults.txt" 

#TODO: failed (earlier queries passed)
sleep 3
rm -fr SRCH2Cluster
test_case "exact_Attribute_Based_Search" "python ./exact_attribute_based_search/exact_Attribute_Based_Search.py $SRCH2_ENGINE ./exact_attribute_based_search/queriesAndResults.txt" 

#passed
sleep 3
rm -fr SRCH2Cluster
test_case "fuzzy_Attribute_Based_Search" "python ./fuzzy_attribute_based_search/fuzzy_Attribute_Based_Search.py $SRCH2_ENGINE ./fuzzy_attribute_based_search/queriesAndResults.txt"

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "exact_Attribute_Based_Search_Geo" "python ./exact_attribute_based_search_geo/exact_Attribute_Based_Search_Geo.py $SRCH2_ENGINE ./exact_attribute_based_search_geo/queriesAndResults.txt"

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "fuzzy_Attribute_Based_Search_Geo" "python ./fuzzy_attribute_based_search_geo/fuzzy_Attribute_Based_Search_Geo.py $SRCH2_ENGINE ./fuzzy_attribute_based_search_geo/queriesAndResults.txt"

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "faceted search" "python ./faceted_search/faceted_search.py '--srch' $SRCH2_ENGINE '--qryNrslt' ./faceted_search/queriesAndResults.txt '--frslt' ./faceted_search/facetResults.txt"

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "sort filter" "python ./sort_filter/sort_filter.py $SRCH2_ENGINE ./sort_filter/queriesAndResults.txt ./sort_filter/facetResults.txt" 

#TODO: failed
sleep 4
rm -fr SRCH2Cluster
test_case "filter query" "python ./filter_query/filter_query.py $SRCH2_ENGINE ./filter_query/queriesAndResults.txt ./filter_query/facetResults.txt"

#TODO: failed
sleep 4
rm -fr SRCH2Cluster
test_case "test_solr_compatible_query_syntax" "python ./test_solr_compatible_query_syntax/test_solr_compatible_query_syntax.py $SRCH2_ENGINE \
    ./test_solr_compatible_query_syntax/queriesAndResults.txt ./test_solr_compatible_query_syntax/facetResults.txt"

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "test_search_by_id" "python ./test_search_by_id/test_search_by_id.py $SRCH2_ENGINE" 

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "date and time implementation" "python ./date_time_new_features_test/date_time_new_features_test.py $SRCH2_ENGINE ./date_time_new_features_test/queriesAndResults.txt" 

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "geo" "python ./geo/geo.py $SRCH2_ENGINE ./geo/queriesAndResults.txt" 

#TODO: failed (earlier queries passed)
sleep 3
rm -fr SRCH2Cluster
test_case "term type" "python ./term_type/term_type.py $SRCH2_ENGINE ./term_type/queriesAndResults.txt"

# passed
sleep 3
rm -fr SRCH2Cluster
test_case "analyzer end to end" "python ./analyzer_exact_a1/analyzer_exact_A1.py $SRCH2_ENGINE ./analyzer_exact_a1/queriesAndResults.txt"

# passed
sleep 3
rm -fr SRCH2Cluster
test_case "top_k" "python ./top_k/test_srch2_top_k.py $SRCH2_ENGINE food 10 20"

#passed
sleep 3
rm -fr SRCH2Cluster
test_case "reset logger" "python ./reset_logger/test_reset_logger.py $SRCH2_ENGINE"
rm -rf data/ *.idx reset_logger/indexes

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "batch upsert" "python ./upsert_batch/test_upsert_batch.py $SRCH2_ENGINE" 
rm -rf data/ *.idx upsert_batch/indexes upsert_batch/*.idx upsert_batch/indexes/*.idx

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "batch insert" "python ./upsert_batch/test_insert_batch.py $SRCH2_ENGINE"
rm -rf data/ upsert_batch/*.idx upsert_batch/indexes/*.idx

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
rm -f ./multicore/core?/*.idx ./multicore/core?/srch2-log.txt
test_case "multicore" "python ./multicore/multicore.py $SRCH2_ENGINE ./multicore/queriesAndResults.txt ./multicore/queriesAndResults2.txt"
rm -rf data/ multicore/core?/*.idx

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
rm -f ./multiport/core?/*.idx ./multiport/core?/srch2-log.txt
test_case "multiport" "python ./multiport/multiport.py $SRCH2_ENGINE ./multiport/queriesAndResults.txt"
rm -rf data/ multiport/core?/*.idx

#passed
sleep 3
rm -fr SRCH2Cluster
test_case "authorization" "python ./authorization/authorization.py $SRCH2_ENGINE ./authorization/queriesAndResults.txt"

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "test loading different schema" "python ./test_load_diff_schema/test_load_diff_schema.py $SRCH2_ENGINE"

#passed
sleep 3
rm -fr SRCH2Cluster
test_case "refining attribute type" "python ./refining_attr_type/refining_attr_type.py $SRCH2_ENGINE"

#TODO: failed (earlier queries passed)
sleep 3
rm -fr SRCH2Cluster
test_case "primary key - refining field" "python ./refining_field_primary_key/testPrimaryKey.py $SRCH2_ENGINE ./refining_field_primary_key/queriesAndResults.txt"

#passed
sleep 3
rm -fr SRCH2Cluster
test_case "run engine with missing parameters from config file" "python ./missing_parameters_from_cm/missingParameters_config.py $SRCH2_ENGINE ./missing_parameters_from_cm/queriesAndResults.txt"

#passed
sleep 3
rm -fr SRCH2Cluster
test_case "empty record boost field" "python ./empty_recordBoostField/empty_recordBoostField.py $SRCH2_ENGINE ./empty_recordBoostField/queriesAndResults.txt"

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "heart_beat_test"  "python ./heartbeat/heart_beat.py $SRCH2_ENGINE"

#TODO: failed (earlier queries passed)
sleep 3
rm -fr SRCH2Cluster
test_case "test field list parameter in query" "python ./test_fieldList_inQuery/test_fieldList.py $SRCH2_ENGINE ./test_fieldList_inQuery/queriesAndResults.txt"

#TODO: failed
sleep 3
rm -fr SRCH2Cluster
test_case "validate json response" "python ./json_response/json_response_format_test.py $SRCH2_ENGINE"

#passed
sleep 3
rm -fr SRCH2Cluster
test_case "test Chinese" "python ./chinese/chinese_analyzer.py $SRCH2_ENGINE"

#TODO: disabled for now
#sleep 3
#rm -fr SRCH2Cluster
#test_case "adapter_mysql" "python ./adapter_mysql/adapter_mysql.py $SRCH2_ENGINE \
#    ./adapter_sqlite/testCreateIndexes_sql.txt ./adapter_sqlite/testCreateIndexes.txt \
#    ./adapter_sqlite/testRunListener_sql.txt ./adapter_sqlite/testRunListener.txt \
#    ./adapter_sqlite/testOfflineLog_sql.txt ./adapter_sqlite/testOfflineLog.txt" \
#    255 "-- SKIPPED: Cannot connect to the MySQL. Check if MySQL is installed."

#sleep 3
#rm -fr SRCH2Cluster
#test_case "adapter_mysql_recover" "python ./adapter_mysql/adapter_mysql_recover.py $SRCH2_ENGINE \
#    ./adapter_sqlite/testCreateIndexes_sql.txt ./adapter_sqlite/testCreateIndexes.txt \
#    ./adapter_sqlite/testRunListener_sql.txt ./adapter_sqlite/testRunListener.txt \
#    ./adapter_sqlite/testOfflineLog_sql.txt ./adapter_sqlite/testOfflineLog.txt" \
#    255 "-- SKIPPED: Cannot connect to the MySQL. Check if MySQL is installed."

#sleep 3
#rm -fr SRCH2Cluster
#test_case "adapter_sqlite" "python ./adapter_sqlite/adapter_sqlite.py $SRCH2_ENGINE \
#    ./adapter_sqlite/testCreateIndexes_sql.txt ./adapter_sqlite/testCreateIndexes.txt \
#    ./adapter_sqlite/testRunListener_sql.txt ./adapter_sqlite/testRunListener.txt \
#    ./adapter_sqlite/testOfflineLog_sql.txt ./adapter_sqlite/testOfflineLog.txt" \
#    255 "-- SKIPPED: Cannot connect to the Sqlite. Check if sqlite3 is installed."

#sleep 3
#rm -fr SRCH2Cluster
#test_case "adapter_sqlite_recover" "python ./adapter_sqlite/adapter_sqlite_recover.py $SRCH2_ENGINE \
#    ./adapter_sqlite/testCreateIndexes_sql.txt ./adapter_sqlite/testCreateIndexes.txt \
#    ./adapter_sqlite/testRunListener_sql.txt ./adapter_sqlite/testRunListener.txt \
#    ./adapter_sqlite/testOfflineLog_sql.txt ./adapter_sqlite/testOfflineLog.txt" \
#    255 "-- SKIPPED: Cannot connect to the Sqlite. Check if sqlite3 is installed."

# The following cases may not run on Mac, so we put them to the end
#
#if [ $os != "$macName" ];then
#    test_case "high_insert" "./high_insert_test/autotest.sh $SRCH2_ENGINE" 
#else
#    echo "-- IGNORING high_insert test on $macName" >> ${output}
#fi

#sleep 3
#rm -fr SRCH2Cluster
#test_case "adapter_mongo" "python ./adapter_mongo/adapter_mongo.py $SRCH2_ENGINE \
#    ./adapter_mongo/testCreateIndexes_sql.txt ./adapter_mongo/testCreateIndexes.txt \
#    ./adapter_mongo/testRunListener_sql.txt ./adapter_mongo/testRunListener.txt \
#    ./adapter_mongo/testOfflineLog_sql.txt ./adapter_mongo/testOfflineLog.txt" \
#    255 "-- SKIPPED: Cannot connect to the MongoDB. \
#    Check instructions in the file db_connectors/mongo/readme.txt. "

#sleep 3
#rm -fr SRCH2Cluster
#test_case "adapter_mongo_recover" "python ./adapter_mongo/adapter_mongo_recover.py  $SRCH2_ENGINE \
#    ./adapter_mongo/testCreateIndexes_sql.txt ./adapter_mongo/testCreateIndexes.txt \
#    ./adapter_mongo/testRunListener_sql.txt ./adapter_mongo/testRunListener.txt \
#    ./adapter_mongo/testOfflineLog_sql.txt ./adapter_mongo/testOfflineLog.txt" \
#    255 "-- SKIPPED: Cannot connect to the MongoDB. \
#    Check instructions in the file db_connectors/mongo/readme.txt. "

# server is a little slow to exit for reset_logger, causing the server in statemedia's first test (write_correctness)
# to fail to bind the port, hanging the test script, so wait just a sec here
#sleep 2
#rm -fr SRCH2Cluster
#rm -rf data/tests_used_for_statemedia
#if [ $HAVE_NODE -gt 0 ]; then
#
#    if [ $HAVE_RUBY -eq 0 ]; then
#	echo "-- ruby NOT INSTALLED - SKIPPING large_insertion component of ${test_id}" >> ${output}
#    fi
#
#    if [ `uname -s` != 'Darwin' ]; then
#        test_case "tests_used_for_statemedia" "NODECMD=${NODE_CMD:-node} ./tests_used_for_statemedia/autotest.sh $SRCH2_ENGINE" 
#    else
#        echo "-- IGNORING tests_used_for_statemedia on MacOS"
#    fi
#else
#    echo "-- node.js NOT INSTALLED - SKIPPING: ${test_id}" >> ${output}
#fi

# TODO - hack until we figure out why tests_used_for_statemedia/large_insertion_test/large_insertion_test.rb
# won't run and tests_used_for_statemedia/update_endpoint_test
#echo "-- IGNORING FAILURE: $test_id" >> ${output}
#rm -rf data/ *.idx

# clear the output directory. First make sure that we are in correct directory
#if [ "$(pwd)" = "$SYSTEM_TEST_DIR" ]; then
#    rm -rf data
#fi
#
#if [ "$html" -gt 0 ]; then
#    echo '<h2>Individual Test Results - Log Output</h2>' >> ${output}
#    echo '<pre>' >> ${output}
#    cat system_test.log >> ${output}
#    echo '</pre>' >> ${output}
#    echo '</body></html>' >> ${output}
#fi

#if [ "$upload" != '' ]; then
#    eval $upload
#fi


cd $PWD_DIR
