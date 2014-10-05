
// $Id: IndexerInternal.h 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $

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

 * Copyright 2010 SRCH2 Inc. All rights reserved
 */

#ifndef __INDEX_HEALTH_INFO_H__
#define __INDEX_HEALTH_INFO_H__

#include <string>
#include <vector>
#include <ctime>

using std::vector;
using std::string;



namespace srch2
{
namespace instantsearch
{

class IndexData;

class IndexHealthInfo
{

public:
    unsigned readCount;
    unsigned writeCount;
	unsigned docCount;
	std::string lastMergeTimeString;
	bool isMergeRequired;
	bool isBulkLoadDone;


	static void populateReport(IndexHealthInfo & report, IndexData *index);


    unsigned getNumberOfBytes() const;
    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serialize(void * buffer);

    IndexHealthInfo(const IndexHealthInfo & info);
    IndexHealthInfo();

    //given a byte stream recreate the original object
    void * deserialize(void* buffer);

    IndexHealthInfo & operator=(const IndexHealthInfo & info);

    bool operator==(const IndexHealthInfo & rhs);



    void getString(struct std::tm *timenow, string &in);
    void getLatestHealthInfo(unsigned doc_count);
    const std::string getIndexHealthString() const;
    const void getIndexHealthStringComponents(std::string & lastMergeTimeString, unsigned & docCount) const;
};

}
}

#endif // __INDEX_HEALTH_INFO_H__
