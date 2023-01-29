file(GLOB_RECURSE SRC_LIST *.cpp *.cc *.h *.S)
add_executable(ProxyService ${SRC_LIST})
target_link_libraries(ProxyService corpc pthread yaml-cpp protobuf dl zookeeper_mt mysqlclient hiredis rocketmq z rt ssl)