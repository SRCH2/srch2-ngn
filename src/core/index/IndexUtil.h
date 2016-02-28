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
 */

#pragma once
#ifndef __INDEX_H__
#define __INDEX_H__

namespace srch2
{
namespace instantsearch
{

typedef enum
{
    INDEX_LOAD = 0, // load an existing index
    INDEX_BUILD = 1, // build an index from a file
    INDEX_UPDATE = 2 // update an existing index
} IndexOpenMode;

class IndexConfig
{
public:
    static const char* const trieFileName;
    static const char* const invertedIndexFileName;
    static const char* const forwardIndexFileName;
    static const char* const recordIdConverterFileName;
    static const char* const permissionMapFileName;
    static const char* const schemaFileName;
    static const char* const analyzerFileName;
    static const char* const indexCountsFileName;
    static const char* const AccessControlFile;

    static const char* const quadTreeFileName;

    static const char* const normalizerFileName;
    static const char* const stemmerFileName;

    static const char* const queryTrieFileName;
    static const char* const queryFeedbackFileName;
};
}}

#endif //__INDEX_H__
