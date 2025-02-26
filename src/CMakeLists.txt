CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# 设置引入的 cmake 文件路径为当前目录
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

# 定义当前模块的名称
SET(PROJECT_NAME "gpupixel")
PROJECT(${PROJECT_NAME})

# 定义导出 API 的宏定义(Windows下才有意义)
ADD_DEFINITIONS(-DMYMATH_EXPORT_LIBRARY)

# Detect platform
# --------
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(CURRENT_OS "linux")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    SET(CURRENT_OS "windows")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	SET(CURRENT_OS "macos")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "iOS")
	SET(CURRENT_OS "ios")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Android")
	SET(CURRENT_OS "android")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
	SET(CURRENT_OS "wasm")
	add_definitions(-D__emscripten__)
ELSE()
    MESSAGE(FATAL_ERROR "NOT SUPPORT THIS SYSTEM")
ENDIF()

# Config build output path
# --------
SET(OUTPUT_INSTALL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../output")
SET(OUTPUT_RESOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../output/resources")
SET(CMAKE_INCLUDE_OUTPUT_DIRECTORY "${OUTPUT_INSTALL_PATH}/include")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_INSTALL_PATH}/library/${CURRENT_OS}")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_INSTALL_PATH}/library/${CURRENT_OS}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_INSTALL_PATH}/library/${CURRENT_OS}")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Config source and header file
# ---------
# header include path
INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/core
	${CMAKE_CURRENT_SOURCE_DIR}/filter
	${CMAKE_CURRENT_SOURCE_DIR}/source
	${CMAKE_CURRENT_SOURCE_DIR}/target
	${CMAKE_CURRENT_SOURCE_DIR}/utils
	${CMAKE_CURRENT_SOURCE_DIR}/face_detect
	${CMAKE_CURRENT_SOURCE_DIR}/android/jni
	${CMAKE_CURRENT_SOURCE_DIR}/target/objc
	${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/include
	${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
	${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include
	${CMAKE_CURRENT_SOURCE_DIR}/third_party/libyuv/include
	${CMAKE_CURRENT_SOURCE_DIR}/third_party/mars-face-kit/include
)
 
# Add common source file
FILE(GLOB SOURCE_FILES     
	"${CMAKE_CURRENT_SOURCE_DIR}/core/*"        
	"${CMAKE_CURRENT_SOURCE_DIR}/filter/*"         
	"${CMAKE_CURRENT_SOURCE_DIR}/source/*"       
	"${CMAKE_CURRENT_SOURCE_DIR}/target/*"                               
	"${CMAKE_CURRENT_SOURCE_DIR}/face_detect/*"                 
	"${CMAKE_CURRENT_SOURCE_DIR}/utils/*"                 
	"${CMAKE_CURRENT_SOURCE_DIR}/third_party/libyuv/source/*"
)

# Add export header file
FILE(GLOB EXPORT_HEADER 
	"${CMAKE_CURRENT_SOURCE_DIR}/core/*.h"         
	"${CMAKE_CURRENT_SOURCE_DIR}/filter/*.h"         
	"${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"       
	"${CMAKE_CURRENT_SOURCE_DIR}/target/*.h"                      
	"${CMAKE_CURRENT_SOURCE_DIR}/utils/*.h"                 
	"${CMAKE_CURRENT_SOURCE_DIR}/face_detect/*.h"                 
)

FILE(GLOB RESOURCE_FILES 
	"${CMAKE_CURRENT_SOURCE_DIR}/resources/*"   
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mars-face-kit/models/*"                      
)

# Add platform source and header and lib link search path
IF(${CURRENT_OS} STREQUAL "windows") 	
	set(CMAKE_SHARED_LIBRARY_PREFIX "")													# windows
	# Source 
	FILE(GLOB GLAD_SOURCE_FILE  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/src/*.c" )
	list(APPEND SOURCE_FILES ${GLAD_SOURCE_FILE})

	# link libs find path
	LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/lib-mingw-w64)
ELSEIF(${CURRENT_OS} STREQUAL "linux" OR ${CURRENT_OS} STREQUAL "wasm")	
	# Source 
	FILE(GLOB GLAD_SOURCE_FILE  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/src/*.c" )
	list(APPEND SOURCE_FILES ${GLAD_SOURCE_FILE})
ELSEIF(${CURRENT_OS} STREQUAL "macos" OR ${CURRENT_OS} STREQUAL "ios")						# ios and mac
	# Header
	FILE(GLOB OBJC_HEADER_FILE  "${CMAKE_CURRENT_SOURCE_DIR}/target/objc/*.h")
	list(APPEND EXPORT_HEADER 	${OBJC_HEADER_FILE})
	
	# Source 
	FILE(GLOB OBJC_SOURCE_FILE  "${CMAKE_CURRENT_SOURCE_DIR}/target/objc/*")
	list(APPEND SOURCE_FILES ${OBJC_SOURCE_FILE})

	
ELSEIF(${CURRENT_OS} STREQUAL "android")													# android 
	# Header
	FILE(GLOB OBJC_HEADER_FILE  "${CMAKE_CURRENT_SOURCE_DIR}/android/jni/*.h")
	list(APPEND EXPORT_HEADER 	${OBJC_HEADER_FILE})
	
	# Source 
	FILE(GLOB JNI_SOURCE_FILE  "${CMAKE_CURRENT_SOURCE_DIR}/android/jni/*")
	list(APPEND SOURCE_FILES ${JNI_SOURCE_FILE})

ENDIF()

# Config project 
# ----------
# build shared or static lib
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${RESOURCE_FILES})

# set platform project 
IF(${CURRENT_OS} STREQUAL "linux")
	# 设置要构建的目标库的名称和类型
	add_library(mars-face-kit STATIC IMPORTED)
	# 设置目标库的实际路径
	set_target_properties(mars-face-kit PROPERTIES IMPORTED_LOCATION
	${CMAKE_CURRENT_SOURCE_DIR}/third_party/mars-face-kit/libs/${CURRENT_OS}/libmars-face-kit.a)

	add_library(MNN STATIC IMPORTED)
	# 设置目标库的实际路径
	set_target_properties(MNN PROPERTIES IMPORTED_LOCATION
	${CMAKE_CURRENT_SOURCE_DIR}/third_party/mnn/libs/${CURRENT_OS}/libMNN.a)

ELSEIF(${CURRENT_OS} STREQUAL "windows")
	# 设置要构建的目标库的名称和类型
	add_library(mars-face-kit STATIC IMPORTED)
	# 设置目标库的实际路径
	set_target_properties(mars-face-kit PROPERTIES IMPORTED_LOCATION
	${CMAKE_CURRENT_SOURCE_DIR}/third_party/mars-face-kit/libs/${CURRENT_OS}/libmars-face-kit.a)

ELSEIF(${CURRENT_OS} STREQUAL "macos" OR ${CURRENT_OS} STREQUAL "ios")
    LINK_DIRECTORIES(
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mnn/libs/${CURRENT_OS}
    )
	set_target_properties(${PROJECT_NAME} PROPERTIES
		XCODE_ATTRIBUTE_PRODUCT_NAME ${PROJECT_NAME}
		COMPILE_FLAGS "-x objective-c++"
		FRAMEWORK TRUE
		MACOSX_FRAMEWORK_IDENTIFIER net.pixpark.${PROJECT_NAME}
		PRODUCT_BUNDLE_IDENTIFIER net.pixpark.${PROJECT_NAME}
		CMAKE_XCODE_ATTRIBUTE_BUILT_PRODUCTS_DIR ${PROJECT_NAME}
		MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
		FRAMEWORK_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/
		PUBLIC_HEADER "${EXPORT_HEADER}"
		RESOURCE "${RESOURCE_FILES}"
	)
 
ELSEIF(${CURRENT_OS} STREQUAL "android")
    add_library(mars-face-kit STATIC IMPORTED)
    # 设置目标库的实际路径
    set_target_properties(mars-face-kit PROPERTIES IMPORTED_LOCATION
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mars-face-kit/libs/${CURRENT_OS}/${ANDROID_ABI}/libmars-face-kit.a)
	add_library(MNN SHARED IMPORTED)
	# 设置目标库的实际路径
	set_target_properties(MNN PROPERTIES IMPORTED_LOCATION
	${CMAKE_CURRENT_SOURCE_DIR}/third_party/mnn/libs/${CURRENT_OS}/${ANDROID_ABI}/libMNN.so)
 
	add_library(MNN_Express SHARED IMPORTED)
	# 设置目标库的实际路径
	set_target_properties(MNN_Express PROPERTIES IMPORTED_LOCATION
	${CMAKE_CURRENT_SOURCE_DIR}/third_party/mnn/libs/${CURRENT_OS}/${ANDROID_ABI}/libMNN_Express.so)
	
ELSEIF(${CURRENT_OS} STREQUAL "wasm")
	set_target_properties(${PROJECT_NAME} PROPERTIES 
						SUFFIX ".wasm"
    					LINK_FLAGS "-Os -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_GLFW=3  -s WASM=1")
ENDIF()
 
# link libs
# -------
IF(${CURRENT_OS} STREQUAL "linux" OR ${CURRENT_OS} STREQUAL "wasm")
	TARGET_LINK_LIBRARIES(
						${PROJECT_NAME}  
						PRIVATE
						GL
						glfw
						mars-face-kit
						MNN)

ELSEIF(${CURRENT_OS} STREQUAL "windows")
	TARGET_LINK_LIBRARIES(
						${PROJECT_NAME} 
						opengl32
						glfw3
						mars-face-kit)
ELSEIF(${CURRENT_OS} STREQUAL "macos")
	TARGET_LINK_LIBRARIES(
		${PROJECT_NAME} "-framework OpenGL"
						"-framework AppKit"
						"-framework QuartzCore"
						"-framework CoreVideo"
						"-framework CoreGraphics"
						"-framework AVFoundation"
						"-framework CoreMedia"
                        "-framework Metal"
                        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mnn/libs/${CURRENT_OS}/MNN.framework"
                        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mars-face-kit/libs/${CURRENT_OS}/libmars-face-kit.a"
	)
ELSEIF(${CURRENT_OS} STREQUAL "ios")
	TARGET_LINK_LIBRARIES(
	${PROJECT_NAME} "-framework OpenGLES" 	
					"-framework UIKit" 	
					"-framework QuartzCore"  
					"-framework CoreVideo"  
					"-framework CoreGraphics"
					"-framework AVFoundation"
					"-framework CoreMedia"
					"-framework Metal"
					"-framework CoreML"
                    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mnn/libs/${CURRENT_OS}/MNN.framework"
                    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mars-face-kit/libs/${CURRENT_OS}/libmars-face-kit.a"
	)
ELSEIF(${CURRENT_OS} STREQUAL "android")
	TARGET_LINK_LIBRARIES(
					${PROJECT_NAME}  
					log
					android
					GLESv3
					EGL
					jnigraphics
					MNN
					MNN_Express
					atomic
					mars-face-kit)
ENDIF()

MACRO(EXPORT_INCLUDE)
	# copy header
	ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} PRE_BUILD 
				COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_RESOURCE_PATH}
				COMMAND ${CMAKE_COMMAND} -E copy 
				${RESOURCE_FILES} ${OUTPUT_RESOURCE_PATH}
				COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}
				COMMAND ${CMAKE_COMMAND} -E copy 
				${EXPORT_HEADER} ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}
				COMMENT "Copying headers and resource to output directory.")
ENDMACRO()

EXPORT_INCLUDE()
