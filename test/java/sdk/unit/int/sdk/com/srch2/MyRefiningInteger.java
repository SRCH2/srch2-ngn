package com.srch2;

public class MyRefiningInteger implements RefiningIntegerInterface {
  Integer value;
  MyRefiningInteger(Integer value) { this.value = value; }

  public int getInt() { return this.value; }
  public Integer getValue() { return this.value; }
}
