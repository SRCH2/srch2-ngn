/*
 * =====================================================================================
 *
 *       Filename:  DictionaryBuilder.cpp
 *
 *    Description:  This exe is the binary dictionary builder.
 *
 *        Version:  1.0
 *        Created:  08/09/2013 05:14:19 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Jianfeng (), jianfengjia@srch2.com
 *   Organization:  SRCH2.com
 *
 * =====================================================================================
 */
#include "analyzer/Dictionary.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "util/Logger.h"

using namespace std;
using namespace srch2::instantsearch;
using namespace srch2::util;

bool buildDict(Dictionary &dict, const char* input, const char* output){
    ifstream fs(input, ios::in);
    if(!fs.is_open()){
        Logger::error("%s open failed", input);
        return false;
    }
    string line;
    int count = 0;
    while(std::getline(fs,line)){

        if( ++ count % 10000 == 0){
            Logger::info("line:%d", count);
        }
        string word;
        float freq = -1;
        stringstream ss (line);
        ss >> word >> freq;
        if (freq < 0){
            Logger::warn("line:%d read error:%s\t%s:%.1f", count, line.c_str(), word.c_str(), freq);
        }
        dict.insert(word, (short)freq);
    }
    Logger::info("line:%d read over", count);
    return dict.saveDict(string (output));
}

int main(int argc, const char* argv[]){
    if(argc < 2){
        Logger::error("usage:buildDict <inputTextFile> <outputBinaryFile>");
        return EXIT_FAILURE;
    }
    Dictionary dict;
    if(buildDict(dict, argv[1], argv[2])){
        return EXIT_SUCCESS;
    }else{
        Logger::error("EXIT_FAILURE");
        return EXIT_FAILURE;
    }
}
