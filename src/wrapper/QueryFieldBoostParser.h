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
