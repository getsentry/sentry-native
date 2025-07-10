cmake_minimum_required(VERSION 3.10)
project(sentry_feedback LANGUAGES C)

add_executable(sentry_feedback feedback.c)
target_link_libraries(sentry_feedback PRIVATE sentry)
