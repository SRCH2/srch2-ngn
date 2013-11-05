
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
class StringEngine {
  /** Performed each time Java Loads this Class */
  static {
    /** Dynamically links in libsrch2_string_core.so to bind native methods */
    java.lang.System.loadLibrary("string_engine");
  }

  /* A handle to the c++ part of this StringEngine */
  private long handle;

  StringEngine() throws NoSuchMethodException {
     /* Java handle to the SearchableString method getValue, invoking this
        handle with a SearchableString instance, namely,

           getString.invoke(searchableStringInstance) 

             is equivalent to 
           
           searchableStringInstance.getValue();
     */
     Method getString = SearchableStringInterface.class.getMethod("getValue");
     /* Java handle to the SearchableString constructor, invoking this
        handle with a UTF16 encoded string, namely

        makeSearchableString.newInstance(String);

          is equivalent to

        SearchableString(String);
     */
     Constructor makeSearchableString=
       SearchableString.class.getConstructor(String.class);
     handle = createStringEngine(SearchableString.class, getString,
         makeSearchableString);
  }

  /** Sets up the c++ part of this StringEngine, informing it of language 
      used by the Java part, namely this instance, of this StringEngine. 
      The Language used by this engine surrounds the SearchableString Class,
      which is instanced as an Object basing all its instances; this is the 
      first parameter passed. This class contains two functional members to 
      manipulate its instances: one, which the second parameter refences a
      handle of, is a method returning a constant reference to its underlying
      UFT16 encoded string value; the second, which bases the handle referenced
      by the third parameter, is a constructor of a SearchableString object
      which takes a single argument, namely its UTF16 String value
  */
  native long
    createStringEngine(Class<SearchableString> searchableStringClass,
        Method getString, Constructor createString);

  /** Passing a SearchableString the StringEngine. This is used in testing the
      JNI handling of SearchableStrings */
  public void setString(SearchableString string) {
    setString(handle, string);
  }
  /** Passes the SearchableString to the C++ part of this Engine, reference by
      the passed handle, for storage */
  private native void setString(long handle, SearchableString string);


  /** Returns a SearchableString from the StringEngine. This is used in
      testing the JNI handling of SearchableStrings */
  public SearchableString getString() {
    return getString(handle);
  }
  /** Returns the SearchableString stored in the C++ part of this Engine,
      reference by the passed handle */
  private native SearchableString getString(long handle);

  /** Free the heap memory storing the c++ side of this StringEngine */
  private native void deleteStringEngine(long handle);
}

