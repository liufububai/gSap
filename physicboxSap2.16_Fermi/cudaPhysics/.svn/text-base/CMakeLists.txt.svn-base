
# Add current directory to the nvcc include line.
CUDA_INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
)

##############################################################################
# Link cuda code in a library to something else.
INCLUDE_DIRECTORIES(/usr/local/cuda/include)
INCLUDE_DIRECTORIES(../include)
INCLUDE_DIRECTORIES(../src)
LINK_DIRECTORIES(../lib)

CUDA_ADD_EXECUTABLE(simpleTest
#	simpleTest.cpp
	connectorTest.cpp
)
TARGET_LINK_LIBRARIES(simpleTest
	cutil
	glut
	cudaDem
	slicedGrid
#	uniformGrid
	cudaDistanceFunction
	connectorOfParticles
)


