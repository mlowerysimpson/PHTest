cmake_minimum_required(VERSION 2.8)
project(PHTest)
# Locate libraries and headers
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")


find_package(Threads REQUIRED)
find_library(wiringPi_LIB wiringPi)
find_package(CURL REQUIRED)
find_package( OpenCV REQUIRED )
find_package(libgps REQUIRED)

# Include headers
include_directories(${WIRINGPI_INCLUDE_DIRS})
include_directories(${LIBGPS_INCLUDE_DIR})
include_directories(${CURL_INCLUDE_DIR})
include_directories("../RemoteControlTest")

#the file(GLOB...) allows for wildcard additions:
file(GLOB SOURCES "./*.cpp")
file(GLOB remoteSources "../RemoteControlTest/*.cpp")
add_executable(PHTest ${SOURCES} ${remoteSources})

# Link against libraries
target_link_libraries(PHTest ${wiringPi_LIB} )
target_link_libraries(PHTest ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(PHTest ${LIBGPS_LIBRARIES} )
target_link_libraries(PHTest ${CURL_LIBRARIES})
target_link_libraries(PHTest ${OpenCV_LIBS} )



