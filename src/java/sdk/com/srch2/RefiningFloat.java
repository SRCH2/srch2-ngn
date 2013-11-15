
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

package com.srch2;

public class RefiningFloat implements RefiningFloatInterface {
  private final float value;
  /** Create a new instance surrounding this Float value. The Float value
      now can be used to find the instance of a given class containing this
      instance as a field */
  public RefiningFloat(float value) {
    this.value = value;
  }

  /** override */
  public Float getValue() {
    return this.value;
  }
  /** override */
  public float getFloat() {
    return this.value;
  }
}

