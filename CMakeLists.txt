# CMake最低版本要求（兼容99%的Linux系统）
cmake_minimum_required(VERSION 3.10)

# 1. 设置项目名称（可自定义，比如改成MyCProject），指定语言为C
project(MyCProject C)

# 2. 设置C语言编译标准（C11，兼容所有C语法，Linux默认支持）
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 3. 自动扫描当前目录下 ALL .c 源文件（无需手动添加文件名，新增文件自动识别）
aux_source_directory(. SRC_LIST)

# 4. 指定生成C语言可执行程序（程序名可自定义，比如改成main/run）
# 格式：add_executable(程序名 所有源文件列表)
add_executable(mycapp ${SRC_LIST})
