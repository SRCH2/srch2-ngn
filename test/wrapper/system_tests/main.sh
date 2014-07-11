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


test_id="adapter_mongo"
printTestBanner "$test_id"
python ./adapter_mongo/MongoTest.py $SRCH2_ENGINE ./adapter_mongo/queries.txt  | eval "${html_escape_command}" >> system_test.log 2>&1

fun_ret=${PIPESTATUS[0]}
if [ $fun_ret -gt 0 ]; then
    if [ $fun_ret -eq 10 ]; then
        echo "-- SKIPPED: Cannot connect to the MongoDB. Check instructions at https://srch2inc.atlassian.net/browse/SRCN-457 and http://docs.mongodb.org/manual/tutorial/convert-standalone-to-replica-set/ " >> ${output}
    else
        echo "${html_fail_pre}FAILED: $test_id${html_fail_post}" >> ${output}

        if [ $force -eq 0 ]; then
	    exit 255
        fi
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi
rm -rf data/*.idx
rm -rf data/mongodb_data

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
