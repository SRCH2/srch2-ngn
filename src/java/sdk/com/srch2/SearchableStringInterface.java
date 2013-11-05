/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

package com.srch2;

/**
  Implementing Classes are indexed by Srch2 to allow them to be searched. 
  Implementing this expresses that the class can be converted to a String,
  returned by this getValue method, which should be indexed to allow
  subsequence searches over the dataspace, namely instances of classes with
  a field of this type, for certain neighbourhoods of a this String, as that
  field's value. 
*/
public interface SearchableStringInterface
extends Attribute<String>, Searchable {}
