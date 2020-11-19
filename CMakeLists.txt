cmake_minimum_required(VERSION 2.8.12)

PROJECT(ChromaKeying)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG_MODE")

#SET(OpenCV_DIR /home/hp/workfolder/OpenCV-Installation/installation/OpenCV-master/lib/cmake/opencv4)

######################## EDIT IF REQUIRED ####################
# ###Uncomment the line below and specify the path to OpenCV directory i.e. the path to the OpenCVConfig.cmake file. Check the examples given below.
#SET(OpenCV_DIR Enter-the-path-of-OpenCV-installation-on-your-system)
#set(OpenCV_DIR /opt/opencv/4.4.0/installation/lib/cmake/opencv4)

################### OpenCV_DIR Examples  #####################

### MACOS : /usr/local/Cellar/opencv/3.3.1_1/share/OpenCV/

### UBUNTU : /usr/local/share/OpenCV/

### WINDOWS : C:\Users\yourname\Documents\opencv-3.3.1\build\install

##############################################################




###################   ***DO NOT EDIT***  #####################

#############  Common Instructions for all Users  ############

find_package( OpenCV REQUIRED )

include_directories( ${OpenCV_INCLUDE_DIRS})


ADD_EXECUTABLE(chromak main.cpp mediasource.h imagefilereader.h imagefilereader.cpp videofilereader.h videofilereader.cpp mediasink.h imagefilewriter.h imagefilewriter.cpp videofilewriter.h videofilewriter.cpp mediafactory.h mediafactory.cpp chromakeyer.h chromakeyer.cpp)
TARGET_LINK_LIBRARIES(chromak ${OpenCV_LIBS} )

if(MSVC)
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	SET_PROPERTY(TARGET chromak PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()
