package com.srch2;

import org.junit.*;

public class StringTest {

  @Test
  public void testASCIIString() throws Exception {
    SearchableString testString= new SearchableString("apple");
    StringEngine e= new StringEngine();

    e.setString(testString);
    SearchableString result= e.getString();

    /* Asserts that the the results object is not the same object as the test
       input object; this is java's equivalent of comparing pointers */
    Assert.assertFalse("failure- SearchableString is the same String",
        result == testString);
    Assert.assertEquals("failure- Incorrect value",
        testString.getValue(), result.getValue());
  } 
}
