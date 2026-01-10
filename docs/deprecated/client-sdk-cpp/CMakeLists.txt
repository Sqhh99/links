cmake_minimum_required(VERSION 3.31.0)
project(livekit LANGUAGES C CXX)

# ==========================================
# [修复 1] 解决 LNK2038 运行库冲突
# 强制 C++ 项目使用静态运行时 (/MT) 以匹配 Rust 的默认行为
# ==========================================
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  # 解决中文编码警告
  add_compile_options(/utf-8)
endif()

# ---- C++ standard ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set PROTOC for Rust's prost-build (vcpkg path)
set(ENV{PROTOC} "D:/vcpkg/installed/x64-windows-static/tools/protobuf/protoc.exe")

# Set LIBCLANG_PATH for bindgen (used by yuv-sys)
set(LIBCLANG_PATH "D:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/x64/bin")
set(ENV{LIBCLANG_PATH} "${LIBCLANG_PATH}")

# ---- Protobuf (FFI protos) ----
set(FFI_PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/client-sdk-rust/livekit-ffi/protocol)
set(FFI_PROTO_FILES
    ${FFI_PROTO_DIR}/handle.proto
    ${FFI_PROTO_DIR}/ffi.proto
    ${FFI_PROTO_DIR}/participant.proto
    ${FFI_PROTO_DIR}/room.proto
    ${FFI_PROTO_DIR}/track.proto
    ${FFI_PROTO_DIR}/video_frame.proto
    ${FFI_PROTO_DIR}/audio_frame.proto
    ${FFI_PROTO_DIR}/e2ee.proto
    ${FFI_PROTO_DIR}/stats.proto
    ${FFI_PROTO_DIR}/data_stream.proto
    ${FFI_PROTO_DIR}/rpc.proto
    ${FFI_PROTO_DIR}/track_publication.proto
)
set(PROTO_BINARY_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTO_BINARY_DIR})

find_package(protobuf CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)

# Object library that owns generated .pb.cc/.pb.h
add_library(livekit_proto OBJECT ${FFI_PROTO_FILES})
target_include_directories(livekit_proto PRIVATE
  "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>"
  ${Protobuf_INCLUDE_DIRS}
)
target_link_libraries(livekit_proto PRIVATE protobuf::libprotobuf)

protobuf_generate(
  LANGUAGE cpp
  TARGET livekit_proto
  PROTOC_OUT_DIR ${PROTO_BINARY_DIR}
  IMPORT_DIRS ${FFI_PROTO_DIR}
)

# Find cargo
find_program(CARGO_EXECUTABLE NAMES cargo REQUIRED)
set(RUST_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/client-sdk-rust)

