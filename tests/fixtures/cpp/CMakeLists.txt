cmake_minimum_required(VERSION 3.10)
project(sentry_test_cpp LANGUAGES CXX)

add_executable(sentry_test_cpp main.cpp)
target_link_libraries(sentry_test_cpp PRIVATE sentry)
