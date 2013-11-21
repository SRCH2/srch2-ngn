/*
 * CustomizableJsonWriter.cpp
 *
 *  Created on: Nov 18, 2013
 *      Author: Xin Wang
 */

#include <util/CustomizableJsonWriter.h>

using namespace Json;

CustomizableJsonWriter::CustomizableJsonWriter(std::vector<std::string> tags)
{
	this->skipTags = tags;
	yamlCompatiblityEnabled_ = false;
}

void CustomizableJsonWriter::enableYAMLCompatibility() {
	yamlCompatiblityEnabled_ = true;
}

std::string
CustomizableJsonWriter::write( const Value &root )
{
   document_ = "";
   writeValue( root );
   document_ += "\n";
   return document_;
}


void CustomizableJsonWriter::writeValue( const Value &value )
{
	   switch ( value.type() )
	   {
	   case Json::nullValue:
	      document_ += "null";
	      break;
	   case Json::intValue:
	      document_ += Json::valueToString( value.asInt() );
	      break;
	   case Json::uintValue:
	      document_ += Json::valueToString( value.asUInt() );
	      break;
	   case Json::realValue:
	      document_ += Json::valueToString( value.asDouble() );
	      break;
	   case Json::stringValue:
	      document_ += Json::valueToQuotedString( value.asCString() );
	      break;
	   case Json::booleanValue:
	      document_ += Json::valueToString( value.asBool() );
	      break;
	   case Json::arrayValue:
	      {
	         document_ += "[";
	         int size = value.size();
	         for ( int index =0; index < size; ++index )
	         {
	            if ( index > 0 )
	               document_ += ",";
	            writeValue( value[index] );
	         }
	         document_ += "]";
	      }
	      break;
	   case Json::objectValue:
	      {
	    	  Json::Value::Members members( value.getMemberNames() );
	         document_ += "{";
	         for ( Json::Value::Members::iterator it = members.begin();
	               it != members.end();
	               ++it )
	         {
	            const std::string &name = *it;
	            if ( it != members.begin() )
	               document_ += ",";

	            // if current field is in skipTags, then regard this field as a string
	            bool skipFlag = false;
	            for(int i = 0; i < this->skipTags.size(); i++) {
	            	if(name == this->skipTags[i]) {
	            		skipFlag = true; break;
	            	}
	            }

	            if(!skipFlag) {
					document_ += Json::valueToQuotedString( name.c_str() );
					document_ += yamlCompatiblityEnabled_ ? ": "
														  : ":";
					writeValue( value[name] );
	            } else {
					document_ += Json::valueToQuotedString( name.c_str() );
					document_ += yamlCompatiblityEnabled_ ? ": "
														  : ":";
					std::string content = value[name].asCString();
					document_ += content ;
	            }
	         }
	         document_ += "}";
	      }
	      break;
	   }
}


