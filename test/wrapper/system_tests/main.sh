#!/bin/sh
# $1 is <srch2-main-dir>/test/wrapper/system_tests
SYSTEM_TEST_DIR=$1
# $2 is <srch2-main-dir>/build/src/server
SRCH2_ENGINE_DIR=$2
PWD_DIR=$(pwd)
cd $SYSTEM_TEST_DIR
echo '----do exact_A1 test--------------'
python ./exact_a1/exact_A1.py $SRCH2_ENGINE_DIR ./exact_a1/queriesAndResults.txt

if [ $? -gt 0 ]; then
    echo " --- error ---"
    exit -1
fi

echo '----do fuzzy_A1 test--------------'
python ./fuzzy_a1/fuzzy_A1.py $SRCH2_ENGINE_DIR ./fuzzy_a1/queriesAndResults.txt

if [ $? -gt 0 ]; then
    echo " --- error ---"
    exit -1
fi

echo '----do exact_M1 test--------------'
python ./exact_m1/exact_M1.py $SRCH2_ENGINE_DIR ./exact_m1/queriesAndResults.txt

if [ $? -gt 0 ]; then
    echo " --- error ---"
    exit -1
fi

echo '----do fuzzy_M1 test--------------'
python ./fuzzy_m1/fuzzy_M1.py $SRCH2_ENGINE_DIR ./fuzzy_m1/queriesAndResults.txt

if [ $? -gt 0 ]; then
    echo " --- error ---"
    exit -1
fi

echo '----do exact_Attribute_Based_Search test--------------'
python ./exact_attribute_based_search/exact_Attribute_Based_Search.py $SRCH2_ENGINE_DIR ./exact_attribute_based_search/queriesAndResults.txt

if [ $? -gt 0 ]; then
    echo " --- error ---"
    exit -1
fi

echo '----do fuzzy_Attribute_Based_Search test--------------'
python ./fuzzy_attribute_based_search/fuzzy_Attribute_Based_Search.py $SRCH2_ENGINE_DIR ./fuzzy_attribute_based_search/queriesAndResults.txt

if [ $? -gt 0 ]; then
    echo " --- error ---"
    exit -1
fi

echo '----do exact_Attribute_Based_Search_Geo test--------------'
python ./exact_attribute_based_search_geo/exact_Attribute_Based_Search_Geo.py $SRCH2_ENGINE_DIR ./exact_attribute_based_search_geo/queriesAndResults.txt

if [ $? -gt 0 ]; then
    echo " --- error ---"
    exit -1
fi

echo '----do fuzzy_Attribute_Based_Search_Geo test--------------'
python ./fuzzy_attribute_based_search_geo/fuzzy_Attribute_Based_Search_Geo.py $SRCH2_ENGINE_DIR ./fuzzy_attribute_based_search_geo/queriesAndResults.txt

if [ $? -gt 0 ]; then
    echo " --- error ---"
    exit -1
fi

echo '----do geo test--------------'
python ./geo/geo.py $SRCH2_ENGINE_DIR ./geo/queriesAndResults.txt

if [ $? -gt 0 ]; then
    echo " --- error ---"
    exit -1
fi

echo '----do term type test--------------'
python ./term_type/term_type.py $SRCH2_ENGINE_DIR ./term_type/queriesAndResults.txt

if [ $? -gt 0 ]; then
    echo " --- error ---"
    exit -1
fi

echo '----do top_k test--------------'
python ./top_k/test_srch2_top_k.py $SRCH2_ENGINE_DIR food 10 20

if [ $? -gt 0 ]; then
    echo " --- error ---"
    exit -1
fi

echo '----do tests_used_for_statemedia--------------'
#./tests_used_for_statemedia/autotest.sh $SRCH2_ENGINE_DIR

#if [ $? -gt 0 ]; then
#    echo " --- error ---"
#    exit -1
#fi
# clear the output directory. First make sure that we are in correct directory
if [ "$(pwd)" = "$SYSTEM_TEST_DIR" ]; then
    rm -rf data
fi

cd $PWD_DIR
