package com.srch2;

import java.lang.reflect.Method;

/** Mock up of the Srch2 Engine to test a particular facet of the sdk */
class Srch2EngineTest {
  long handle;

  Srch2EngineTest() throws NoSuchMethodException {
     Method getString = SearchableStringI.class.getMethod("getValue");
     handle = createSearchableEngine(SearchableString.class, getString);
  }

  /** Sets up the JNI Env and gives it the Searchable */
  native long
    createSearchableEngine(Class<SearchableString> string, Method getString);

  /** Test passing a SearchableString */
  void setString(SearchableString string) {
    setString(handle, string);
  }
  SearchableString getString(SearchableString string) {
    return getString(handle);
  }
  native void setString(long handle, SearchableString string);
  native SearchableString getString(long handle);
}
