# Sets the minimum version of CMake required to build the native library.
CMAKE_MINIMUM_REQUIRED(VERSION 3.4.1)

# 执行 src 目录下的 CMakeLists.txt
ADD_SUBDIRECTORY(src/main/cpp/giflib)

# 对文件集合取变量
FILE(
        GLOB
        SRC_LIST
        "${PROJECT_SOURCE_DIR}/src/main/cpp/*.cpp"
        "${PROJECT_SOURCE_DIR}/src/main/cpp/stream/*.cpp"
)

# 添加要打包的资源
ADD_LIBRARY(
        # 打包后的库名称
        giftool
        # 库类型
        SHARED
        # 打包的文件
        ${SRC_LIST}
)

# 为 gifkit 添加外部依赖
TARGET_LINK_LIBRARIES(
        # Specifies the target library.
        giftool
        # 外部链接库
        giflib
        # Anroid libs

        jnigraphics
        log
)