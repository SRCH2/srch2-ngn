#include <string>
#include "ParsedParameterContainer.h"

#ifndef __WRAPPER_QUERY_FIELD_BOOST_PARSER_H__
#define __WRAPPER_QUERY_FIELD_BOOST_PARSER_H__

namespace srch2 {
namespace httpwrapper {

struct QueryFieldBoostParser {
  /* Parses the qf parameter of url header, ie. qf=title^10+body^2 and adds
     the parse information into the QueryFieldBoostContainer's qfterms list
     as a new structure 
     Vector boosts :
      [0] struct QueryFieldBoostContainer {
            attribute = "title";
            boostFactor = 10;
          };
      [1] struct QueryFieldBoostContainer {
            attribute = "body";
            boostFactor = 2;
          };
   */
  static bool parseAndAddCriterion(QueryFieldBoostContainer&, std::string&);
};

}}

#endif /*__QUERY_FIELD_BOOST_PARSER_H__*/
