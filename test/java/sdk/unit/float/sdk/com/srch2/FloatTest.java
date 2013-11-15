/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

package com.srch2;

import org.junit.*;

public class FloatTest {

  private void testFloat(final float testValue) throws NoSuchMethodException {
    FloatEngine e = new FloatEngine();
    RefiningFloat testFloat = new RefiningFloat(testValue);

    e.setFloat(testFloat);
    Attribute<Float> result = e.getRefiningFloat();

    /* Asserts that the the results object is not the same object as the test
       input object; this is java's equivalent of comparing pointers */
    Assert.assertNotSame("failure- returned RefiningFloat is the same",
         result, testFloat);
    Assert.assertEquals("failure- RefiningFloat has incorrect value",
        testValue, (float) result.getValue(), Float.MIN_VALUE);
  }  

  @Test
  public void testBasic() throws NoSuchMethodException {
    testFloat(123);
  }

  @Test
  public void testZero() throws NoSuchMethodException {
    testFloat(0);
  }
  @Test
  public void testNegative() throws NoSuchMethodException {
    testFloat(-1);
  }
  @Test
  public void testMaxFloat() throws NoSuchMethodException {
    testFloat(Float.MAX_VALUE);
  }
  public void testMinFloat() throws NoSuchMethodException {
    testFloat(Float.MIN_VALUE);
  }

  /* Test RefiningFloat logic with an Float class Constructor rather than
     the float primitive. */
  @Test
  public void testFloatClassConstructor() throws NoSuchMethodException {
    float testValue = 5;
    FloatEngine e = new FloatEngine();
    MyRefiningFloat testFloat = new MyRefiningFloat(testValue);

    e.setFloat(testFloat);
    Attribute<Float> result = e.getRefiningFloat();

    /* Asserts that the the results object is not the same object as the test
       input object; this is java's equivalent of comparing pointers */
    Assert.assertNotSame("failure- returned RefiningFloat is the same",
         result, testFloat);
    Assert.assertEquals("failure- RefiningFloat has incorrect value",
        testValue, (float) result.getValue(), Float.MIN_VALUE);
  }  
}
