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
#include "analyzer/Normalizer.h"
#include <string.h>
#include <stdio.h>
#include <iostream>
#include "util/Assert.h"

using namespace std;
using namespace srch2::instantsearch;

void testNormalizer()

{
	string FILE_DIR = getenv("file_dir");
	Normalizer *normalizer_handler = new Normalizer(srch2::instantsearch::ONLY_NORMALIZER, FILE_DIR);
	vector<string> tokens;
	string token = "wal";
	tokens.push_back(token);
	token = "mart";
	tokens.push_back(token);
	token = "wal mart";
	tokens.push_back(token);
	token = "cheese";
	tokens.push_back(token);
	token = "factory";
	tokens.push_back(token);
	token = "star";
	tokens.push_back(token);
	token = "bucks";
	tokens.push_back(token);

    normalizer_handler->normalize(tokens);

    assert(tokens.size()==11);

}


int main(int argc, char *argv[])
{


	testNormalizer();
	cout<<"\n\nNormalizer unit tests passed!!";

	return 0;

}
