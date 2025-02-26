/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "util.h"
#include "gpupixel_context.h"
#include <cstdarg>
#if defined(GPUPIXEL_ANDROID)
#include <android/log.h>
#include "jni_helpers.h"
#elif defined(GPUPIXEL_IOS)
#import <Foundation/foundation.h>
#import <UIKit/UIKit.h>
#elif defined(GPUPIXEL_WIN)
#include <malloc.h>
#include <stdio.h>
#endif
 
#if defined(GPUPIXEL_IOS) || defined(GPUPIXEL_MAC)
@interface GPXObjcHelper : NSObject
+ (NSString*)getResourcePath:(NSString*)name;
+ (NSString*)getBundleResourceWithName:(NSString*)bundleName
                              fileName:(NSString*)fileName
                                  type:(NSString*)fileType;
@end

@implementation GPXObjcHelper

+ (NSString*)getResourcePath:(NSString*)name {
  NSString* path = [[[NSBundle bundleForClass:self.class] resourcePath]
      stringByAppendingPathComponent:name];
  return path;
}

+ (NSString*)getModelsPath {
  NSString* path = [[NSBundle bundleForClass:self.class] resourcePath];
  return path;
}

+ (NSString*)getBundleResourceWithName:(NSString*)bundleName
                              fileName:(NSString*)fileName
                                  type:(NSString*)fileType {
  NSBundle* sdkMainBundle = [NSBundle bundleForClass:[self class]];
  NSString* resourceBundlePath =
      [sdkMainBundle pathForResource:bundleName ofType:@"bundle"];
  NSBundle* resourceBundle = [NSBundle bundleWithPath:resourceBundlePath];
  NSString* filePath =
      [resourceBundle pathForResource:fileName ofType:fileType];
  return filePath;
}

@end

#endif


NS_GPUPIXEL_BEGIN

std::string Util::resourceRoot = "";

std::string Util::getResourcePath(std::string name) {
#if defined(GPUPIXEL_IOS) || defined(GPUPIXEL_MAC)
  NSString* oc_path = [GPXObjcHelper
      getResourcePath:[[NSString alloc] initWithUTF8String:name.c_str()]];
  std::string path = [oc_path UTF8String];
#elif defined(GPUPIXEL_ANDROID)
  std::string path = getResourcePathJni(name);
#else
    std::string path = resourceRoot.empty() ? name : (resourceRoot + "/" + name);
#endif
  return path;
}

void Util::setResourceRoot(std::string root) {
    resourceRoot = root;
}
#if defined(GPUPIXEL_IOS) || defined(GPUPIXEL_MAC)
std::string Util::getResourcePath(std::string bundle_name,
                                  std::string file_name,
                                  std::string type) {
  NSString* oc_path = [GPXObjcHelper
      getBundleResourceWithName:[[NSString alloc]
                                    initWithUTF8String:bundle_name.c_str()]
                       fileName:[[NSString alloc]
                                    initWithUTF8String:file_name.c_str()]
                           type:[[NSString alloc]
                                    initWithUTF8String:type.c_str()]];
  std::string path = [oc_path UTF8String];
  return path;
}

#endif

#ifdef GPUPIXEL_WIN
int vasprintf(char** strp, const char* fmt, va_list ap) {
  // _vscprintf tells you how big the buffer needs to be
  int len = _vscprintf(fmt, ap);
  if (len == -1) {
    return -1;
  }
  size_t size = (size_t)len + 1;
  char* str = (char*)malloc(size);
  if (!str) {
    return -1;
  }
  int r = _vsprintf_s_l(str, len + 1, fmt, nullptr, ap);
  if (r == -1) {
    free(str);
    return -1;
  }
  *strp = str;
  return r;
}
#endif


#if defined(GPUPIXEL_ANDROID)
std::string Util::getResourcePathJni(std::string name) {
  // Todo(Jeayo) @see https://developer.android.com/ndk/guides/image-decoder?hl=zh-cn
  JavaVM *jvm = GetJVM();
  JNIEnv *env = GetEnv(jvm);

  // 定义类路径和方法签名
  const char *className = "com/pixpark/gpupixel/GPUPixel";
  const char *methodName = "getResource_path";
  const char *methodSignature = "()Ljava/lang/String;";

  // GPUPixel
  jclass class_id = env->FindClass(className);
  if (class_id == NULL) {
    return NULL;
  }

  jmethodID mtd = env->GetStaticMethodID(class_id, methodName,
                                         methodSignature);
  if (mtd == NULL) {
    return NULL;
  }

  // 调用createBitmap方法
  jstring path = (jstring)env->CallStaticObjectMethod(class_id,
                                                      mtd);

  std::string str = env->GetStringUTFChars(path, nullptr);
  env->DeleteLocalRef(class_id);
  return str + "/" + name;
}
#endif

std::string Util::str_format(const char* fmt, ...) {
  std::string strResult = "";
  if (NULL != fmt) {
    va_list marker;
    va_start(marker, fmt);
    char* buf = 0;
    int result = vasprintf(&buf, fmt, marker);
    if (!buf) {
      va_end(marker);
      return strResult;
    }

    if (result < 0) {
      free(buf);
      va_end(marker);
      return strResult;
    }

    result = (int)strlen(buf);
    strResult.append(buf, result);
    free(buf);
    va_end(marker);
  }
  return strResult;
}

int64_t Util::nowTimeMs() {
  auto time_now = std::chrono::system_clock::now();
  auto duration_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      time_now.time_since_epoch());
  int64_t ts = duration_in_ms.count();
  return ts;
}

 void Util::Log(const std::string& tag, std::string format, ...) {
  char buffer[10240];
  va_list args;
  va_start(args, format);
#if defined(GPUPIXEL_WIN)
  // 使用 vsnprintf_s 在 Windows 上
  vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format.c_str(), args);
#else
  // 使用 vsnprintf 在其他平台上
  vsnprintf(buffer, sizeof(buffer), format.c_str(), args);
#endif
  va_end(args);
#if defined(GPUPIXEL_ANDROID)
  __android_log_print(ANDROID_LOG_INFO, tag.c_str(), "%s", buffer);
#elif defined(GPUPIXEL_IOS) || defined(GPUPIXEL_MAC)
  NSLog(@"%s", buffer);
#elif defined(GPUPIXEL_LINUX)
  printf("%s\n", buffer);
#endif
}

NS_GPUPIXEL_END
