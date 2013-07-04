
// $Id: IndexUtil.h 3456 2013-06-14 02:11:13Z jiaying $
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
    static const char* const schemaFileName;
    static const char* const analyzerFileName;
    static const char* const indexCountsFileName;

    static const char* const quadTreeFileName;

    static const char* const normalizerFileName;
    static const char* const stemmerFileName;
};
}}

#endif //__INDEX_H__
