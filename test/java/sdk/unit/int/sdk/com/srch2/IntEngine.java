
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
    /** Dynamically links in libsrch2_string_core.so to bind native methods */
    java.lang.System.loadLibrary("int_engine");
  }

  /* A handle to the c++ part of this IntEngine */
  private long handle;

  IntEngine() throws NoSuchMethodException {
     /* Java handle to the RefiningInteger method getValue, invoking this
        handle with a RefiningInteger instance, namely,

           getInt.invoke(refiningIntegerInstance) 

             is equivalent to 
           
           refiningIntegerInstance.getValue();
     */
     Method getInt = RefiningIntegerInterface.class.getMethod("getInt");

     /* Java handle to the RefiningInt constructor, invoking this
        handle with integer, namely

        makeRefiningInt.newInstance(Integer);

          is equivalent to

        RefiningIntger(Integer);
     */
     Constructor makeRefiningInt=
       RefiningInteger.class.getConstructor(Integer.TYPE);


     handle = createIntEngine(getInt, RefiningInteger.class, makeRefiningInt);
  }

  /** Sets up the c++ part of this IntEngine, informing it of language 
      used by the Java part, namely this instance, of this IntEngine. */
  native static long
    createIntEngine(Method getInt, 
        Class<RefiningInteger> refiningIntClass,
        Constructor createRefiningInt);

  /** Passing a Int Attribute to the StringEngine. This is used in testing
      the JNI handling of Int Attributes */
  public void setInt(Attribute<Integer> i) {
    setInt(handle, i);
  }
  /** Passes the Int Attribute to the C++ part of this Engine, reference by
      the passed handle, for storage */
  private static native void setInt(long handle, Attribute<Integer> i);

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

