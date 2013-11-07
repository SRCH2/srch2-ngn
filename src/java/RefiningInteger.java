
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

package com.srch2;

public class RefiningInteger implements RefiningIntegerInterface {
  private final int value;
  /** Create a new instance surrounding this String value. The String value
      now can be used to find the instance of a given class containing this
      instance as a field */
  public RefiningInteger(int value) {
    this.value = value;
  }

  /** override */
  public Integer getValue() {
    return this.value;
  }
  public int getInt() {
    return this.value;
  }
}

