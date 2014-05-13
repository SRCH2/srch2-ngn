#!/bin/bash

PWD_DIR=$(pwd)

if [ $# -lt 2 ]; then
    echo "Usage: $0 [-f] <system-test-directory> <server-executable>"
    exit 1
fi

machine=`uname -m`
os=`uname -s`
macName='Darwin'
LOG_DIRECTORY='./test-data/core1/logs'
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
    echo "${html_results_anchor}${banner}" >> ./test-data/core1/logs/system_test.log
}

# Clear output file if file was specified
if [ -f "$output" ]; then
    rm -f "$output"
fi

# setup some common HTML highlighting
if [ "$html" -gt 0 ]; then
    html_fail_pre='<font color="#FF0000">'
    html_fail_post='</font>'

    rm -f ./test-data/core1/logs/system_test.log
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

mkdir -p $LOG_DIRECTORY

# python ./2n-insertA-deleteA/test-sharding.py ../../../build/src/server/srch2-search-server ./2n-insertA-deleteA/2n-insertA-deleteA.txt ./2n-insertA-deleteA/list-of-2-nodes.txt
test_id="2n-insertA-deleteA"
printTestBanner "$test_id"
python ./2n-insertA-deleteA/test-sharding.py $SRCH2_ENGINE ./2n-insertA-deleteA/2n-insertA-deleteA.txt ./2n-insertA-deleteA/list-of-2-nodes.txt | eval "${html_escape_command}" >> ./test-data/core1/logs/system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}-- FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi

test_id="2n-insertA-deleteB"
printTestBanner "$test_id"
python ./AllTestCases/test-sharding.py $SRCH2_ENGINE ./AllTestCases/2n-insertA-deleteB.txt ./AllTestCases/list-of-2-nodes.txt | eval "${html_escape_command}" >> ./test-data/core1/logs/system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}-- FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi

test_id="2n-insertA-R1-B-R2-deleteA-R1-B-R2"
printTestBanner "$test_id"
python ./AllTestCases/test-sharding.py $SRCH2_ENGINE ./AllTestCases/2n-insertA-R1-B-R2-deleteA-R1-B-R2.txt ./AllTestCases/list-of-2-nodes.txt | eval "${html_escape_command}" >> ./test-data/core1/logs/system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}-- FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi

test_id="2n-insertA-R1-B-R2-deleteB-R1-A-R1"
printTestBanner "$test_id"
python ./AllTestCases/test-sharding.py $SRCH2_ENGINE ./AllTestCases/2n-insertA-R1-B-R2-deleteB-R1-A-R1.txt ./AllTestCases/list-of-2-nodes.txt | eval "${html_escape_command}" >> ./test-data/core1/logs/system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}-- FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi

test_id="2n-insertA-R1-B-R2-deleteB-R1-queryA-R1"
printTestBanner "$test_id"
python ./AllTestCases/test-sharding.py $SRCH2_ENGINE ./AllTestCases/2n-insertA-R1-B-R2-deleteB-R1-queryA-R1.txt ./AllTestCases/list-of-2-nodes.txt | eval "${html_escape_command}" >> ./test-data/core1/logs/system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}-- FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi

test_id="./AllTestCases/2n-insertAB-queryAB"
printTestBanner "$test_id"
python ./AllTestCases/test-sharding.py $SRCH2_ENGINE ./AllTestCases/2n-insertAB-queryAB.txt ./AllTestCases/list-of-2-nodes.txt | eval "${html_escape_command}" >> ./test-data/core1/logs/system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}-- FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi

test_id="./AllTestCases/2n-insertAABB"
printTestBanner "$test_id"
python ./AllTestCases/test-sharding.py $SRCH2_ENGINE ./AllTestCases/2n-insertAABB.txt ./AllTestCases/list-of-2-nodes.txt | eval "${html_escape_command}" >> ./test-data/core1/logs/system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}-- FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi

test_id="./AllTestCases/3n-insertABC-queryABC"
printTestBanner "$test_id"
python ./AllTestCases/test-sharding.py $SRCH2_ENGINE ./AllTestCases/3n-insertABC-queryABC ./AllTestCases/list-of-3-nodes.txt | eval "${html_escape_command}" >> ./test-data/core1/logs/system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}-- FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi

test_id="./AllTestCases/3n-insertABC-queryC"
printTestBanner "$test_id"
python ./AllTestCases/test-sharding.py $SRCH2_ENGINE ./AllTestCases/3n-insertABC-queryC ./AllTestCases/list-of-3-nodes.txt | eval "${html_escape_command}" >> ./test-data/core1/logs/system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}-- FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi

test_id="./AllTestCases/3n-insertA-queryABC"
printTestBanner "$test_id"
python ./AllTestCases/test-sharding.py $SRCH2_ENGINE ./AllTestCases/3n-insertA-queryABC ./AllTestCases/list-of-3-nodes.txt | eval "${html_escape_command}" >> ./test-data/core1/logs/system_test.log 2>&1
if [ ${PIPESTATUS[0]} -gt 0 ]; then
    echo "${html_fail_pre}-- FAILED: $test_id${html_fail_post}" >> ${output}
    if [ $force -eq 0 ]; then
        exit 255
    fi
else
    echo "-- PASSED: $test_id" >> ${output}
fi

