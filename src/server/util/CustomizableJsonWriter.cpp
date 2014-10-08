/*
 * CustomizableJsonWriter.cpp
 *
 *  Created on: Nov 18, 2013
 *      Author: Xin Wang
 */

#include <util/CustomizableJsonWriter.h>

using namespace Json;
using namespace std;

CustomizableJsonWriter::CustomizableJsonWriter(const vector<pair<string, string> > *tags)
{
    this->skipTags = tags;
    yamlCompatiblityEnabled_ = false;
}

void CustomizableJsonWriter::enableYAMLCompatibility() {
    yamlCompatiblityEnabled_ = true;
}

std::string
CustomizableJsonWriter::write( const Value &root )const
{
    std::string result = this->writeValue( root ) + "\n";
    return result;
}

std::string 
CustomizableJsonWriter::write( const Json::Value &root )
{
    std::string result = this->writeValue( root ) + "\n";
    return result;
}

// This is a thread-safe write function. 
std::string CustomizableJsonWriter::writeValue( const Value &value )const
{
    string document_ ("");
    switch ( value.type() )
    {
        case nullValue:
            //document_ += "null";
            break;
        case intValue:
            document_ += valueToString( value.asInt() );
            break;
        case uintValue:
            document_ += valueToString( value.asUInt() );
            break;
        case realValue:
            document_ += valueToString( value.asDouble() );
            break;
        case stringValue:
            document_ += valueToQuotedString( value.asCString() );
            break;
        case booleanValue:
            document_ += valueToString( value.asBool() );
            break;
        case arrayValue:
            {
                document_ += "[";
                int size = value.size();
                for ( int index =0; index < size; ++index )
                {
                    if ( index > 0 )
                        document_ += ",";
                    document_ += writeValue( value[index] );
                }
                document_ += "]";
            }
            break;
        case objectValue:
            {
                Value::Members members( value.getMemberNames() );
                document_ += "{";
                for ( Value::Members::iterator it = members.begin();
                        it != members.end();
                        ++it )
                {
                    const string &name = *it;
                    if ( it != members.begin() )
                        document_ += ",";

                    // if current field is in skipTags, then regard this field as a string
                    bool skipFlag = false;
                    unsigned idx = 0;
                    for(idx = 0; idx < this->skipTags->size(); idx++) {
                        // If the name of the value is equal to one of the tags we want
                        // to skip, we set the "skipFlag" to true.
                        if(name == this->skipTags->at(idx).first) {
                            skipFlag = true;
                            break;
                        }
                    }

                    if(!skipFlag) {
                        document_ += Json::valueToQuotedString( name.c_str() );
                        document_ += yamlCompatiblityEnabled_ ? ": "
                            : ":";
                        document_ += writeValue( value[name] );
                    } else {
                        // We use the second string in the pair as the json label
                        document_ += Json::valueToQuotedString( this->skipTags->at(idx).second.c_str() );
                        document_ += yamlCompatiblityEnabled_ ? ": "
                            : ":";
                        document_ += value[name].asCString() ;
                    }
                }
                document_ += "}";
            }
            break;
    }
    return document_;
}

