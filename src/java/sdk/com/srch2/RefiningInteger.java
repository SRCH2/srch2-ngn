
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

package com.srch2;

public class RefiningInteger implements RefiningIntegerInterface {
  private final Integer value;
  /** Create a new instance surrounding this Integer value. The Integer value
      now can be used to find the instance of a given class containing this
      instance as a field */
  public RefiningInteger(int value) {
    this.value = value;
  }

  /** override */
  public Integer getValue() {
    return this.value;
  }
  /** override */
  public int getInt() {
    return this.value;
  }
}

