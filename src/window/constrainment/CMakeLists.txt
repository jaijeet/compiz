pkg_check_modules (
  GLIBMM
  REQUIRED
  glibmm-2.4 glib-2.0
)

INCLUDE_DIRECTORIES (  
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  
  ${compiz_SOURCE_DIR}/include

  ${compiz_SOURCE_DIR}/src/point/include
  ${compiz_SOURCE_DIR}/src/rect/include
  ${compiz_SOURCE_DIR}/src/window/geometry/include
  ${compiz_SOURCE_DIR}/src/window/geometry-saver/include  
  ${Boost_INCLUDE_DIRS}
  
  ${GLIBMM_INCLUDE_DIRS}
)

LINK_DIRECTORIES (${GLIBMM_LIBRARY_DIRS}) 

SET ( 
  PUBLIC_HEADERS 
  ${CMAKE_CURRENT_SOURCE_DIR}/include/core/windowconstrainment.h
)

SET ( 
  PRIVATE_HEADERS 
)

SET( 
  SRCS 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/windowconstrainment.cpp
)

ADD_LIBRARY( 
  compiz_window_constrainment STATIC
  
  ${SRCS}
  
  ${PUBLIC_HEADERS}
  ${PRIVATE_HEADERS}
)

ADD_SUBDIRECTORY( ${CMAKE_CURRENT_SOURCE_DIR}/tests )

SET_TARGET_PROPERTIES(
  compiz_window_constrainment PROPERTIES
  PUBLIC_HEADER "${PUBLIC_HEADERS}"
  COMPILE_FLAGS "-fPIC"
)

INSTALL(
  TARGETS                                 compiz_window_constrainment
  RUNTIME             DESTINATION         bin
  LIBRARY             DESTINATION         lib
  ARCHIVE             DESTINATION         lib
  PUBLIC_HEADER       DESTINATION         include/compiz/core
)



TARGET_LINK_LIBRARIES( 
  compiz_window_constrainment
  compiz_window_geometry
  compiz_point
  compiz_rect
  
  ${GLIBMM_LIBRARIES}
)