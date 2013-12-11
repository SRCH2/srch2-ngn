#ifndef __JNI_H__
#define __JNI_H__

/* Mock up of JNI functions used in map_constructor */

typedef int jclass;
typedef int jmethodID;
typedef int jfieldID;
typedef int jint;

union jobject {
  jobject* array;
  jmethodID methodID;
  jfieldID fieldID;
  jclass cls;
};

struct JNIEnv {
  inline jobject CallObjectMethod(jobject& o,jmethodID,jint key) {
    return o.array[key];
  }
  inline jmethodID FromReflectedMethod(jobject& o) {return o.methodID;}
  inline jfieldID FromReflectedField(jobject& o) {return o.fieldID;}
  inline jclass NewGlobalRef(jclass& cls) {return cls;}
};

#endif /* __JNI_H__ */
