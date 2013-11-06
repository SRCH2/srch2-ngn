jboolean JNIClass::Attribute::isInstance(jobject obj) const {
  return env->IsAssignableFrom(env->GetObjectClass(obj), classPtr);
}
jobject JNIClass::Attribute::createNew(jvalue& content) const {
  return env->NewObject(classPtr, constructor, content);
}


