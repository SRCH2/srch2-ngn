
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
