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
#include "URLParser.h"

namespace srch2
{
namespace httpwrapper
{


//for search
const char URLParser::queryDelimiter = '+';
const char URLParser::filterDelimiter = ':';
const char URLParser::fieldsAndDelimiter = ',';
const char URLParser::fieldsOrDelimiter = '.';

const char* const URLParser::searchTypeParamName = "type";
const char* const URLParser::keywordsParamName = "q";
const char* const URLParser::termTypesParamName = "termtypes";
const char* const URLParser::termBoostsParamName = "termboosts";
const char* const URLParser::fuzzyQueryParamName = "fuzzy";
const char* const URLParser::similarityBoostsParamName = "simboost";
const char* const URLParser::resultsToRetrieveStartParamName = "start";
const char* const URLParser::resultsToRetrieveLimitParamName = "limit";
const char* const URLParser::attributeToSortParamName = "sortby";
const char* const URLParser::orderParamName = "order";
const char* const URLParser::lengthBoostParamName = "lengthboost";
const char* const URLParser::jsonpCallBackName = "callback";

const char* const URLParser::leftBottomLatitudeParamName = "lb_lat";
const char* const URLParser::leftBottomLongitudeParamName = "lb_lng";
const char* const URLParser::rightTopLatitudeParamName = "rt_lat";
const char* const URLParser::rightTopLongitudeParamName = "rt_lng";

const char* const URLParser::centerLatitudeParamName = "ct_lat";
const char* const URLParser::centerLongitudeParamName = "ct_lng";
const char* const URLParser::radiusParamName = "radius";
const char* const URLParser::nameParamName = "exported_data_file";
const char* const URLParser::logNameParamName = "log_data_file";
const char* const URLParser::setParamName = "set";
const char* const URLParser::shutdownForceParamName = "force";
const char* const URLParser::shutdownSaveParamName = "save";

// Schema will be used in Attribute-based search to set attribute bitmap.

}}
