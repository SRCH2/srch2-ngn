package com.srch2;

/* User defined RefiningInteger that has a backing of Integer class, rather
   than the java integer primative. */
public class MyRefiningInteger implements RefiningIntegerInterface {
  Integer value;
  MyRefiningInteger(Integer value) { this.value = value; }

  public int getInt() { return this.value; }
  public Integer getValue() { return this.value; }
}
