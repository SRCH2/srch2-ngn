/*
 * CustomizableJsonWriter.h
 *
 *  Created on: Nov 18, 2013
 *      Author: Xin Wang
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

	virtual std::string write( const Json::Value &root );

private:
	void writeValue( const Json::Value &value );

	const std::vector<std::pair<std::string, std::string> > *skipTags;
    std::string document_;
    bool yamlCompatiblityEnabled_;
};

#endif /* __WRAPPER_UTIL_CUSTOMIZABLEJSONWRITER_H__ */
