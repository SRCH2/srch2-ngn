package com.srch2;

/**
  The fundamental Attribute type of the Srch2Engine. It expresses that the
  underlying String should be indexed to allow subsequence searches over
  the dataspace, namely records, containing certain neighbourhoods of a given
  String as this Attribute's value.
  */
class SearchableString implements SearchableStringI {
  private final String value;
  /** Create a new instance surrounding this String value. The String value
      now can be used to find the instance of a given class containing this
      instance as a Field */
  SearchableString(String value) {
    this.value = value;
  }

  /** override */
  public String getValue() {
    return this.value;
  }
}
