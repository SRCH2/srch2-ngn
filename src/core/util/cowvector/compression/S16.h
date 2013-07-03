
// $Id: S16.h 3410 2013-06-05 12:58:08Z jiaying $
#ifndef __S_16_H__
#define __S_16_H__

#include <cmath>
#include <cassert>
#include <iostream>
#include <string>
#include <cstring>

class S16
{
private:
	static const unsigned COMMAND_LEN = 4;
	static const unsigned COMMAND_MASK = (1<< COMMAND_LEN) - 1;
	static const unsigned MAX_COL = 14;
	static const unsigned NULL_NUMBER = 0;
public:
	static const unsigned big_directory[][MAX_COL];
	static const unsigned small_directory[][MAX_COL];

	static int byte(const int x)
	{
		assert(x >= 0);
		if(!x) return 1;
		else return (int)log2(x)+1;
	}

	// encodeUnsigned compress array[from, from+num) in one unsigned using S16 directory
	// input:
	//			array, from, num: the array[from, from+num)
	//			command: the compressed method in directory
	//			directory: the compress directory (optional)
	// output:
	//			out: a unsigned compressed data
	// return:	null
	static void encodeUnsigned(const unsigned *array, const unsigned from, const unsigned num, unsigned command, unsigned &out, const unsigned directory[][MAX_COL] = small_directory)
	{
		assert(command < 16);
		out = command;
		int idx = 4;
		const unsigned *p = array+from;
		for(int i = 0; i< num; i++, p++)
		{
			out |= (*p << idx);
			idx += directory[command][i];
		}
	}

	// encode compress array[0, size) to a array encoded_array using S16 directory
	// input:
	//			array, size: denote the array array[0, size)
	//			directory: the compress directory (optional)
	// output:
	//			encoded_array: a new array contains compressed data
	// return:	null
	static void encode(const unsigned *array, const unsigned size, unsigned *encoded_array, const unsigned directory[][MAX_COL] = small_directory)
	{
		unsigned idx = 0, eidx = 0;
		unsigned temp[MAX_COL];
		for(idx = 0, eidx = 0; idx < size; eidx++)
		{
			memset(temp, 0, MAX_COL * sizeof(unsigned));
			for(int row = 0; ; row++)
			{
				bool finished = false;
				int col;
				for(col =0; ; col++)
				{
					if(col < MAX_COL && directory[row][col] != NULL_NUMBER)
					{
						if(temp[col] ==  NULL_NUMBER)
						{
							if(idx + col < size)
								temp[col] = byte(array[idx+col]);
							else
								break;
						}
						if(temp[col] > directory[row][col])
							break;
					}
					else
					{
						finished = true;
						break;
					}
				}
				if(finished)
				{
					encodeUnsigned(array, idx, col, row, encoded_array[eidx], directory);
					idx += col;
					break;
				}
			}
		}
	}

	// getEncodeSize compute the compressed size of array[0, size) using S16 directory
	// input:
	//			array, size: denote the array array[0, size)
	//			directory: the compress directory (optional)
	// output:	null
	// return:	the compressed size
	static unsigned getEncodeSize(const unsigned *array, const unsigned size, const unsigned directory[][MAX_COL] = small_directory)
	{
		unsigned idx = 0, eidx = 0;
		unsigned temp[MAX_COL];
		for(idx = 0, eidx = 0; idx < size; eidx++)
		{
			memset(temp, 0, MAX_COL * sizeof(unsigned));
			for(int row = 0; ; row++)
			{
				bool finished = false;
				int col;
				for(col =0; ; col++)
				{
					if(col < MAX_COL && directory[row][col] != NULL_NUMBER)
					{
						if(temp[col] ==  NULL_NUMBER)
						{
							if(idx + col < size)
								temp[col] = byte(array[idx+col]);
							else
								break;
						}
						if(temp[col] > directory[row][col])
							break;
					}
					else
					{
						finished = true;
						break;
					}
				}
				if(finished)
				{
					idx += col;
					break;
				}
			}
		}
		return eidx;
	}

	// decodeUnsigned decompress a unsigned data to out[offset,...) using S16 directory
	// input:
	//			encoded: the unsigned data
	//			directory: the compress directory (optional)
	// output:
	//			out, offset: the array start at offset
	// return:	null
	static void decodeUnsigned(unsigned encoded, unsigned *out, unsigned &offset, const unsigned directory[][MAX_COL] = small_directory)
	{
		unsigned command = encoded & COMMAND_MASK;
		encoded >>= COMMAND_LEN;
		for(int i = 0; i < MAX_COL && directory[command][i] > 0; i++)
		{
			 out[offset++] = ((1 << directory[command][i]) - 1) & encoded;
			 encoded >>= directory[command][i];
		}
	}

	// decode decompress encodedArray[0, size) to a array out using S16 directory
	// input:
	//			encodedArray, size: denote the encodedArray[0, size)
	//			directory: the compress directory (optional)
	// output:
	//			out: a new array contains the decompressed data
	// return:	null
	static void decode(const unsigned *encodedArray, const unsigned size, unsigned *out, const unsigned directory[][MAX_COL] = small_directory)
	{
		unsigned offset = 0;
		for(int i = 0; i< size; i++)
		{
			decodeUnsigned(encodedArray[i], out, offset, directory);
		}
	}
};

#endif
