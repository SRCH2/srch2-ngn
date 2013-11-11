#ifndef __ATTRIBUTE_STRING_H__
#define __ATTRIBUTE_STRING_H__

/*********************6********************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

#include<string>
#include "util/utf8/unchecked.h"
#include "util/utf16/utf16.h"

/* Case encasing logic for converting a UTF16 java string into an equivalent
   UTF8 encoded std::string */
namespace JNIClass {
  struct MakeUTF8StringFromUTF16JString {
    /* creates a new java string with UTF16 encoding passed as an out parameter 
       from the given std::string; a temporary buffer is also given to perform
       the necessary conversion */
    private:
    static void inline creatNew(std::string& content, jchar* buffer,
        JNIEnv &env, jstring& internalString) {
      jchar *end;
  
      /* convert a UTF8 string to a UTF16 encoded array*/
      end= utf8::unchecked::utf8to16(content.begin(), content.end(), buffer);
  
      /* Creates a new String on the Java heap */
      internalString= (jstring) env.NewString(buffer, end - buffer);
    }
    public:
    /* return a std::string encoding in UTF8 with value equivalent to the
       given java UTF16 string */
    static std::string convertJavaToCPP(JNIEnv& env, jobject srch2String) {
      const jchar *utf16CharStringValue=
        env.GetStringChars((jstring)srch2String, JNI_FALSE);
    
      /* converts a UTF16 char array to UTF8 */
      utf8::uint16_t* utf16Start= (utf8::uint16_t*) utf16CharStringValue;
      utf8::uint16_t* utf16CharString= (utf8::uint16_t*) utf16CharStringValue;
      utf8::uint16_t* utf16End= (utf8::uint16_t*) 
      ((char*) utf16Start +
      utf16::byteLength(env.GetStringLength((jstring) srch2String),
        utf16CharString));
      char utf8Start[1024];
      char *utf8End=
        utf8::unchecked::utf16to8(utf16Start, utf16End, utf8Start);
  
        return std::string(utf8Start, utf8End);
      }
    
    /* returns a reference to a newly created UFT16 encoded java string with
       value equivalent to the given UTF8 encoded std::string */
    static jvalue makeJValue(JNIEnv& env, std::string& content) {
    /* Checks the size of input string to see if it can be 
       handled on the stack*/
    jchar *buffer;
    jvalue internalString;
    jobject rtn;
  
    if((content.length() < 512)) {
      jchar uft16start[1024];
      buffer= uft16start;
    } else {
      buffer= new jchar[content.length() * 2];
    }
    /* Creates a new Object of StringAttribute type on the java heap using
       the constructor specified with internalString as an argument.
  
       Runs 
           new StringAttribute(internalString)
  
       in the JVM, and returns a handle to the new object */
    MakeUTF8StringFromUTF16JString::creatNew(content,
        buffer, env, (jstring&) internalString.l);
      
    /* free heap memory if needed */
    if((content.length() >= 512)) 
      delete buffer;
  
    return internalString;
  }
};
}
#endif /* __STRING_ATTRIBUTE_H__ */