# Helper script for cargo
set(RUN_CARGO_SCRIPT ${CMAKE_BINARY_DIR}/run_cargo.cmake)
file(WRITE ${RUN_CARGO_SCRIPT}
"if(NOT DEFINED CFG)
  set(CFG Debug)
endif()
if(NOT DEFINED RUST_ROOT)
  message(FATAL_ERROR \"RUST_ROOT not set\")
endif()
if(NOT DEFINED CARGO)
  message(FATAL_ERROR \"CARGO not set\")
endif()
# Set PROTOC for Rust's prost-build (vcpkg path)
set(ENV{PROTOC} \"D:/vcpkg/installed/x64-windows-static/tools/protobuf/protoc.exe\")
set(ARGS build)
if(NOT CFG STREQUAL \"Debug\")
  list(APPEND ARGS --release)
endif()
message(STATUS \"[run_cargo.cmake] CFG=\${CFG}  CARGO=\${CARGO}\")
execute_process(
  COMMAND \"\${CARGO}\" \${ARGS}
  WORKING_DIRECTORY \"\${RUST_ROOT}\"
  RESULT_VARIABLE rv
)
if(rv)
  message(FATAL_ERROR \"cargo build failed with code: \${rv}\")
endif()
")

# 自动判断库文件名
if(MSVC)
    set(RUST_LIB_NAME "livekit_ffi.lib")
else()
    set(RUST_LIB_NAME "liblivekit_ffi.a")
endif()

add_library(livekit_ffi STATIC IMPORTED GLOBAL)
set_target_properties(livekit_ffi PROPERTIES
  IMPORTED_LOCATION_DEBUG          "${RUST_ROOT}/target/debug/${RUST_LIB_NAME}"
  IMPORTED_LOCATION_RELWITHDEBINFO "${RUST_ROOT}/target/release/${RUST_LIB_NAME}"
  IMPORTED_LOCATION_MINSIZEREL     "${RUST_ROOT}/target/release/${RUST_LIB_NAME}"
  IMPORTED_LOCATION_RELEASE        "${RUST_ROOT}/target/release/${RUST_LIB_NAME}"
  INTERFACE_INCLUDE_DIRECTORIES    "${RUST_ROOT}/livekit-ffi/include"
)

add_custom_target(build_rust_ffi ALL
  COMMAND "${CMAKE_COMMAND}"
          -DCFG=$<CONFIG>
          -DRUST_ROOT=${RUST_ROOT}
          -DCARGO=${CARGO_EXECUTABLE}
          -P "${RUN_CARGO_SCRIPT}"
  USES_TERMINAL
  COMMENT "Invoking cargo for Rust FFI ($<CONFIG>)"
  VERBATIM
)

# ---- C++ wrapper library ----
# C++ SDK sources
set(LIVEKIT_SOURCES
    src/ffi_client.cpp
    src/room.cpp
    src/participant.cpp
    src/track.cpp
    src/video_source.cpp
    src/audio_source.cpp
)

add_library(livekit
  include/livekit/room.h
  include/livekit/ffi_client.h
  include/livekit/livekit.h
  include/livekit/participant.h
  include/livekit/track.h
  include/livekit/video_source.h
  include/livekit/audio_source.h
  ${LIVEKIT_SOURCES}
)

target_sources(livekit PRIVATE $<TARGET_OBJECTS:livekit_proto>)

target_include_directories(livekit PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${RUST_ROOT}/livekit-ffi/include
  ${PROTO_BINARY_DIR}
)

target_link_libraries(livekit
  PUBLIC
    livekit_ffi
    protobuf::libprotobuf
)
message(STATUS "Protobuf: version=${Protobuf_VERSION}")
add_dependencies(livekit build_rust_ffi)

# Platform specific settings
if(APPLE)
  # (Mac settings omitted for brevity, keep your original if needed)
endif()

# Protobuf/Abseil linking
if (Protobuf_VERSION VERSION_GREATER_EQUAL 6.0)
  if (absl_FOUND)
    target_link_libraries(livekit PUBLIC absl::log absl::check absl::strings absl::base)
  elseif (Abseil_FOUND)
    target_link_libraries(livekit PUBLIC Abseil::log Abseil::check Abseil::strings Abseil::base)
  endif()
endif()

if(UNIX AND NOT APPLE)
  find_package(OpenSSL REQUIRED)
  target_link_libraries(livekit PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()

# ==========================================
# [修复 2] 补全所有缺失的 Windows 系统库
# ==========================================
if(WIN32)
  target_link_libraries(livekit PUBLIC 
      ws2_32            # 基础网络
      bcrypt            # 加密
      userenv           # 用户环境
      ntdll             # 系统底层
      secur32           # 安全接口
      crypt32           # 证书/加密
      winmm             # 多媒体定时器 (解决 timeGetTime)
      iphlpapi          # 网络适配器 (解决 GetAdaptersAddresses)
      dmoguids          # DirectX Media Objects (解决 CLSID_CWMAudioAEC)
      wmcodecdspuuid    # Windows Media Codecs
      msdmo             # Media Objects 运行时
      strmiids          # Streaming Media Interface IDs
  )
endif()

# Warnings
if (MSVC)
  target_compile_options(livekit PRIVATE /permissive- /Zc:__cplusplus /W4)
else()
  target_compile_options(livekit PRIVATE -Wall -Wextra -Wpedantic)
endif()



# Clean helpers (optional, keeping your original logic)
add_custom_target(clean_generated COMMAND ${CMAKE_COMMAND} -E rm -rf "${PROTO_BINARY_DIR}")