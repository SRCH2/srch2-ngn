package com.srch2;

public class RefiningString implements RefiningStringInterface {
  private final String value;
  /** Create a new instance surrounding this String value. The String value
      now can be used to find the instance of a given class containing this
      instance as a field */
  public RefiningString(String value) {
    this.value = value;
  }

  /** override */
  public String getValue() {
    return this.value;
  }
}
