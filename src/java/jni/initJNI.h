#ifndef SRCH2_JNI_INIT_H
#define SRCH2_JNI_INIT_H

#include<jni.h>

/* Keep globalReference to make sure java's ClassLoader keeps the class in
   memory. Once it gets garbaged collected, then jclass will become a null
   pointer. */

/* Store a logic mapping of Fields to Class */
struct FieldMapping {
  jfieldID type;
  jfieldID value;
};

struct ClassMethodIDs {
  jclass classPtr;
  jclass fieldClassPtr;
  jmethodID getFields;
  jmethodID getClassName;
  jmethodID getFieldClass;

};

struct JavaSDKClasses {
  struct {
    jclass ptr;
    jmethodID getFields;
    jmethodID getName;
  } javaClassRef;
  struct {
    jclass ptr;
    jmethodID getType;
  } javaFieldClassRef;
  jclass refiningClassPtr;
  jclass searchableClassPtr;
  jclass attributeClassPtr;
  jclass primaryKeyClassPtr;
  jclass refiningFloatClassPtr;
  jclass refiningUnsignedClassPtr;
  jclass refiningDateClassPtr;
  jclass refiningIntervalClassPtr;
  jclass refiningStringClassPtr;
  jclass indexableStringClassPtr;
  jclass attributeExClassPtr;
  struct FieldMapping {
    jclass ptr;
    jfieldID type;
   jfieldID value;
  } parameterMappingClassRef;

  enum {
    JAVA_LANG_CLASS = 0,
    JAVA_REFLECT_FIELD = 1,
    COM_SRCH2_REFINING = 2,
    COM_SRCH2_SEARCHABLE = 3,
    COM_SRCH2_ATTRIBUTE = 4,
    COM_SRCH2_PRIMARY_KEY = 5,
    COM_SRCH2_REFININGFLOAT = 6,
    COM_SRCH2_REFININGUNSIGNED = 7,
    COM_SRCH2_REFININGDATE = 9,
    COM_SRCH2_REFININGINTERVAL = 10,
    COM_SRCH2_REFININGSTRING = 11,
    COM_SRCH2_INDEXABLESTRING = 12,
    COM_SRCH2_ATTRIBUTE_EXECEPTION = 13,
    COM_SRCH2_PARAMETER_MAPPING = 14
  };

  /* Nested mappings, member of associated class, have a their classPtr mapped
     as 0, so their enum must start at 1 */
  static const int CLASS_PTR=0;
  enum {JAVA_LANG_CLASS_GETNAME=1, JAVA_LANG_CLASS_GETFIELDS};
  enum {JAVA_REFLECT_FIELD_GET_TYPE=1};
  enum {PARAMETER_MAPPING_FIELD=1, PARAMETER_MAPPING_VALUE};

  JavaSDKClasses(JNIEnv &env, jobject javaClassMap, jclass mapClass,
      jmethodID lookupMethod) {

    /* Gets class java.lang.map, as a work around to the direct map 
       call undefined behaviour */
   // refiningClassPtr=(jclass) env.
 //   CallObjectMethod(javaClassMap, lookupMethod,
   //       COM_SRCH2_REFINING);
 //   lookupMethod= (jmethodID) env.NewGlobalRef((jobject)lookupMethod);

    jint i=0;
    env.CallObjectMethod(javaClassMap, lookupMethod, i);
    env.CallObjectMethod(javaClassMap, lookupMethod, (i=6));

    /*Closure around the init Map objects, and their possible return values*/
   union Closure {
     struct { jobject javaClassMap; jmethodID lookupMethod; JNIEnv&  env; };
     jobject method;
     jclass ptr;
     jobject field;

     Closure(jmethodID lookupMethod, JNIEnv &env, jobject javaClassMap) :
       javaClassMap(javaClassMap), lookupMethod(lookupMethod), env(env) {}
     Closure() {ptr=0;}

     /* Looks up item in map */
     inline Closure lookup(jint key) {
       jobject value= env.
         CallObjectMethod(javaClassMap, lookupMethod, key);
       return Closure(lookupMethod, env, value); 
     }
     inline jmethodID getMethod() { return env.FromReflectedMethod(field); }
     inline jfieldID getField() { return env.FromReflectedField(method); }
     inline jclass getPtr() { return (jclass) env.NewGlobalRef(ptr); }
    } closure(lookupMethod, env, javaClassMap), cc;
 
    refiningClassPtr= closure.lookup(COM_SRCH2_REFINING).ptr;
    searchableClassPtr= closure.lookup(COM_SRCH2_SEARCHABLE).ptr;
    attributeClassPtr= closure.lookup(COM_SRCH2_ATTRIBUTE).ptr;
    primaryKeyClassPtr= closure.lookup(COM_SRCH2_PRIMARY_KEY).ptr;
    refiningFloatClassPtr= closure.lookup(COM_SRCH2_REFININGFLOAT).ptr;
    refiningUnsignedClassPtr= closure.lookup(COM_SRCH2_REFININGUNSIGNED).ptr;
    refiningDateClassPtr= closure.lookup(COM_SRCH2_REFININGDATE).ptr;
    refiningIntervalClassPtr= closure.lookup(COM_SRCH2_REFININGINTERVAL).ptr;
    refiningStringClassPtr= closure.lookup(COM_SRCH2_REFININGSTRING).ptr;
    indexableStringClassPtr= closure.lookup(COM_SRCH2_INDEXABLESTRING).ptr;
    attributeExClassPtr= closure.lookup(COM_SRCH2_ATTRIBUTE_EXECEPTION).ptr;

    cc= closure.lookup(JAVA_REFLECT_FIELD);
    javaFieldClassRef.ptr= cc.lookup(CLASS_PTR).ptr;
    javaFieldClassRef.getType=
      cc.lookup(JAVA_REFLECT_FIELD_GET_TYPE).getMethod();

    cc= closure.lookup(JAVA_LANG_CLASS);
    javaClassRef.ptr= cc.lookup(CLASS_PTR).ptr;
    javaClassRef.getName= cc.lookup(JAVA_LANG_CLASS_GETNAME).getMethod();
    javaClassRef.getFields= cc.lookup(JAVA_LANG_CLASS_GETFIELDS).getMethod();

    cc= closure.lookup(COM_SRCH2_PARAMETER_MAPPING);
    parameterMappingClassRef.ptr= cc.lookup(CLASS_PTR).ptr;
    parameterMappingClassRef.value=
      cc.lookup(PARAMETER_MAPPING_VALUE).getField();
    parameterMappingClassRef.type=
      cc.lookup(PARAMETER_MAPPING_FIELD).getField();
  }
};

/* Gets a reference pointer and active Global reference to a java Class
   object loaded by the active ClassLoader */
#endif /* SRCH2_JNI_INIT_H */
