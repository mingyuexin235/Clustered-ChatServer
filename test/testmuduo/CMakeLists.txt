cmake_minimum_required(VERSION 3.0)
project(main)

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)  # 可调试可执行文件

#配置头文件搜索路径
# include_directories()

#配置库文件搜索路径
# link_directories()

# 设置需要编译的源文件列表
set(SRC_LIST ./muduo_server.cpp)
# 设置可执行文件存储的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)


# 把.指定路径下的所有源文件放入SRC_LIST变量名中
# aux_source_directory(.SRC_LIST)

# 表示生成可执行文件server，由SRC_LIST变量定义的源文件而来
add_executable(server ${SRC_LIST})  # 生成可执行文件

#表示server这个目标程序，需要链接muduo_net muduo_base pthread库文件
target_link_libraries(server muduo_net muduo_base pthread)