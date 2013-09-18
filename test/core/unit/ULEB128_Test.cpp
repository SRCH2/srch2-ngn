/*
 * ULEB128_Test.cpp
 *
 *  Created on: Sep 13, 2013
 *      Author: sbisht
 */
#include "util/ULEB128.h"
#include <cassert>
#include "util/Logger.h"
#include "util/Assert.h"
#include <iostream>
using namespace std;
using srch2::util::ULEB128;
using srch2::util::Logger;
using namespace srch2::instantsearch;

unsigned testInt32EncodeDecode(unsigned input);
void testInt32ArrayEncodeDecode(const vector<unsigned>& input, vector<unsigned>& output);

unsigned testInt32EncodeDecode(unsigned input) {
    uint8_t buf[5] = {0};
    short size = 0;
    Logger::info("input = %u", input);
    ULEB128::uInt32ToVarLengthBytes(input, buf, &size);
    Logger::info("converted to VLB of length =  %u", size);
    unsigned result = 0;
    int status = ULEB128::varLengthBytesToUInt32(buf, &result, &size);
    Logger::info("read back VLB length = %u", size);
    Logger::info("converted back number = %u", result);
    if (status != 0)
        	Logger::info("error ocurred");
    return result;
}
void testInt32ArrayEncodeDecode(const vector<unsigned>& input, vector<unsigned>& output) {


    for (unsigned i =0; i < input.size(); ++i)
    	Logger::info("%u, ", input[i]);
    cout << endl;

    uint8_t* buffer;
    Logger::info("converting array to VLB");
    int size = ULEB128::uInt32VectorToVarLenArray(input, &buffer);
    Logger::info("done ...");
    Logger::info("now converting VLB to array ");
    ULEB128::varLenByteArrayToInt32Vector(buffer, size, output);
    Logger::info("done ..");

    for (unsigned i =0; i < output.size(); ++i)
    	Logger::info("%u, ", output[i]);
    cout << endl;
}

int main()
{
    cout << "-----------testInt32EncodeDecode -----------" << endl;
    unsigned result = 0;
    result = testInt32EncodeDecode(1);
    ASSERT(result == 1);
    result = testInt32EncodeDecode(10);
    ASSERT(result == 10);
    result = testInt32EncodeDecode(128);
    ASSERT(result == 128);
    result = testInt32EncodeDecode(129);
    ASSERT(result == 129);
    result = testInt32EncodeDecode(255);
    ASSERT(result == 255);
    result = testInt32EncodeDecode(256);
    ASSERT(result == 256);
    cout << "-----------testInt32ArrayEncodeDecode -----------" << endl;
    vector<unsigned> input;
    vector<unsigned> output;
    //input.push_back(12); input.push_back(32); input.push_back(42); input.push_back(72); input.push_back(93); input.push_back(234);
    unsigned testData1[] = {12, 32, 42, 72, 93, 234};
    input.assign(testData1, testData1 + (sizeof(testData1) / sizeof(unsigned)));
    testInt32ArrayEncodeDecode(input, output);
    ASSERT(input.size() == output.size());

    output.clear();
    unsigned testData2[] = { 1, 10, 128, 129, 255, 256};
    input.assign(testData2, testData2 + (sizeof(testData2) / sizeof(unsigned)));
    testInt32ArrayEncodeDecode(input, output);
    ASSERT(input.size() == output.size());

    output.clear();
    unsigned testData3[] = { 23892, 29180, 91290, 1829102, 1290192009};
    input.assign(testData3, testData3 + (sizeof(testData3) / sizeof(unsigned)));
    testInt32ArrayEncodeDecode(input, output);
    ASSERT(input.size() == output.size());

}



