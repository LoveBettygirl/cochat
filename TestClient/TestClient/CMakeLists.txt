file(GLOB_RECURSE SRC_LIST *.cpp *.cc *.h *.S)
add_executable(TestClient ${SRC_LIST})
target_link_libraries(TestClient corpc pthread yaml-cpp protobuf dl zookeeper_mt ssl crypto)