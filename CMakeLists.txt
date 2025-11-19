# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 4.0)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# set(PROJECT_VER "0.0.0")


set(COMPONENTS main esp_eth)
project(firmware-idf)
