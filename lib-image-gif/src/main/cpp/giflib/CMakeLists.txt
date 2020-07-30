# Sets the minimum version of CMake required to build the native library.
CMAKE_MINIMUM_REQUIRED(VERSION 3.4.1)

# 对文件集合取变量
FILE(
        GLOB GIF_SRC_LIST
        "${PROJECT_SOURCE_DIR}/src/main/cpp/giflib/*.c"
)

# 添加要打包的资源
ADD_LIBRARY(
        # 打包后的库名称
        giflib
        # 库类型
        SHARED
        # 打包的文件
        ${GIF_SRC_LIST}
)