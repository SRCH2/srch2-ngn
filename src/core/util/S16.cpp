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

// $Id: S16.cpp 3410 2013-06-05 12:58:08Z jiaying $
#include "util/cowvector/compression/S16.h"

const unsigned S16::big_directory[][S16::MAX_COL] =
{
	{7 , 7 , 7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{8 , 8 , 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{10, 9 , 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{14, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{28, 0 , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

const unsigned S16::small_directory[][S16::MAX_COL] =
{
	{2 , 2 , 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
	{4 , 3 , 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0},
	{4 , 4 , 4, 4, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0},
	{4 , 4 , 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0},
	{5 , 5 , 5, 5, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0},
	{6 , 5 , 5, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0},
	{6 , 6 , 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0},
	{6 , 6 , 6, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{6 , 6 , 6, 6, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{7 , 7 , 7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{8 , 7 , 7, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{8 , 8 , 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{10, 9 , 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{14, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{15, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{28, 0 , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};
