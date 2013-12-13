#include "jni.h"
#include "initJNI.h"

#include<cassert>
#include<cstddef>
#include<cstdio>

int main() {
  jobject array[14];
  jobject map;
  
  /* Creates a map backed by the given array */
  map.array= array;
  
  /* a flat array with depth filled in by self-references; for example,
     the first element of the array contains a pointer to the arrray
     itself */
  array[JavaSDKClasses::JAVA_LANG_CLASS].array = array;
  array[JavaSDKClasses::JAVA_REFLECT_FIELD].array = array;
  array[JavaSDKClasses::COM_SRCH2_REFINING].methodID = 124;
  array[JavaSDKClasses::COM_SRCH2_SEARCHABLE].methodID = 3;
  array[JavaSDKClasses::COM_SRCH2_ATTRIBUTE].methodID = 4;
  array[JavaSDKClasses::COM_SRCH2_PRIMARY_KEY].methodID = 5;
  array[JavaSDKClasses::COM_SRCH2_REFININGFLOAT].methodID = 6;
  array[JavaSDKClasses::COM_SRCH2_REFININGUNSIGNED].methodID = 7;
  array[JavaSDKClasses::COM_SRCH2_REFININGDATE].methodID = 9;
  array[JavaSDKClasses::COM_SRCH2_REFININGINTERVAL].methodID = 10;
  array[JavaSDKClasses::COM_SRCH2_REFININGSTRING].methodID = 11;
  array[JavaSDKClasses::COM_SRCH2_INDEXABLESTRING].methodID = 12;
  array[JavaSDKClasses::COM_SRCH2_ATTRIBUTE_EXECEPTION].methodID = 13;
  array[JavaSDKClasses::COM_SRCH2_PARAMETER_MAPPING].array= array;

  JNIEnv e= JNIEnv();
  
  /* Creates a wrapping this map */
  JavaSDKClasses sdk(e, map ,  0, 0); 
  
  /* Checks that the map looks up directly correctly */
  assert(sdk.refiningClassPtr == 124);
  /* Checks that the map looks up depth, nested maps, correctly */
  assert(sdk.javaClassRef.ptr == map.cls);
  /* Checks second level look up: look ups on nested map */ 
  assert(sdk.javaClassRef.getFields == 124);
  /* Checks last map element edge case */
  assert(sdk.parameterMappingClassRef.ptr == map.cls);

  return 0;
}
