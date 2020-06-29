set(CMAKE_AUTORCC ON)

# compile in debug mode
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)
 
#message($ENV{QT5_DIR})
if($ENV{QT5_DIR})
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "$ENV{QT5_DIR}/lib/cmake")
endif()

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(VTK)
IF(VTK_FOUND)
  INCLUDE(${VTK_USE_FILE})
  INCLUDE_DIRECTORIES(${VTK_INCLUDE_DIR})
ELSE(VTK_FOUND)
  MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
#  FIND_PATH(VTK_DIR VTKConfig.cmake ~/Personal_Libraries/VTK-build-5.8/)
#  INCLUDE(${VTK_USE_FILE})
ENDIF(VTK_FOUND)

FIND_PACKAGE(BOOST)

OPTION(QT5 "QT5" ON)

if(QT5)
  MESSAGE("Using Qt5")
  FIND_PACKAGE(Qt5Widgets REQUIRED)
  FIND_PACKAGE(Qt5Gui REQUIRED)
#  FIND_PACKAGE(Qt5PrintSupport REQUIRED)
else()
  MESSAGE("Using Qt4")
  FIND_PACKAGE(Qt4 REQUIRED)

  SET(QT_USE_QTOPENGL ON)
  SET(QT_USE_QTWEBKIT OFF)

  INCLUDE(${QT_USE_FILE})
  ADD_DEFINITIONS(${QT_DEFINITIONS})
endif()

SET(QCUSTOMPLOT_SOURCE_DIR CACHE PATH "Path to the QCustomPlot source files.")
OPTION(QCUSTOMPLOT "Activate for using QCustomPlot. Beware GPL version is not compatible with VITA LGPL, so please use a commercial version or contact the developer")
IF(QCUSTOMPLOT)
  ADD_DEFINITIONS(-DUSE_QCUSTOMPLOT)
  FILE(GLOB QCUSTOMPLOT_SRC_FILES ${QCUSTOMPLOT_SOURCE_DIR}/*.cpp)
ENDIF(QCUSTOMPLOT)

OPTION(JET_INSTALL "Activate for JAC or FREIA installations")
SET(JET_INSTALL FALSE)
IF(JET_INSTALL)
  ADD_DEFINITIONS(-DJET_INSTALL)
  INCLUDE_DIRECTORIES(${PPF_INCLUDE})
  FIND_LIBRARY(ppf_lib NAMES libppf.so libppf.so.1 libppf PATHS ${PPF_DIR})
 
  INCLUDE_DIRECTORIES(${FLUSH_INCLUDE})
  FIND_LIBRARY(flush_lib NAMES libflush.so libflush.so.1 libflush.a PATHS ${FLUSH_DIR})
  LINK_DIRECTORIES(${FLUSH_DIR})
ENDIF(JET_INSTALL)

FIND_PATH(MKNIX_SOURCE_DIR HINTS ${PROJECT_SOURCE_DIR}/../MkniX)
FIND_PATH(MKNIX_LINK_DIR HINTS ${PROJECT_SOURCE_DIR}/../MkniX-build/src  NAMES libthermiXlib.a MkniXlib.a MkniXlib.dll)

# Make sure the compiler can find include files from our program.
include_directories (
${CMAKE_SOURCE_DIR}/src 
${MKNIX_SOURCE_DIR}/src/
${MKNIX_SOURCE_DIR}/src/core
${MKNIX_SOURCE_DIR}/src/reader
${MKNIX_SOURCE_DIR}/src/simulation
${MKNIX_SOURCE_DIR}/src/system
${MKNIX_SOURCE_DIR}/LMX-1.0/src/
${VTK_INCLUDE_DIR}
${Boost_INCLUDE_DIR}
${QCUSTOMPLOT_SOURCE_DIR}
)
 
SET(VITASrcs 
  main.cpp 
  compnode.cpp
  compmesh2D.cpp
  compmesh3D.cpp
  compmeshfree.cpp
  compmeshfree3D.cpp
  compradmap.cpp
  graphwidget.cpp
  graphics.cpp
  model.cpp
  modeltile6_523.cpp
  modeltile6_104.cpp
  modeltile6_109.cpp
  modelexternalmesh.cpp
  plot2D.cpp
  vitamainwindow.cpp
  vtkinterface.cpp
)
 
SET(VITA_HEADERS 
  vitamainwindow.h
  graphwidget.h
)
SET(VITA_RESOURCES
  application.qrc
)

IF(QCUSTOMPLOT)
  SET(VITA_HEADERS ${VITA_HEADERS} ${QCUSTOMPLOT_SOURCE_DIR}/qcustomplot.h ) 
ENDIF(USE_QCUSTOMPLOT)


if(QT5)
  QT5_WRAP_CPP(VITA_HEADERS_MOC ${VITA_HEADERS})
  QT5_ADD_RESOURCES(VITA_RESOURCES_RCC ${VITA_RESOURCES})
else()
  QT4_WRAP_CPP(VITA_HEADERS_MOC ${VITA_HEADERS})
  QT4_ADD_RESOURCES(VITA_RESOURCES_RCC ${VITA_RESOURCES})
endif()

 
ADD_EXECUTABLE(vita
  ${VITASrcs}
  ${VITA_HEADERS_MOC}
  ${VITA_RESOURCES_RCC}
  ${QCUSTOMPLOT_SRC_FILES}
)

if(QT5)
  QT5_USE_MODULES(vita Widgets Gui PrintSupport)
endif()

INSTALL( TARGETS vita DESTINATION bin )
 
link_directories ( 
  ${VTK_DIR}
  ${MKNIX_LINK_DIR}
)
ADD_LIBRARY(mknix_library STATIC IMPORTED)
ADD_LIBRARY(mknix_library_debug STATIC IMPORTED)
IF(WIN32)
	SET_TARGET_PROPERTIES(mknix_library PROPERTIES
		IMPORTED_LOCATION ${MKNIX_LINK_DIR}/Release/mknixlib.lib)
	SET_TARGET_PROPERTIES(mknix_library_debug PROPERTIES
		IMPORTED_LOCATION ${MKNIX_LINK_DIR}/Debug/mknixlib.lib)
ELSE()
	SET_TARGET_PROPERTIES(mknix_library PROPERTIES
		IMPORTED_LOCATION ${MKNIX_LINK_DIR}/libmknix_static.a)
	SET_TARGET_PROPERTIES(mknix_library_debug PROPERTIES
		IMPORTED_LOCATION ${MKNIX_LINK_DIR}/libmknix_static.a)
ENDIF(WIN32)
 
message("libMkniXlib is at: ${MKNIX_LINK_DIR}")

# Link the executable to the MkniX, Qt, and VTK libraries.
TARGET_LINK_LIBRARIES ( vita optimized
  mknix_library
  ${VTK_LIBRARIES}
)
TARGET_LINK_LIBRARIES ( vita debug
  mknix_library_debug
  ${VTK_LIBRARIES}
)
TARGET_LINK_LIBRARIES(vita
  ${VTK_LIBRARIES}
  ${QT_LIBRARIES}
  #QVTK
)

IF(JET_INSTALL)
  TARGET_LINK_LIBRARIES ( vita
    netcsl7
    ${ppf_lib}
    ${flush_lib}
  )
ENDIF(JET_INSTALL)
 
