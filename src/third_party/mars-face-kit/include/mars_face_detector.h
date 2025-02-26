//
//  mars-face-kit
//
//  Created by PixPark on 2024/11/18.
//

#pragma once

#include "mars_defines.h"
namespace mars_face_kit {
class MarsFaceDetector {
public:
    static std::shared_ptr<MarsFaceDetector> CreateFaceDetector();
    // set models and resources root path
    virtual int Init(const std::string res_path) = 0;
    // detect face
    virtual int Detect(const MarsImage &in, std::vector<FaceDetectionInfo> &objects) = 0;
};
}
