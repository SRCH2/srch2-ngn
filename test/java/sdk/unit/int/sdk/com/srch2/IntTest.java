/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

package com.srch2;

import org.junit.*;

public class IntTest {

  private void testInt(final int testValue) throws NoSuchMethodException {
    IntEngine e= new IntEngine();
    RefiningInteger testInteger= new RefiningInteger(testValue);

    e.setInt(testInteger);
    Attribute<Integer> result= e.getRefiningInt();

    /* Asserts that the the results object is not the same object as the test
       input object; this is java's equivalent of comparing pointers */
    Assert.assertNotSame("failure- returned RefiningInteger is the same",
         result, testInteger);
    Assert.assertEquals("failure- RefiningInteger has incorrect value",
        testValue, (int) result.getValue());
  }  

  @Test
  public void testBasic() throws NoSuchMethodException {
    testInt(123);
  }
  @Test
  public void testZero() throws NoSuchMethodException {
    testInt(0);
  }
  @Test
  public void testNegative() throws NoSuchMethodException {
    testInt(-1);
  }
  @Test
  public void testMaxInt() throws NoSuchMethodException {
    testInt(Integer.MAX_VALUE);
  }
  public void testMinInt() throws NoSuchMethodException {
    testInt(Integer.MIN_VALUE);
  }
}
