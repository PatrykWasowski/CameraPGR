# Include the directory itself as a path to include directories
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

# Create a variable containing all .cpp files:
FILE(GLOB files *.cpp)

# Find required packages
FIND_PACKAGE( OpenCV REQUIRED )


LINK_DIRECTORIES(/usr/lib )
include_directories(/usr/include/flycapture)
# Create an executable file from sources:
ADD_LIBRARY(CameraPGR SHARED ${files})

# Link external libraries
TARGET_LINK_LIBRARIES(CameraPGR ${DisCODe_LIBRARIES} 
	${OpenCV_LIBS}
	libflycapture.so)

INSTALL_COMPONENT(CameraPGR)
