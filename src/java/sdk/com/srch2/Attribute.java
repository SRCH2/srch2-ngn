
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

package com.srch2;

interface Attribute<T> {
  /** Lookup value of this instance 
   
      @return converts this instance into a searchable String to be indexed by
              Srch2 
  */
  T getValue();
}

