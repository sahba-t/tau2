/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class edu_uoregon_TAU_Profile */

#ifndef _Included_edu_uoregon_TAU_Profile
#define _Included_edu_uoregon_TAU_Profile

jlong &TheLastJDWPEventThreadID(void);

#ifdef __cplusplus
extern "C" {
#endif
#undef edu_uoregon_TAU_Profile_TAU_DEFAULT
#define edu_uoregon_TAU_Profile_TAU_DEFAULT 4294967295LL

jlong get_java_thread_id(void);
char *get_java_thread_name(void);

/*
 * Class:     edu_uoregon_TAU_Profile
 * Method:    NativeProfile
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;J)V
 */
JNIEXPORT void JNICALL Java_edu_uoregon_TAU_Profile_NativeProfile
  (JNIEnv *, jobject, jstring, jstring, jstring, jlong);

/*
 * Class:     edu_uoregon_TAU_Profile
 * Method:    NativeStart
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_edu_uoregon_TAU_Profile_NativeStart
  (JNIEnv *, jobject);

/*
 * Class:     edu_uoregon_TAU_Profile
 * Method:    NativeStop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_edu_uoregon_TAU_Profile_NativeStop
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif
