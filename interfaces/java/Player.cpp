/*
 * Copyright (c) 2018 Spotify AB.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <NFSmartPlayer/nf_smart_player.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <mutex>

#include "com_spotify_nativeformat_Player.h"

struct NF_SMART_PLAYER_JNI_HANDLE {
  JNIEnv *environment;
  jobject player_object;
  char *final_result;
  int final_result_length;
  NF_SMART_PLAYER_JNI_HANDLE(JNIEnv *environment, jobject player_object)
      : environment(environment),
        player_object(player_object),
        final_result(nullptr),
        final_result_length(0) {}
  virtual ~NF_SMART_PLAYER_JNI_HANDLE() {
    if (final_result != nullptr) {
      free(final_result);
    }
  }
};

static jfieldID playerPointerFieldFromJNI(JNIEnv *environment,
                                          jobject player_object) {
  jclass java_class = environment->GetObjectClass(player_object);
  return environment->GetFieldID(java_class, "nPlayerPointer", "J");
}

static NF_SMART_PLAYER_HANDLE handleFromJNI(JNIEnv *environment,
                                            jobject player_object) {
  jfieldID player_pointer_field_ID =
      playerPointerFieldFromJNI(environment, player_object);
  jlong player_pointer_long =
      environment->GetLongField(player_object, player_pointer_field_ID);
  NF_SMART_PLAYER_HANDLE handle = (NF_SMART_PLAYER_HANDLE)player_pointer_long;
  if (handle == nullptr) {
    jclass null_pointer_exception_class =
        environment->FindClass("java/lang/NullPointerException");
    if (null_pointer_exception_class != nullptr) {
      environment->ThrowNew(null_pointer_exception_class,
                            "Cannot find the smartplayer handle in C");
    }
  }
  return handle;
}

static jobject listenerObjectFromJNI(JNIEnv *environment,
                                     jobject player_object) {
  jclass player_class = environment->GetObjectClass(player_object);
  jfieldID player_listener_field = environment->GetFieldID(
      player_class, "listener", "Lcom/spotify/nativeformat/PlayerListener");
  return environment->GetObjectField(player_object, player_listener_field);
}

static jmethodID methodFromPlayerListenerJNI(JNIEnv *environment,
                                             jobject player_listener,
                                             const char *name,
                                             const char *sig) {
  jclass player_listener_class = environment->GetObjectClass(player_listener);
  return environment->GetMethodID(player_listener_class, name, sig);
}

static void JNILoadCallback(void *context, int success,
                            const char *error_message) {
  NF_SMART_PLAYER_JNI_HANDLE *jni_handle =
      (NF_SMART_PLAYER_JNI_HANDLE *)context;
  jobject player_listener =
      listenerObjectFromJNI(jni_handle->environment, jni_handle->player_object);
  jmethodID native_format_did_load = methodFromPlayerListenerJNI(
      jni_handle->environment, player_listener, "nativeFormatDidLoad",
      "(ZLjava/lang/String;)V");
  jstring jerror_message = jni_handle->environment->NewStringUTF(error_message);
  jni_handle->environment->CallObjectMethod(
      player_listener, native_format_did_load, !!success, jerror_message);
}

static const char *JNIResolveVariableCallback(void *context,
                                              const char *plugin_namespace,
                                              const char *variable_identifier) {
  NF_SMART_PLAYER_JNI_HANDLE *jni_handle =
      (NF_SMART_PLAYER_JNI_HANDLE *)context;
  jobject player_listener =
      listenerObjectFromJNI(jni_handle->environment, jni_handle->player_object);
  jmethodID native_format_resolve_variable_method = methodFromPlayerListenerJNI(
      jni_handle->environment, player_listener, "nativeFormatResolveVariable",
      "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
  jstring jplugin_namespace =
      jni_handle->environment->NewStringUTF(plugin_namespace);
  jstring jvariable_identifier =
      jni_handle->environment->NewStringUTF(variable_identifier);
  jobject result = jni_handle->environment->CallObjectMethod(
      player_listener, native_format_resolve_variable_method, jplugin_namespace,
      jvariable_identifier);
  jstring string_result = (jstring)result;
  jboolean is_copy = false;
  const char *cstring_result =
      jni_handle->environment->GetStringUTFChars(string_result, &is_copy);
  jsize string_size = jni_handle->environment->GetStringLength(string_result);
  if (string_size >= jni_handle->final_result_length) {
    if (jni_handle->final_result != nullptr) {
      free(jni_handle->final_result);
    }
    jni_handle->final_result_length = string_size + 1;
    jni_handle->final_result = (char *)malloc(jni_handle->final_result_length);
  }
  memset(jni_handle->final_result, '\0', jni_handle->final_result_length);
  memcpy(jni_handle->final_result, cstring_result, string_size);
  jni_handle->environment->ReleaseStringUTFChars(string_result, cstring_result);
  return jni_handle->final_result;
}

static void JNIMessageCallback(void *context, const char *message_identifier,
                               const char *sender_identifier,
                               NF_SMART_PLAYER_MESSAGE_TYPE message_type,
                               const void *payload) {
  NF_SMART_PLAYER_JNI_HANDLE *jni_handle =
      (NF_SMART_PLAYER_JNI_HANDLE *)context;
  jobject player_listener =
      listenerObjectFromJNI(jni_handle->environment, jni_handle->player_object);
  jmethodID native_format_received_message_method = methodFromPlayerListenerJNI(
      jni_handle->environment, player_listener, "nativeFormatReceivedMessage",
      "(Ljava/lang/String;Ljava/lang/String;Lcom/spotify/nativeformat/"
      "MessageType;Ljava/lang/Object;)V");
  jstring jmessage_identifier =
      jni_handle->environment->NewStringUTF(message_identifier);
  jstring jsender_identifier =
      jni_handle->environment->NewStringUTF(sender_identifier);
  jclass message_type_class = jni_handle->environment->FindClass(
      "com/spotify/nativeformat/MessageType");
  jmethodID message_type_constructor = jni_handle->environment->GetMethodID(
      message_type_class, "<init>", "(I)V");
  jobject jmessage_type = jni_handle->environment->NewObject(
      message_type_class, message_type_constructor, (jint)message_type);
  jobject jpayload = nullptr;
  switch (message_type) {
    case NF_SMART_PLAYER_MESSAGE_TYPE_NONE: {
      jclass object_class =
          jni_handle->environment->FindClass("java/lang/Object");
      jmethodID object_constructor = jni_handle->environment->GetMethodID(
          message_type_class, "<init>", "()V");
      jpayload =
          jni_handle->environment->NewObject(object_class, object_constructor);
      break;
    }
    case NF_SMART_PLAYER_MESSAGE_TYPE_GENERIC: {
      jstring jpayload_string =
          jni_handle->environment->NewStringUTF((const char *)payload);
      jpayload = (jstring)jpayload_string;
      break;
    }
  }
  jni_handle->environment->CallObjectMethod(
      player_listener, native_format_received_message_method,
      jmessage_identifier, jsender_identifier, jmessage_type, jpayload);
}

static bool JNIIsClosed(JNIEnv *environment, jobject player_object) {
  jclass java_class = environment->GetObjectClass(player_object);
  jfieldID closed_field = environment->GetFieldID(java_class, "closed", "Z");
  return environment->GetBooleanField(player_object, closed_field);
}

JNIEXPORT jboolean JNICALL Java_com_spotify_nativeformat_Player_getPlaying(
    JNIEnv *environment, jobject player_object) {
  if (JNIIsClosed(environment, player_object)) {
    return false;
  }
  NF_SMART_PLAYER_HANDLE handle = handleFromJNI(environment, player_object);
  if (handle == nullptr) {
    return false;
  }
  return smartplayer_is_playing(handle);
}

JNIEXPORT void JNICALL Java_com_spotify_nativeformat_Player_setPlaying(
    JNIEnv *environment, jobject player_object, jboolean playing) {
  if (JNIIsClosed(environment, player_object)) {
    return;
  }
  NF_SMART_PLAYER_HANDLE handle = handleFromJNI(environment, player_object);
  if (handle == nullptr) {
    return;
  }
  smartplayer_set_playing(handle, playing);
}

JNIEXPORT jdouble JNICALL Java_com_spotify_nativeformat_Player_getTime(
    JNIEnv *environment, jobject player_object) {
  if (JNIIsClosed(environment, player_object)) {
    return 0.0;
  }
  NF_SMART_PLAYER_HANDLE handle = handleFromJNI(environment, player_object);
  if (handle == nullptr) {
    return 0.0;
  }
  return smartplayer_render_time(handle);
}

JNIEXPORT void JNICALL Java_com_spotify_nativeformat_Player_setTime(
    JNIEnv *environment, jobject player_object, jdouble time) {
  if (JNIIsClosed(environment, player_object)) {
    return;
  }
  NF_SMART_PLAYER_HANDLE handle = handleFromJNI(environment, player_object);
  if (handle == nullptr) {
    return;
  }
  smartplayer_set_render_time(handle, time);
}

JNIEXPORT jboolean JNICALL Java_com_spotify_nativeformat_Player_getLoaded(
    JNIEnv *environment, jobject player_object) {
  if (JNIIsClosed(environment, player_object)) {
    return false;
  }
  NF_SMART_PLAYER_HANDLE handle = handleFromJNI(environment, player_object);
  if (handle == nullptr) {
    return false;
  }
  return smartplayer_is_loaded(handle);
}

JNIEXPORT jfloat JNICALL Java_com_spotify_nativeformat_Player_getValueForPath(
    JNIEnv *environment, jobject player_object, jstring path) {
  if (JNIIsClosed(environment, player_object)) {
    return 0.0f;
  }
  NF_SMART_PLAYER_HANDLE handle = handleFromJNI(environment, player_object);
  if (handle == nullptr) {
    return 0.0f;
  }
  const char *cpath = environment->GetStringUTFChars(path, nullptr);
  float value = smartplayer_get_value_for_path(handle, cpath);
  environment->ReleaseStringUTFChars(path, cpath);
  return value;
}

JNIEXPORT void JNICALL Java_com_spotify_nativeformat_Player_setValueForPath(
    JNIEnv *environment, jobject player_object, jstring path,
    jfloatArray values) {
  if (JNIIsClosed(environment, player_object)) {
    return;
  }
  NF_SMART_PLAYER_HANDLE handle = handleFromJNI(environment, player_object);
  if (handle == nullptr) {
    return;
  }
  const char *cpath = environment->GetStringUTFChars(path, nullptr);
  jfloat *cvalues = environment->GetFloatArrayElements(values, nullptr);
  jsize values_length = environment->GetArrayLength(values);
  smartplayer_set_values_for_path(handle, cpath, cvalues, values_length);
  environment->ReleaseFloatArrayElements(values, cvalues, 0);
  environment->ReleaseStringUTFChars(path, cpath);
}

JNIEXPORT void JNICALL Java_com_spotify_nativeformat_Player_setJson(
    JNIEnv *environment, jobject player_object, jstring json) {
  if (JNIIsClosed(environment, player_object)) {
    return;
  }
  NF_SMART_PLAYER_HANDLE handle = handleFromJNI(environment, player_object);
  if (handle == nullptr) {
    return;
  }
  const char *cjson = environment->GetStringUTFChars(json, nullptr);
  smartplayer_set_json(handle, cjson, &JNILoadCallback);
  environment->ReleaseStringUTFChars(json, cjson);
}

JNIEXPORT void JNICALL Java_com_spotify_nativeformat_Player_sendMessage(
    JNIEnv *environment, jobject player_object, jstring message,
    jobject messageType, jobject payload) {
  if (JNIIsClosed(environment, player_object)) {
    return;
  }
  NF_SMART_PLAYER_HANDLE handle = handleFromJNI(environment, player_object);
  if (handle == nullptr) {
    return;
  }
  const char *cmessage = environment->GetStringUTFChars(message, nullptr);
  jclass envelope_type_class =
      environment->FindClass("package/of/envelopeType");
  jmethodID envelope_type_method =
      environment->GetMethodID(envelope_type_class, "ordinal", "()I");
  jint message_type_value =
      environment->CallIntMethod(messageType, envelope_type_method);
  switch (message_type_value) {
    case NF_SMART_PLAYER_MESSAGE_TYPE_NONE:
      smartplayer_send_message(handle, cmessage,
                               NF_SMART_PLAYER_MESSAGE_TYPE_NONE, nullptr);
      break;
    case NF_SMART_PLAYER_MESSAGE_TYPE_GENERIC: {
      jstring payload_string = (jstring)payload;
      const char *cpayload =
          environment->GetStringUTFChars(payload_string, nullptr);
      smartplayer_send_message(handle, cmessage,
                               NF_SMART_PLAYER_MESSAGE_TYPE_GENERIC, cpayload);
      environment->ReleaseStringUTFChars(payload_string, cpayload);
      break;
    }
  }
  environment->ReleaseStringUTFChars(message, cmessage);
}

JNIEXPORT jstring JNICALL Java_com_spotify_nativeformat_Player_getPlayerVersion(
    JNIEnv *environment, jclass player_class) {
  jstring variable = environment->NewStringUTF(NF_SMART_PLAYER_VERSION);
  return variable;
}

JNIEXPORT void JNICALL Java_com_spotify_nativeformat_Player_init(
    JNIEnv *environment, jobject player_object) {
  NF_SMART_PLAYER_JNI_HANDLE *jni_handle =
      new NF_SMART_PLAYER_JNI_HANDLE(environment, player_object);
  NF_SMART_PLAYER_SETTINGS settings = {0, 0, "", 0, false};
  jclass player_class = environment->GetObjectClass(player_object);
  jfieldID driver_type_field = environment->GetFieldID(
      player_class, "driverType", "Lcom/spotify/nativeformat/DriverType");
  jobject driver_type =
      environment->GetObjectField(player_object, driver_type_field);
  jclass envelope_type_class =
      environment->FindClass("package/of/envelopeType");
  jmethodID envelope_type_method =
      environment->GetMethodID(envelope_type_class, "ordinal", "()I");
  jint driver_type_value =
      environment->CallIntMethod(driver_type, envelope_type_method);
  jfieldID output_file_field =
      environment->GetFieldID(player_class, "outputFile", "Ljava/lang/String");
  jobject output_file_object =
      environment->GetObjectField(player_object, output_file_field);
  jstring output_file_string = (jstring)output_file_object;
  const char *coutput_file =
      environment->GetStringUTFChars(output_file_string, nullptr);
  NF_SMART_PLAYER_HANDLE handle = smartplayer_open(
      &JNIResolveVariableCallback, jni_handle, settings,
      (NF_SMART_PLAYER_DRIVER_TYPE)driver_type_value, coutput_file);
  environment->ReleaseStringUTFChars(output_file_string, coutput_file);
  smartplayer_set_message_callback(handle, &JNIMessageCallback);
  jfieldID player_pointer_field_ID =
      playerPointerFieldFromJNI(environment, player_object);
  environment->SetLongField(player_object, player_pointer_field_ID,
                            (jlong)handle);
}

JNIEXPORT void JNICALL Java_com_spotify_nativeformat_Player_close(
    JNIEnv *environment, jobject player_object) {
  jclass java_class = environment->GetObjectClass(player_object);
  jfieldID closed_field = environment->GetFieldID(java_class, "closed", "Z");
  environment->SetBooleanField(player_object, closed_field, true);
  NF_SMART_PLAYER_HANDLE handle = handleFromJNI(environment, player_object);
  NF_SMART_PLAYER_JNI_HANDLE *jni_handle =
      (NF_SMART_PLAYER_JNI_HANDLE *)smartplayer_get_context(handle);
  delete jni_handle;
  smartplayer_close(handle);
}
