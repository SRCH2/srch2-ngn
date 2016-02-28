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
 * CustomizableJsonWriter.h
 *
 *  Created on: Nov 18, 2013
 */

#ifndef __WRAPPER_UTIL_CUSTOMIZABLEJSONWRITER_H__
#define __WRAPPER_UTIL_CUSTOMIZABLEJSONWRITER_H__

#include "json/json.h"

// As a subclass of Json::Writer, this class modifies the writeValue() function
// so that when it sees a predefined JSON tag, instead of recursively calling
// the "writeValue" of its children, we just attach the unparsed string.
// The goal is to save parsing time.
class CustomizableJsonWriter : public Json::Writer {
public:
    CustomizableJsonWriter(const std::vector<std::pair<std::string, std::string> > *tags);
    virtual ~CustomizableJsonWriter() { };
    void enableYAMLCompatibility();

    virtual std::string write( const Json::Value &root ) ;
    std::string write( const Json::Value &root ) const ;

private:
    std::string writeValue( const Json::Value &value ) const ;

    const std::vector<std::pair<std::string, std::string> > *skipTags;
    bool yamlCompatiblityEnabled_;
};

#endif /* __WRAPPER_UTIL_CUSTOMIZABLEJSONWRITER_H__ */
