cmake_minimum_required(VERSION 3.10)
project(sentry_early_init LANGUAGES CXX)

add_executable(sentry_early_init early_init.cpp)
target_link_libraries(sentry_early_init PRIVATE sentry)
