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

#include "util/cowvector/compression/cowvector_S16.h"
using namespace srch2::instantsearch;

/*
 * In this test, we verify the correctness and compression ratio of S16 compression methods.
 */

//test the compress method S16
void test1()
{
	unsigned size = 100;
	cout <<" Original size: " << size << endl;
	unsigned *data = new unsigned[size];
	for(unsigned i =0; i< size; i++)
	{
		data[i] = i;
	}
	unsigned sizeAfterCompression = S16::getEncodeSize(data, size);
	unsigned *compressedData = new unsigned[sizeAfterCompression];
	S16::encode(data, size, compressedData);
	cout <<"Compressed size: "<<sizeAfterCompression<<endl;
	unsigned *decompressedData = new unsigned[size];
	S16::decode(compressedData, sizeAfterCompression, decompressedData);
	for(unsigned i =0; i< size; i++)
	{
		assert(data[i] == decompressedData[i]);
	}
	delete[] data;
	delete[] compressedData;
	delete[] decompressedData;
}

//test the vectorview_S16
void test2()
{
	VectorViewS16Big vectorViewS16Big;
	for(unsigned i = 0; i< 100; i++)
		vectorViewS16Big.push_back(i);
	assert(vectorViewS16Big.size() == 100);
	for(unsigned i = 0; i< 100; i++)
		assert(vectorViewS16Big[i] == i);

	VectorViewS16Small vectorViewS16Small;
	for(unsigned i = 0; i< 100; i++)
		vectorViewS16Small.push_back(i);
	for(unsigned i = 0; i< 100; i++)
		assert(vectorViewS16Small[i] == i);
}

int main()
{
	test1();
	test2();
}
