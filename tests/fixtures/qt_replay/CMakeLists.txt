cmake_minimum_required(VERSION 3.10)
project(sentry_qt_replay LANGUAGES CXX)

add_executable(sentry_qt_replay qt_replay.cpp)
target_link_libraries(sentry_qt_replay PRIVATE sentry Qt${Qt_VERSION_MAJOR}::Gui)
