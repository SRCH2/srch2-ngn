
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
class FloatEngine {
  /** Performed each time Java Loads this Class */
  static {
    /** Dynamically links in libfloat_core.so to bind native methods */
    java.lang.System.loadLibrary("float_engine");
  }

  /* A handle to the c++ part of this FloatEngine */
  private long handle;
  /* Java handle to the RefiningFloat constructor, invoking this
        handle with float, namely

        makeRefiningFloat.newInstance(Float);

          is equivalent to

        RefiningFloat(Float);
     */
  Constructor makeRefiningFloat;

  FloatEngine() throws NoSuchMethodException {
     /* Java handle to the RefiningFloat method getValue, invoking this
        handle with a RefiningFloat instance, namely,

           getFloat.invoke(refiningFloatInstance) 

             is equivalent to 
           
           refiningFloatInstance.getValue();
     */
     Method getFloat = RefiningFloatInterface.class.getMethod("getFloat");

     try {
       makeRefiningFloat = RefiningFloat.class.getConstructor(Float.TYPE);
     } catch(NoSuchMethodException ex) {
       /* Special Case logic used to test logic using both an Float.class
          and a java float primitive */
       makeRefiningFloat = MyRefiningFloat.class.getConstructor(Float.class);
     }


     handle =
       createFloatEngine(getFloat, RefiningFloat.class, makeRefiningFloat);
  }

  /** Sets up the c++ part of this FloatEngine, informing it of language 
      used by the Java part, namely this instance, of this FloatEngine. */
  native static long
    createFloatEngine(Method getFloat, 
        Class<RefiningFloat> refiningFloatClass,
        Constructor createRefiningFloat);

  /** Passing an Float Attribute to the FloatEngine. This is used in testing
      the JNI handling of Float Attributes */
  public void setFloat(Attribute<Float> floatValue) {
    setFloat(handle, floatValue);
  }
  /** Passes the Float Attribute to the C++ part of this Engine, reference by
      the passed handle, for storage */
  private static native void setFloat(long handle, Attribute<Float> floatValue);

  /** Returns a RefiningFloat from the FloatEngine. This is used in testing the
      JNI handling of RefiningFloats */
  public RefiningFloat getRefiningFloat() {
    return getRefiningFloat(handle);
  }

  /** Returns the RefiningFloat stored in the C++ part of this Engine,
      reference by the passed handle */
  private static native RefiningFloat getRefiningFloat(long handle);

  /** Free the heap memory storing the c++ side of this FloatEngine */
  private native void deleteFloatEngine(long handle);
}

