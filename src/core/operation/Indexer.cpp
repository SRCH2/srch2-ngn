
#include "IndexerInternal.h"
#include "IndexData.h"
namespace srch2
{
namespace instantsearch
{

Indexer *Indexer::create(IndexMetaData* indexMetaData,Analyzer* analyzer, Schema *schema)
{
    return new IndexReaderWriter(indexMetaData, analyzer, schema);
}

Indexer *Indexer::load(IndexMetaData* indexMetaData)
{
    return new IndexReaderWriter(indexMetaData);
}

}}
