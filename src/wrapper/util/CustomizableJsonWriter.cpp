/*
 * CustomizableJsonWriter.cpp
 *
 *  Created on: Nov 18, 2013
 *      Author: Xin Wang
 */

#include <util/CustomizableJsonWriter.h>

using namespace Json;

CustomizableJsonWriter::CustomizableJsonWriter()
   : yamlCompatiblityEnabled_( false )
{
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
   case nullValue:
	  document_ += "null";
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
			writeValue( value[index] );
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
			const std::string &name = *it;
			if ( it != members.begin() )
			   document_ += ",";
			document_ += valueToQuotedString( name.c_str() );
			document_ += yamlCompatiblityEnabled_ ? ": "
												  : ":";
			writeValue( value[name] );
		 }
		 document_ += "}";
	  }
	  break;
   }
}


