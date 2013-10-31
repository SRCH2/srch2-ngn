package com.srch2;

import org.junit.Assert;

class StringTest {

  @Test
  public void testASCIIString() {
    SearchableString testString= new SearchableString();
    StringEngine e= new StringEngine();

    e.setString(testString);
    SearchableString result= e.getString();

    Assert.assertFalse("failure- SearchableString is the same String",
        result == testString);
    Assert.assertEquals("failure- Incorrect value",
        result.getValue(), testString.getValue());
  } 
}
