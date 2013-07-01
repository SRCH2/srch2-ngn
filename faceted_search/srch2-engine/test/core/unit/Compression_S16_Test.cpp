// $Id: Compression_S16_Test.cpp 3456 2013-06-14 02:11:13Z jiaying $
/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
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
