//$Id: IndexUtil.cpp 3456 2013-06-14 02:11:13Z jiaying $

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

#include "IndexUtil.h"

namespace srch2
{
namespace instantsearch
{

const char* const IndexConfig::trieFileName = "CL1.idx";
const char* const IndexConfig::invertedIndexFileName = "CL2.idx";
const char* const IndexConfig::forwardIndexFileName = "CL3.idx";
const char* const IndexConfig::quadTreeFileName = "CL4.idx";
const char* const IndexConfig::recordIdConverterFileName = "CL5.idx";
const char* const IndexConfig::permissionMapFileName = "CL6.idx";
const char* const IndexConfig::indexCountsFileName = "counts.idx";
const char* const IndexConfig::schemaFileName = "Schema.idx";
const char* const IndexConfig::analyzerFileName = "Analyzer.idx";
const char* const IndexConfig::AccessControlFile = "aclAttributes.idx";

const char* const IndexConfig::queryTrieFileName = "Query.idx";
const char* const IndexConfig::queryFeedbackFileName = "Feedback.idx";

}}
