package com.srch2;

/* User defined RefiningFloat that has a backing of Float class, rather
   than the java float primitive. */
public class MyRefiningFloat implements RefiningFloatInterface {
  Float value;
  MyRefiningFloat(Float value) { this.value = value; }

  public float getFloat() { return this.value; }
  public Float getValue() { return this.value; }
}
