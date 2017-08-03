#---
# File:  FindOpenCV.cmake
# 
# Locate OPENCV
#
# This module defines:
#
# OPENCV_INCLUDE_DIR
#
# OPENCV_FOUND, 
# OPENCV_CORE_FOUND 
# OPENCV_HIGHGUI_FOUND
# OPENCV_IMGPROC_FOUND
# OPENCV_LEGACY_FOUND
# OPENCV_ML_FOUND 
#
# OPENCV_CORE_LIBRARY
# OPENCV_HIGHGUI_LIBRARY
# OPENCV_IMGPROC_LIBRARY
# OPENCV_LEGACY_LIBRARY
# OPENCV_ML_LIBRARY
# OPENCV_LIBRARIES
#
# Created by Garrett Potts.
#
# $Id$

# Find include path:
find_path(OPENCV_INCLUDE_DIR opencv/cv.hpp PATHS /usr/include /usr/local/include)

macro(FIND_OPENCV_LIBRARY MYLIBRARY MYLIBRARYNAME)

   find_library( ${MYLIBRARY}
      NAMES "${MYLIBRARYNAME}${OPENCV_RELEASE_POSTFIX}"
      PATHS
      /usr/lib64
      /usr/lib
     /usr/local/lib
   )

endmacro(FIND_OPENCV_LIBRARY MYLIBRARY MYLIBRARYNAME)

# Required
FIND_OPENCV_LIBRARY(OPENCV_CORE_LIBRARY opencv_core)
FIND_OPENCV_LIBRARY(OPENCV_FEATURES2D_LIBRARY opencv_features2d)
FIND_OPENCV_LIBRARY(OPENCV_XFEATURES2D_LIBRARY opencv_xfeatures2d)
FIND_OPENCV_LIBRARY(OPENCV_FLANN_LIBRARY opencv_flann)
FIND_OPENCV_LIBRARY(OPENCV_HIGHGUI_LIBRARY opencv_highgui)
FIND_OPENCV_LIBRARY(OPENCV_IMGPROC_LIBRARY opencv_imgproc)
FIND_OPENCV_LIBRARY(OPENCV_ML_LIBRARY opencv_ml)

# Optional
FIND_OPENCV_LIBRARY(OPENCV_CALIB3D_LIBRARY opencv_calib3d)
FIND_OPENCV_LIBRARY(OPENCV_IMGCODECS_LIBRARY opencv_imgcodecs)
FIND_OPENCV_LIBRARY(OPENCV_CONTRIB_LIBRARY opencv_contrib)
FIND_OPENCV_LIBRARY(OPENCV_GPU_LIBRARY opencv_gpu)

set(OPENCV_FOUND "NO")
if(OPENCV_INCLUDE_DIR AND OPENCV_CORE_LIBRARY AND OPENCV_FEATURES2D_LIBRARY AND OPENCV_XFEATURES2D_LIBRARY AND OPENCV_FLANN_LIBRARY AND OPENCV_HIGHGUI_LIBRARY AND OPENCV_IMGPROC_LIBRARY AND OPENCV_ML_LIBRARY)
   set(OPENCV_FOUND "YES")
   set(OPENCV_LIBRARIES  ${OPENCV_CORE_LIBRARY} ${OPENCV_FEATURES2D_LIBRARY} ${OPENCV_XFEATURES2D_LIBRARY} ${OPENCV_FLANN_LIBRARY} ${OPENCV_HIGHGUI_LIBRARY} ${OPENCV_IMGPROC_LIBRARY} ${OPENCV_ML_LIBRARY})
else()
   message( WARNING "Could not find OPENCV" )
endif()

set(OPENCV_CONTRIB_FOUND "NO")
if(OPENCV_FOUND AND OPENCV_CONTRIB_LIBRARY)
   set(OPENCV_CONTRIB_FOUND "YES")
   set(OPENCV_LIBRARIES ${OPENCV_LIBRARIES} ${OPENCV_CONTRIB_LIBRARY})
else()
   message( "Could not find optional OPENCV_CONTRIB library. " )
endif()

set(OPENCV_GPU_FOUND "NO")
if(OPENCV_FOUND AND OPENCV_GPU_LIBRARY)
   set(OPENCV_GPU_FOUND "YES")
   set(OPENCV_LIBRARIES ${OPENCV_LIBRARIES} ${OPENCV_GPU_LIBRARY})
else()
   message( "Could not find optional OPENCV GPU Library. " )
endif()

set(OPENCV_IMGCODECS_FOUND "NO")
if(OPENCV_FOUND AND OPENCV_IMGCODECS_LIBRARY)
   set(OPENCV_IMGCODECS_FOUND "YES")
   set(OPENCV_LIBRARIES ${OPENCV_LIBRARIES} ${OPENCV_IMGCODECS_LIBRARY})
else()
   message( "Could not find optional OPENCV Image Codecs Library" )
endif()

set(OPENCV_CALIB3D_FOUND "NO")
if(OPENCV_FOUND AND OPENCV_CALIB3D_LIBRARY)
   set(OPENCV_CALIB3D_FOUND "YES")
   set(OPENCV_LIBRARIES ${OPENCV_LIBRARIES} ${OPENCV_CALIB3D_LIBRARY})
else()
   message( "Could not find optional OPENCV Calib 3D Library" )
endif()

if(OPENCV_FOUND)
   message( STATUS "OPENCV_INCLUDE_DIR = ${OPENCV_INCLUDE_DIR}" )
   message( STATUS "OPENCV_LIBRARIES   = ${OPENCV_LIBRARIES}" )
endif()

