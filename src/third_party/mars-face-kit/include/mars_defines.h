//
//  mars-face-kit
//
//  Created by PixPark on 2024/11/18.
//
#pragma once

#include <string>
#include <vector>
#include <memory>
// version
#define MARSFACE_SDK_VERSION "v1.2.2"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
//define something for Windows (32-bit and 64-bit, this part is common)
#ifdef _WIN64
//define something for Windows (64-bit only)
#define MARSFACE_WIN
#else
//define something for Windows (32-bit only)
#define MARSFACE_WIN
#endif
#elif __APPLE__
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// iOS, tvOS, or watchOS Simulator
#define MARSFACE_IOS
#elif TARGET_OS_MACCATALYST
// Mac's Catalyst (ports iOS API into Mac, like UIKit).
#define MARSFACE_IOS
#elif TARGET_OS_IPHONE
// iOS, tvOS, or watchOS device
#define MARSFACE_IOS
#elif TARGET_OS_MAC
// Other kinds of Apple platforms
#define MARSFACE_MAC
#else
#   error "Unknown Apple platform"
#endif
#elif __ANDROID__
// Below __linux__ check should be enough to handle Android,
// but something may be unique to Android.
#define MARSFACE_ANDROID
#elif __linux__
// linux
#define MARSFACE_LINUX
#elif __emscripten__
#define MARSFACE_LINUX
#elif __unix__ // all unices not caught above
// Unix
#define MARSFACE_LINUX
#elif defined(_POSIX_VERSION)
// POSIX
#   error "Unknown Platform"
#define MARSFACE_POSIX
#else
#   error "Unknown Platform"
#endif

namespace mars_face_kit {
struct Point2D {
    float x;
    float y;
};

struct Point3D {
    float x;
    float y;
    float z;
    float visibility;  // landmark score
    float presence;     // landmark in frame score
};

struct Rect {
    float left;
    float top;
    float right;
    float bottom;
};

struct FaceDetectionInfo {
    Rect rect;
    std::vector<Point2D> landmarks;
    std::vector<Point3D> landmarks3d;
    
    float visibilities;
    int face_id;
    /// Confidence score for the input frame to be a face.
    float score;
    /// Pitch angle, up -, down +. See
    float pitch;
    /// Roll angle, left -, right +. See
    float roll;
    /// Yaw angle, left -, right +. See
    float yaw;
    
    float angle;
    
    uint32_t face_action;
};

enum PixelFormat {
    RGBA = 0,
    RGB = 1,
    BGR = 2,
    GRAY = 3,
    BGRA = 4,
    YCrCb = 5,
    YUV_NV21 = 6,
    YUV_NV12 = 7,
    YUV_I420 = 8,
};

enum RotateType {
    CLOCKWISE_ROTATE_0 = 0,
    CLOCKWISE_ROTATE_90 = 1,
    CLOCKWISE_ROTATE_180 = 2,
    CLOCKWISE_ROTATE_270 = 3,
};

typedef struct {
    uint8_t *data;                  ///< 图像数据指针
    PixelFormat pixel_format;       ///< 像素格式
    int width;                      ///< 宽度(以像素为单位)
    int height;                     ///< 高度(以像素为单位)
    int width_step;                 ///< 跨度, 即每行所占的字节数
    double timestamp;              ///< 时间戳
    RotateType rotate_type;
} MarsImage;

struct Embedding {
    std::vector<float> float_embedding;
    std::string quantized_embedding;
};

struct ClassifierInfo {
    int id_;
    float score_;
};

}
