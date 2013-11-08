
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

package com.srch2;

import java.lang.reflect.Method;
import java.lang.reflect.Constructor;

/** Mock up of the Srch2 Engine to test a particular facet of the sdk */
class IntEngine {
  /** Performed each time Java Loads this Class */
  static {
    /** Dynamically links in libint_core.so to bind native methods */
    java.lang.System.loadLibrary("int_engine");
  }

  /* A handle to the c++ part of this IntEngine */
  private long handle;
  /* Java handle to the RefiningInt constructor, invoking this
        handle with integer, namely

        makeRefiningInt.newInstance(Integer);

          is equivalent to

        RefiningIntger(Integer);
     */
  Constructor makeRefiningInt;

  IntEngine() throws NoSuchMethodException {
     /* Java handle to the RefiningInteger method getValue, invoking this
        handle with a RefiningInteger instance, namely,

           getInt.invoke(refiningIntegerInstance) 

             is equivalent to 
           
           refiningIntegerInstance.getValue();
     */
     Method getInt = RefiningIntegerInterface.class.getMethod("getInt");

     try {
       makeRefiningInt = RefiningInteger.class.getConstructor(Integer.TYPE);
     } catch(NoSuchMethodException ex) {
       makeRefiningInt = MyRefiningInteger.class.getConstructor(Integer.class);
     }


     handle = createIntEngine(getInt, RefiningInteger.class, makeRefiningInt);
  }

  /** Sets up the c++ part of this IntEngine, informing it of language 
      used by the Java part, namely this instance, of this IntEngine. */
  native static long
    createIntEngine(Method getInt, 
        Class<RefiningInteger> refiningIntClass,
        Constructor createRefiningInt);

  /** Passing an Int Attribute to the IntEngine. This is used in testing
      the JNI handling of Int Attributes */
  public void setInt(Attribute<Integer> intValue) {
    setInt(handle, intValue);
  }
  /** Passes the Int Attribute to the C++ part of this Engine, reference by
      the passed handle, for storage */
  private static native void setInt(long handle, Attribute<Integer> intValue);

  /** Returns a RefiningInt from the IntEngine. This is used in testing the
      JNI handling of RefiningInts */
  public RefiningInteger getRefiningInt() {
    return getRefiningInt(handle);
  }

  /** Returns the RefiningInt stored in the C++ part of this Engine,
      reference by the passed handle */
  private static native RefiningInteger getRefiningInt(long handle);

  /** Free the heap memory storing the c++ side of this IntEngine */
  private native void deleteIntEngine(long handle);
}

