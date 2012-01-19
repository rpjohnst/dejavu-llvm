#include <jni.h>

extern "C"
JNIEXPORT void JNICALL Java_org_dejavu_Driver_compile(
	JNIEnv *env, jclass Driver, jobject target, jboolean debug, jobject progress
) {
	jclass ProgressPane = env->GetObjectClass(progress);
	jmethodID append = env->GetMethodID(ProgressPane, "append", "(Ljava/lang/String;)V");
	jmethodID percent = env->GetMethodID(ProgressPane, "percent", "(I)V");
	jmethodID message = env->GetMethodID(ProgressPane, "message", "(Ljava/lang/String;)V");

	env->CallVoidMethod(progress, append, env->NewStringUTF("hello\n"));
	env->CallVoidMethod(progress, percent, 50);
	env->CallVoidMethod(progress, message, env->NewStringUTF("processing"));
}
