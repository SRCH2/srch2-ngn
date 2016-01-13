/*
 */

#include "SchemaInternal.h"
#include "util/Assert.h"
#include <map>
#include <vector>
#include <numeric>

using std::vector;
using std::string;
using std::map;

namespace srch2
{
namespace instantsearch
{

Schema *Schema::create(srch2::instantsearch::IndexType indexType, srch2::instantsearch::PositionIndexType positionIndexType)
{
    return new SchemaInternal(indexType, positionIndexType);
}
Schema *Schema::create()
{
    return new SchemaInternal();
}

}}
