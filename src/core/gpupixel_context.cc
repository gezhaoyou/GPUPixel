/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "gpupixel_context.h"
#include "util.h"

#if defined(GPUPIXEL_IOS) || defined(GPUPIXEL_MAC)

typedef void (^TaskBlock)(void);

@interface iOSHelper : NSObject {
  bool isActive;
}

- (bool)isAppActive;
- (void)run:(TaskBlock)task;
@end

@implementation iOSHelper
- (id)init {
  if (self = [super init]) {
#if defined(GPUPIXEL_IOS)
    // register notification
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(willResignActive)
               name:UIApplicationWillResignActiveNotification
             object:nil];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(didBecomeActive)
               name:UIApplicationDidBecomeActiveNotification
             object:nil];
#endif
    isActive = true;
  }
  return self;
}

- (void)willResignActive {
  @synchronized(self) {
    isActive = false;
  }
}

- (void)didBecomeActive {
  @synchronized(self) {
    isActive = true;
  }
}

- (bool)isAppActive {
  @synchronized(self) {
    return isActive;
  }
}

- (void)run:(TaskBlock)task {
  [self performSelectorOnMainThread:@selector(doRunTask:)
                         withObject:task
                      waitUntilDone:NO];
}
- (void)doRunTask:(TaskBlock)task {
  task();
}
@end

#endif

NS_GPUPIXEL_BEGIN

#if defined(GPUPIXEL_IOS) || defined(GPUPIXEL_MAC)
iOSHelper* iosHelper;
#elif defined(GPUPIXEL_ANDROID)
const std::string kRtcLogTag = "Context";
#elif defined(GPUPIXEL_WIN) || defined(GPUPIXEL_LINUX)
const unsigned int VIEW_WIDTH = 1280;
const unsigned int VIEW_HEIGHT = 720;

#endif

GPUPixelContext* GPUPixelContext::_instance = 0;
std::mutex GPUPixelContext::_mutex;

GPUPixelContext::GPUPixelContext()
    : _curShaderProgram(0),
      isCapturingFrame(false),
      captureUpToFilter(0),
      capturedFrameData(0) {
  _framebufferCache = new FramebufferCache();
  init();
}

GPUPixelContext::~GPUPixelContext() {
  dispatch_queue_.dispatch_flush();
  releaseContext();
  delete _framebufferCache;
}

GPUPixelContext* GPUPixelContext::getInstance() {
  if (!_instance) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (!_instance) {
      _instance = new (std::nothrow) GPUPixelContext;
    }
  }
  return _instance;
};

void GPUPixelContext::destroy() {
  if (_instance) {
    delete _instance;
    _instance = 0;
  }
}

void GPUPixelContext::init() {
  runSync([=] {
    Util::Log("INFO", "start init GPUPixelContext");
    this->createContext();
  });
}

FramebufferCache* GPUPixelContext::getFramebufferCache() const {
  return _framebufferCache;
}

void GPUPixelContext::setActiveShaderProgram(GLProgram* shaderProgram) {
  if (_curShaderProgram != shaderProgram) {
    _curShaderProgram = shaderProgram;
    shaderProgram->use();
  }
}

void GPUPixelContext::purge() {
  _framebufferCache->purge();
}
 
