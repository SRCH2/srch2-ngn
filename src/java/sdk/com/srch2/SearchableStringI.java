package com.srch2;

/**
  Implementing Class are indexed by Srch2 to allow them to be searched. 
  Implementing this expresses that the class can be converted to a String,
  returned by this getValue method, which should be indexed to allow
  subsequence searches over the dataspace, namely instance of Class with
  field of this type, for certain neighbourhoods of a this String, as that
  field's value. 
*/
interface SearchableStringI {
  /** Lookup value of this instance 
   
      @return converts this instance into a searchable String to be index by
              Srch2 
  */
  String getValue();
}
