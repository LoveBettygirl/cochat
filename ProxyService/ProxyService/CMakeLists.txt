file(GLOB_RECURSE SRC_LIST *.cpp *.cc *.h *.hpp *.S)
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} corpc pthread yaml-cpp protobuf dl zookeeper_mt mysqlclient hiredis ssl crypto)
