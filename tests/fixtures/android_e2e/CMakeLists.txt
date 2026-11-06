cmake_minimum_required(VERSION 3.10)
project(sentry_android_e2e LANGUAGES C)

add_library(e2e SHARED crash.c)
