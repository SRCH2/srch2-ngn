/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
 *   Organization:  SRCH2.com
 *
 * =====================================================================================
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include "analyzer/Dictionary.h"
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