void GPUPixelContext::createContext() {
#if defined(GPUPIXEL_IOS) 
  _eglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
  [EAGLContext setCurrentContext:_eglContext];
  iosHelper = [[iOSHelper alloc] init];
#elif defined(GPUPIXEL_MAC)
  NSOpenGLPixelFormatAttribute pixelFormatAttributes[] = {
      NSOpenGLPFADoubleBuffer,
      NSOpenGLPFAOpenGLProfile,
      NSOpenGLProfileVersionLegacy,
      NSOpenGLPFAAccelerated,
      0,
      NSOpenGLPFAColorSize,
      24,
      NSOpenGLPFAAlphaSize,
      8,
      NSOpenGLPFADepthSize,
      24,
      0};

  _pixelFormat =
      [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes];
  imageProcessingContext =
      [[NSOpenGLContext alloc] initWithFormat:_pixelFormat shareContext:nil];

  GLint interval = 0;
  [imageProcessingContext makeCurrentContext];
  [imageProcessingContext setValues:&interval
                       forParameter:NSOpenGLContextParameterSwapInterval];

#elif defined(GPUPIXEL_ANDROID)
  Util::Log("INFO", "GPUPixelContext::createContext start");
  context_inited = true;
  m_surfacewidth = 1;
  m_surfaceheight = 1;  // no use
  m_gpu_context = new _gpu_context_t;
  memset(m_gpu_context, 0, sizeof(_gpu_context_t));
  m_gpu_context->egldisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (EGL_NO_DISPLAY == m_gpu_context->egldisplay) {
    // err_log("eglGetDisplay Error!");
    Util::Log("ERROR", "eglGetDisplay Error!");
    return;
  }

  GLint majorVersion;
  GLint minorVersion;
  if (!eglInitialize(m_gpu_context->egldisplay, &majorVersion, &minorVersion)) {
    // err_log("eglInitialize Error!");
    Util::Log("ERROR", "eglInitialize Error!");
    return;
  }
  // info_log("GL Version minor:%d major:%d", minorVersion, majorVersion);

  // 如果创建WindowSurface使用EGL_WINDOW_BIT，PBufferSurface使用EGL_PBUFFER_BIT
  EGLint config_attribs[] = {EGL_BLUE_SIZE,
                             8,
                             EGL_GREEN_SIZE,
                             8,
                             EGL_RED_SIZE,
                             8,
                             EGL_RENDERABLE_TYPE,
                             EGL_OPENGL_ES2_BIT,
                             EGL_SURFACE_TYPE,
                             EGL_PBUFFER_BIT,
                             EGL_NONE};

  int num_configs = 0;
  EGLConfig eglConfig;
  if (!eglChooseConfig(m_gpu_context->egldisplay, config_attribs, &eglConfig, 1,
                       &num_configs)) {
    // err_log("eglChooseConfig Error!");
    Util::Log("ERROR", "eglChooseConfig Error!");
    return;
  }

  EGLint context_attrib[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  m_gpu_context->eglcontext = eglCreateContext(
      m_gpu_context->egldisplay, eglConfig, EGL_NO_CONTEXT, context_attrib);
  if (EGL_NO_CONTEXT == m_gpu_context->eglcontext) {
    // err_log("eglCreateContext Error!");
    Util::Log("ERROR", "eglCreateContext Error!");
    return;
  }

  int attribListPbuffer[] = {EGL_WIDTH, m_surfacewidth, EGL_HEIGHT,
                             m_surfaceheight, EGL_NONE};

  m_gpu_context->eglsurface = eglCreatePbufferSurface(
      m_gpu_context->egldisplay, eglConfig, attribListPbuffer);
  if (EGL_NO_SURFACE == m_gpu_context->eglsurface) {
    // err_log("eglCreatePbufferSurface Error!");
    Util::Log("ERROR", "eglCreatePbufferSurface Error!");
    return;
  }

  if (!eglQuerySurface(m_gpu_context->egldisplay, m_gpu_context->eglsurface,
                       EGL_WIDTH, &m_surfacewidth) ||
      !eglQuerySurface(m_gpu_context->egldisplay, m_gpu_context->eglsurface,
                       EGL_HEIGHT, &m_surfaceheight)) {
    // err_log("eglQuerySurface Error!");
    Util::Log("ERROR", "eglQuerySurface Error!");
    return;
  }
  // info_log("Create Surface width:%d height:%d", m_surfacewidth,
  // m_surfaceheight);
  Util::Log("INFO", "Create Surface width:%d height:%d", m_surfacewidth,
            m_surfaceheight);
#elif defined(GPUPIXEL_WIN) || defined(GPUPIXEL_LINUX)
  int ret = glfwInit();

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  if (ret) {
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  } else {
    // todo log error
    return;
  }
  gl_context_ = glfwCreateWindow(VIEW_WIDTH, VIEW_HEIGHT, "gpupixel opengl context", NULL, NULL);
  if (!gl_context_) {
    // todo log error
    glfwTerminate();
    return;
  }
  glfwMakeContextCurrent(gl_context_);

  gladLoadGL();
#endif
}

void GPUPixelContext::useAsCurrent() {
  #if defined(GPUPIXEL_IOS)
    if ([EAGLContext currentContext] != _eglContext) {
      [EAGLContext setCurrentContext:_eglContext];
    }
  #elif defined(GPUPIXEL_MAC)
    if ([NSOpenGLContext currentContext] != imageProcessingContext) {
      [imageProcessingContext makeCurrentContext];
    }
#elif defined(GPUPIXEL_ANDROID)
  if (!eglMakeCurrent(m_gpu_context->egldisplay, m_gpu_context->eglsurface,
                      m_gpu_context->eglsurface, m_gpu_context->eglcontext)) {
    // err_log("Set Current Context Error.");
    Util::Log("ERROR", "Set Current Context Error!");
  }
#elif defined(GPUPIXEL_WIN) || defined(GPUPIXEL_LINUX)
   if (glfwGetCurrentContext() != gl_context_) {
    glfwMakeContextCurrent(gl_context_);
  }
#endif
}

void GPUPixelContext::presentBufferForDisplay() {
#if defined(GPUPIXEL_IOS)
  [_eglContext presentRenderbuffer:GL_RENDERBUFFER];
#elif defined(GPUPIXEL_MAC)

#endif
}

void GPUPixelContext::releaseContext() {
#if defined(GPUPIXEL_WIN) || defined(GPUPIXEL_LINUX)
  if (gl_context_) {
    glfwDestroyWindow(gl_context_);
  }
  glfwTerminate();
#elif defined(GPUPIXEL_ANDROID)
  if (!context_inited) {
    return;
  }
  context_inited = false;
  if (m_gpu_context != nullptr && m_gpu_context->egldisplay != EGL_NO_DISPLAY) {
    if (m_gpu_context->eglcontext != EGL_NO_CONTEXT) {
      eglDestroyContext(m_gpu_context->egldisplay, m_gpu_context->eglcontext);
    }
    if (m_gpu_context->eglsurface != EGL_NO_SURFACE) {
      eglDestroySurface(m_gpu_context->egldisplay, m_gpu_context->eglsurface);
    }

    eglMakeCurrent(m_gpu_context->egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);
    if (!eglTerminate(m_gpu_context->egldisplay)) {
      // err_log("Free egldisplay error!");
      Util::Log("ERROR", "Free egldisplay Error!");
    }
  }

  if (m_gpu_context != nullptr) {
    delete m_gpu_context;
    m_gpu_context = nullptr;
  }
#endif
}

void GPUPixelContext::runSync(std::function<void(void)> func) {
    // todo fix android
#if defined(GPUPIXEL_ANDROID)
    func();
#else
 
    dispatch_queue_.dispatch_sync([=](){
        useAsCurrent();
        func();
    });
#endif
}

NS_GPUPIXEL_END
