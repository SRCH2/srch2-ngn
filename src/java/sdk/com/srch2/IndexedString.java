
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

package com.srch2;

public class IndexedString implements IndexedStringInterface {
  private final String value;
  /** Create a new instance surrounding this String value. The String value
      now can be used to find the instance of a given class containing this
      instance as a field */
  public IndexedString(String value) {
    this.value = value;
  }

  /** override */
  public String getValue() {
    return this.value;
  }
}

