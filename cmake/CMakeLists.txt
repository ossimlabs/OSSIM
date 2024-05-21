PROJECT(ossimlabs)

# Allows developer to customize compiler with their own flags (e.g., "-std=c++11")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{OSSIM_CXX_FLAGS}" )

IF ("${OSSIM_DEV_HOME}"  STREQUAL "")
  IF ("$ENV{OSSIM_DEV_HOME}"  STREQUAL "")
    SET(OSSIM_DEV_HOME "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE STRING "Cache the OSSIM dev home" FORCE)
  ELSE()
    SET(OSSIM_DEV_HOME "$ENV{OSSIM_DEV_HOME}" CACHE STRING "Cache the OSSIM dev home" FORCE)
  ENDIF()
ENDIF()

if(EXISTS ${CMAKE_TOOLCHAIN_FILE})
   MESSAGE("Using toolchain file ${CMAKE_TOOLCHAIN_FILE}")
ENDIF()


if(NOT EXISTS ${OSSIM_DEV_HOME})
   MESSAGE(FATAL_ERROR "Please pass in or define a valid OSSIM_DEV_HOME root path where all modules are located. This includes ossim, ossim-planet, ... etc.")
ENDIF()

IF ("$ENV{OSSIM_INSTALL_PREFIX}"  STREQUAL "")
   IF (EXISTS "$ENV{OSSIM_BUILD_DIR}")
      SET(OSSIM_INSTALL_PREFIX "$ENV{OSSIM_BUILD_DIR}" CACHE STRING "Cache the OSSIM install prefix" FORCE)
   ELSE()
      SET(OSSIM_INSTALL_PREFIX "${OSSIM_DEV_HOME}/install" CACHE STRING "Cache the OSSIM install prefix" FORCE)
   ENDIF()
ELSE()
  SET(OSSIM_INSTALL_PREFIX "$ENV{OSSIM_INSTALL_PREFIX}" CACHE STRING "Cache the OSSIM install prefix" FORCE)
ENDIF()

IF(NOT OSSIM_BUILD_ADDITIONAL_DIRECTORIES)
   SET(OSSIM_BUILD_ADDITIONAL_DIRECTORIES "" CACHE PATH "Specify full paths separated by ; to additional libraries you wish to include into the ossim build framework" FORCE)
ENDIF(NOT OSSIM_BUILD_ADDITIONAL_DIRECTORIES)

SET(CMAKE_MODULE_PATH "${OSSIM_DEV_HOME}/ossim/cmake/CMakeModules")
INCLUDE(OssimVersion)
INCLUDE(OssimUtilities)
INCLUDE(OssimCommonVariables)

FILE(REMOVE ${CMAKE_INCLUDE_DIRS_FILE})
FILE(REMOVE ${CMAKE_FILENAMES_FILE})

IF(NOT APPLE)
  cmake_minimum_required(VERSION 2.6)
ELSE(NOT APPLE)
    cmake_minimum_required(VERSION 2.8)
ENDIF(NOT APPLE)

IF(EXISTS "$ENV{OSSIM_DEPENDENCIES}")
   SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${OSSIM_DEPENDENCIES}/include )
   SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${OSSIM_DEPENDENCIES}/lib${LIBSUFFIX})
   SET(CMAKE_FRAMEWORK_PATH "${CMAKE_FRAMEWORK_PATH};$ENV{OSSIM_DEPENDENCIES}/Frameworks")
ENDIF(EXISTS "$ENV{OSSIM_DEPENDENCIES}")

SET(CMAKE_INSTALL_PREFIX ${OSSIM_INSTALL_PREFIX})

MESSAGE( STATUS "\n========================== CMAKE FINAL SETTINGS ==========================" )
MESSAGE( STATUS "CMAKE_MODULE_PATH           = ${CMAKE_MODULE_PATH}")
MESSAGE( STATUS "CMAKE_IGNORE_PATH           = ${CMAKE_IGNORE_PATH}")
MESSAGE( STATUS "CMAKE_INCLUDE_PATH          = ${CMAKE_INCLUDE_PATH}")
MESSAGE( STATUS "CMAKE_LIBRARY_PATH          = ${CMAKE_LIBRARY_PATH}")
MESSAGE( STATUS "CMAKE_PREFIX_PATH           = ${CMAKE_PREFIX_PATH}")
MESSAGE( STATUS "CMAKE_INSTALL_PREFIX        = ${CMAKE_INSTALL_PREFIX}")
MESSAGE( STATUS "CMAKE_FRAMEWORK_PATH        = ${CMAKE_FRAMEWORK_PATH}")
MESSAGE( STATUS "CMAKE_FIND_ROOT_PATH        = ${CMAKE_FIND_ROOT_PATH}")
MESSAGE( STATUS "CMAKE_FIND_LIBRARY_PREFIXES = ${CMAKE_FIND_LIBRARY_PREFIXES}")
MESSAGE( STATUS "CMAKE_FIND_LIBRARY_SUFFIXES = ${CMAKE_FIND_LIBRARY_SUFFIXES}")
MESSAGE( STATUS "CMAKE_SYSTEM_IGNORE_PATH    = ${CMAKE_SYSTEM_IGNORE_PATH}")
MESSAGE( STATUS "CMAKE_SYSTEM_INCLUDE_PATH   = ${CMAKE_SYSTEM_INCLUDE_PATH}")
MESSAGE( STATUS "CMAKE_SYSTEM_LIBRARY_PATH   = ${CMAKE_SYSTEM_LIBRARY_PATH}")
MESSAGE( STATUS "CMAKE_SYSTEM_PREFIX_PATH    = ${CMAKE_SYSTEM_PREFIX_PATH}")
MESSAGE( STATUS "CMAKE_SYSTEM_IGNORE_PATH    = ${CMAKE_SYSTEM_IGNORE_PATH}")
MESSAGE( STATUS "CMAKE_TOOLCHAIN_FILE        = ${CMAKE_TOOLCHAIN_FILE}")
MESSAGE( STATUS "\n" )


#######################################################################################################
#  Initialize the libraries to be the cmake targets and initialize the includes to the proper dev paths
#######################################################################################################

IF(NOT OSSIM_LIBRARY)
   SET(OSSIM_LIBRARY "ossim" CACHE STRING "Caching" FORCE)
ENDIF(NOT OSSIM_LIBRARY)

IF(NOT OSSIM_HOME)
   SET(OSSIM_HOME "${OSSIM_DEV_HOME}/ossim" CACHE STRING "Caching" FORCE)
ENDIF(NOT OSSIM_HOME)

IF(NOT OSSIM_INCLUDE_DIR)
   SET(OSSIM_INCLUDE_DIR "${OSSIM_HOME}/include" CACHE STRING "Caching" FORCE)
ENDIF(NOT OSSIM_INCLUDE_DIR)


#IF(NOT OSSIMPLANETQT_LIBRARY)
#   SET(OSSIMPLANETQT_LIBRARY "ossimPlanetQt" CACHE STRING "Caching" FORCE)
#ENDIF(NOT OSSIMPLANETQT_LIBRARY)

#IF(NOT OSSIMPLANETQT_INCLUDE_DIR)
#   SET(OSSIMPLANETQT_INCLUDE_DIR "${OSSIM_DEV_HOME}/ossimPlanetQt/include" CACHE STRING "Caching" FORCE)
#ENDIF(NOT OSSIMPLANETQT_INCLUDE_DIR)

#IF(NOT OSSIMQT_LIBRARY)
#   SET(OSSIMQT_LIBRARY "ossimQt" CACHE STRING "Caching" FORCE)
#ENDIF(NOT OSSIMQT_LIBRARY)

############################### Add subdirectories to build based on BUILD variables and presence of repo ###########################
add_subdirectory(${OSSIM_HOME} ${CMAKE_CURRENT_BINARY_DIR}/ossim)

IF(BUILD_OSSIM_WMS AND EXISTS ${OSSIM_DEV_HOME}/ossim-wms )
  IF(NOT OSSIMWMS_LIBRARY)
     SET(OSSIMWMS_LIBRARY "ossim-wms" CACHE STRING "Caching" FORCE)
  ENDIF(NOT OSSIMWMS_LIBRARY)

  IF(NOT OSSIMWMS_INCLUDE_DIR)
     SET(OSSIMWMS_INCLUDE_DIR "${OSSIM_DEV_HOME}/ossim-wms/include" CACHE STRING "Caching" FORCE)
  ENDIF(NOT OSSIMWMS_INCLUDE_DIR)
  add_subdirectory(${OSSIM_DEV_HOME}/ossim-wms ${CMAKE_CURRENT_BINARY_DIR}/ossim-wms)
ENDIF()

IF(BUILD_OSSIM_VIDEO AND EXISTS ${OSSIM_DEV_HOME}/ossim-video)
  IF(NOT OSSIMVIDEO_LIBRARY)
      SET(OSSIMVIDEO_LIBRARY "ossim-video" CACHE STRING "Caching" FORCE)
  ENDIF()

  IF(NOT OSSIMVIDEO_INCLUDE_DIR)
      SET(OSSIMVIDEO_INCLUDE_DIR "${OSSIM_DEV_HOME}/ossim-video/include" CACHE STRING "Caching" FORCE)
  ENDIF()
  add_subdirectory(${OSSIM_DEV_HOME}/ossim-video ${CMAKE_CURRENT_BINARY_DIR}/ossim-video)
ENDIF()

IF(BUILD_MSP_PLUGIN AND EXISTS ${OSSIM_DEV_HOME}/ossim-msp-plugin)
  add_subdirectory(${OSSIM_DEV_HOME}/ossim-msp-plugin ${CMAKE_CURRENT_BINARY_DIR}/ossim-msp-plugin)
ENDIF()

IF(BUILD_CSM_PLUGIN AND EXISTS ${OSSIM_DEV_HOME}/ossim-csm-plugin)
  add_subdirectory(${OSSIM_DEV_HOME}/ossim-csm-plugin ${CMAKE_CURRENT_BINARY_DIR}/ossim-csm-plugin)
ENDIF()

IF(BUILD_OMS AND EXISTS ${OSSIM_DEV_HOME}/ossim-oms)
  IF(NOT OMS_LIBRARY)
     SET(OMS_LIBRARY "ossim-oms" CACHE STRING "Caching" FORCE)
  ENDIF(NOT OMS_LIBRARY)

  IF(NOT OMS_INCLUDE_DIR)
     SET(OMS_INCLUDE_DIR "${OSSIM_DEV_HOME}/ossim-oms/coms/include" CACHE STRING "Caching" FORCE)
  ENDIF(NOT OMS_INCLUDE_DIR)
  add_subdirectory(${OSSIM_DEV_HOME}/ossim-oms ${CMAKE_CURRENT_BINARY_DIR}/ossim-oms)
ENDIF()

IF(BUILD_OSSIM_PLANET AND EXISTS ${OSSIM_DEV_HOME}/ossim-planet)

  IF(NOT OSSIMPLANET_LIBRARY)
     SET(OSSIMPLANET_LIBRARY "ossim-planet" CACHE STRING "Caching" FORCE)
  ENDIF(NOT OSSIMPLANET_LIBRARY)
  IF(NOT OSSIMPLANET_INCLUDE_DIR)
     SET(OSSIMPLANET_INCLUDE_DIR "${OSSIM_DEV_HOME}/ossim-planet/include" CACHE STRING "Caching" FORCE)
  ENDIF(NOT OSSIMPLANET_INCLUDE_DIR)

  add_subdirectory(${OSSIM_DEV_HOME}/ossim-planet ${CMAKE_CURRENT_BINARY_DIR}/ossim-planet)
ENDIF()

IF(BUILD_OSSIM_PLANET_GUI AND EXISTS ${OSSIM_DEV_HOME}/ossim-planet-gui)
  IF(NOT OSSIMPLANETGUI_LIBRARY)
     SET(OSSIMPLANETGUI_LIBRARY "ossim-planet-gui" CACHE STRING "Caching" FORCE)
  ENDIF(NOT OSSIMPLANETGUI_LIBRARY)
  IF(NOT OSSIMPLANETGUI_INCLUDE_DIR)
     SET(OSSIMPLANETGUI_INCLUDE_DIR "${OSSIM_DEV_HOME}/ossim-planet-gui/include" CACHE STRING "Caching" FORCE)
  ENDIF(NOT OSSIMPLANETGUI_INCLUDE_DIR)

  add_subdirectory(${OSSIM_DEV_HOME}/ossim-planet-gui ${CMAKE_CURRENT_BINARY_DIR}/ossim-planet-gui)
ENDIF()


IF(BUILD_OSSIM_GUI AND EXISTS ${OSSIM_DEV_HOME}/ossim-gui)
     add_subdirectory(${OSSIM_DEV_HOME}/ossim-gui ${CMAKE_CURRENT_BINARY_DIR}/ossim-gui)
ENDIF()

IF(BUILD_OSSIM_QT4 AND EXISTS ${OSSIM_DEV_HOME}/ossim-qt4)
     add_subdirectory(${OSSIM_DEV_HOME}/ossim-qt4 ${CMAKE_CURRENT_BINARY_DIR}/ossim-qt4)
ENDIF()

IF(EXISTS ${OSSIM_DEV_HOME}/ossim-plugins)
  add_subdirectory(${OSSIM_DEV_HOME}/ossim-plugins ${CMAKE_CURRENT_BINARY_DIR}/ossim-plugins)
ENDIF()

IF(OSSIM_BUILD_ADDITIONAL_DIRECTORIES)
    FOREACH(it IN LISTS OSSIM_BUILD_ADDITIONAL_DIRECTORIES)
        MESSAGE(STATUS "**********************Adding additional subdirectory ${it}*************************")
        SUBDIRS(${it})
    ENDFOREACH(it)
ENDIF()

IF(BUILD_DSMG AND EXISTS ${OSSIM_DEV_HOME}/ossim-dsmg)
  add_subdirectory(${OSSIM_DEV_HOME}/ossim-dsmg ${CMAKE_CURRENT_BINARY_DIR}/ossim-dsmg)
ENDIF()

IF(BUILD_OSSIM_ISA AND EXISTS ${OSSIM_DEV_HOME}/ossim-isa)
   add_subdirectory(${OSSIM_DEV_HOME}/ossim-isa ${CMAKE_CURRENT_BINARY_DIR}/ossim-isa)
ENDIF()

IF(BUILD_OSSIM_FPGA AND EXISTS ${OSSIM_DEV_HOME}/ossim-fpga)
   add_subdirectory(${OSSIM_DEV_HOME}/ossim-fpga ${CMAKE_CURRENT_BINARY_DIR}/ossim-fpga)
ENDIF()

IF(BUILD_OSSIM_DEEPCORE AND EXISTS ${OSSIM_DEV_HOME}/ossim-deepcore)
   add_subdirectory(${OSSIM_DEV_HOME}/ossim-deepcore ${CMAKE_CURRENT_BINARY_DIR}/ossim-deepcore)
ENDIF()


############################## Doxygen installation for current ossim core module ###########################################
FIND_PACKAGE(Doxygen)

MACRO(OSSIM_GENERATE_DOXYGEN  DOXYGEN_OUTPUT_DIRECTORY CONFIGURE_FILE WORKING_DIRECTORY DOXYGEN_INPUT BASE_TARGET_NAME CLASS_PREFIX_TO_IGNORE)

     SET(DOXYGEN_IGNORE_PREFIX "${CLASS_PREFIX_TO_IGNORE}")

     IF(NOT EXISTS "${DOXYGEN_OUTPUT_DIRECTORY}")
         make_directory("${DOXYGEN_OUTPUT_DIRECTORY}")
     ENDIF()
     CONFIGURE_FILE("${CONFIGURE_FILE}"
                    "${DOXYGEN_INPUT}" @ONLY IMMEIDATE)

     ADD_CUSTOM_COMMAND(
       OUTPUT ${DOXYGEN_OUTPUT_DIRECTORY}
       COMMAND ${CMAKE_COMMAND} -E echo_append "Building API Documentation..."
       COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_INPUT}
       COMMAND ${CMAKE_COMMAND} -E echo "Done."
       WORKING_DIRECTORY ${WORKING_DIRECTORY}
       DEPENDS ${DOXYGEN_INPUT}
     )

     ADD_CUSTOM_TARGET(${BASE_TARGET_NAME}-doc ALL DEPENDS ${DOXYGEN_OUTPUT_DIRECTORY})

     ADD_CUSTOM_TARGET(${BASE_TARGET_NAME}-doc-forced
       COMMAND ${CMAKE_COMMAND} -E echo_append "Building API Documentation..."
       COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_INPUT}
       COMMAND ${CMAKE_COMMAND} -E echo "Done."
       WORKING_DIRECTORY "${WORKING_DIRECTORY}")
ENDMACRO(OSSIM_GENERATE_DOXYGEN)

IF (DOXYGEN_FOUND)
  OPTION(OSSIM_BUILD_DOXYGEN "Set to ON to generate oxygen documentation files" OFF)
  IF(OSSIM_BUILD_DOXYGEN)

     SET(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/doc/ossim-${OSSIM_VERSION}/ossimApi")
     OSSIM_GENERATE_DOXYGEN("${DOXYGEN_OUTPUT_DIRECTORY}"
                      "${OSSIM_DEV_HOME}/ossim/doc/Doxyfile.in"
                      "${OSSIM_DEV_HOME}/ossim/doc"
                      "${CMAKE_BINARY_DIR}/OssimDoxyfile"
                      "ossim-api"
                      "ossim")
     FILE(GLOB ossim_docs "${DOXYGEN_OUTPUT_DIRECTORY}/html/*")
     INSTALL(FILES ${ossim_docs} DESTINATION "share/ossim/${OSSIM_VERSION}/docs/doxygen/ossimapi/html" COMPONENT ossim-doc)

     IF(BUILD_OSSIM_PLANET)
        SET(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/doc/ossim-${OSSIM_VERSION}/ossimPlanetApi")
        OSSIM_GENERATE_DOXYGEN("${DOXYGEN_OUTPUT_DIRECTORY}"
                         "${OSSIM_DEV_HOME}/ossim/Doxyfile.in"
                         "${OSSIM_DEV_HOME}/ossim-planet"
                         "${CMAKE_BINARY_DIR}/OssimPlanetDoxyfile"
                         "ossimPlanet-api"
                         "ossimPlanet")
        FILE(GLOB ossim_docs "${DOXYGEN_OUTPUT_DIRECTORY}/html/*")
        INSTALL(FILES ${ossim_docs} DESTINATION "share/ossim/${OSSIM_VERSION}/docs/doxygen/ossimPlanetApi/html" COMPONENT ossim-doc)
     ENDIF(BUILD_OSSIM_PLANET)

   ENDIF(OSSIM_BUILD_DOXYGEN)
ENDIF (DOXYGEN_FOUND)

############################## Setup shared file installation ##############################
if(EXISTS ${OSSIM_HOME}/share/ossim/geoids/egm96.grd)
  INSTALL(FILES ${OSSIM_HOME}/share/ossim/geoids/egm96.grd DESTINATION "share/ossim/geoids" COMPONENT ossim)
ELSE()
  MESSAGE(WARNING "File ${OSSIM_HOME}/share/ossim/geoids/egm96.grd not found for install!")
ENDIF()
# Fonts removed for rpms
# INSTALL(FILES ${OSSIM_DEV_HOME}/ossim_package_support/fonts/arial.ttf DESTINATION "share/ossim/fonts" COMPONENT ossim)
# INSTALL(FILES ${OSSIM_DEV_HOME}/ossim_package_support/fonts/times.ttf DESTINATION "share/ossim/fonts" COMPONENT ossim)

FILE(GLOB ossim_cmake_modules ${OSSIM_DEV_HOME}/ossim/cmake/CMakeModules/*.cmake* )
INSTALL(FILES ${ossim_cmake_modules} DESTINATION share/ossim/CMakeModules COMPONENT ossim)

FILE(GLOB projection_files "${OSSIM_HOME}/share/ossim/projection/*.csv"  "${OSSIM_HOME}/share/ossim/projection/*.txt")
INSTALL(FILES ${projection_files} DESTINATION share/ossim/projection COMPONENT ossim)

INSTALL(FILES ${OSSIM_HOME}/share/ossim/templates/ossim_preferences_template DESTINATION "share/ossim" COMPONENT ossim RENAME ossim-preferences-template)

############################## Setup the packages ##############################
# present the packaging option only if we have the cpack command defined (effectively >= 2.6.0)
IF(CMAKE_CPACK_COMMAND)
    OPTION(BUILD_OSSIM_PACKAGES "Set to ON to generate CPack configuration files and packaging targets" OFF)
    IF(BUILD_OSSIM_PACKAGES)
      INCLUDE(OssimCPack)
    ENDIF()
ENDIF()

set(PKGCONFIG_FILES ossim)

foreach(PKGCONFIG_FILE ${PKGCONFIG_FILES})
  configure_file(${OSSIM_DEV_HOME}/ossim/cmake/packaging/pkgconfig/${PKGCONFIG_FILE}.pc.in
    ${${PROJECT_NAME}_BINARY_DIR}/packaging/pkgconfig/${PKGCONFIG_FILE}.pc
    @ONLY
    )
  INSTALL(FILES ${${PROJECT_NAME}_BINARY_DIR}/packaging/pkgconfig/${PKGCONFIG_FILE}.pc DESTINATION lib${LIBSUFFIX}/pkgconfig COMPONENT ossim)
ENDFOREACH(PKGCONFIG_FILE)


#####################################################################################################
# We may want to add this as a build step instead so it can capture different output directories
# Will comment out for now until I figure this out.  This is used to symlink the pre built frameworks to the build
# dir so you can run things within Xcode
#
#####################################################################################################
#IF(APPLE)
#    IF(MAKE_APPENDS_BUILD_TYPE)
#        IF(OSSIM_DEPENDENCIES)
#             IF(NOT EXISTS ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/OpenThreads.framework AND EXISTS ${OSSIM_DEPENDENCIES}/Frameworks)
#                  FILE(GLOB framework_files "${OSSIM_DEPENDENCIES}/Frameworks/*.framework")
#                  foreach(x ${framework_files})
#                     execute_process(COMMAND ln -s  ${x} .
#                                     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/)
#                  endforeach(x)
#             ENDIF()
#        ENDIF()
#    ENDIF()
#ENDIF()


############################ setup common INSTALL support files #############################################
# SET(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS "YES")
include(InstallRequiredSystemLibraries)
################################### SETUP Uninstall TARGETS ####################################################
OSSIM_ADD_COMMON_MAKE_UNINSTALL()

MESSAGE( STATUS "PROJECT_NAME                    = ${PROJECT_NAME}" )
MESSAGE( STATUS "OSSIM_DEV_HOME                  = ${OSSIM_DEV_HOME}" )
MESSAGE( STATUS "OSSIM_INSTALL_PREFIX            = ${OSSIM_INSTALL_PREFIX}" )
MESSAGE( STATUS "OSSIM_DEPENDENCIES              = ${OSSIM_DEPENDENCIES}" )
MESSAGE( STATUS "CMAKE_CURRENT_BINARY_DIR        = ${CMAKE_CURRENT_BINARY_DIR}" )
MESSAGE( STATUS "CMAKE_CURRENT_SOURCE_DIR        = ${CMAKE_CURRENT_SOURCE_DIR}" )
MESSAGE( STATUS "CMAKE_MODULE_PATH               = ${CMAKE_MODULE_PATH}" )
MESSAGE( STATUS "CMAKE_BUILD_TYPE                = ${CMAKE_BUILD_TYPE}" )
MESSAGE( STATUS "CMAKE_C_COMPILER                = ${CMAKE_C_COMPILER}" )
MESSAGE( STATUS "CMAKE_CXX_COMPILER              = ${CMAKE_CXX_COMPILER}" )
MESSAGE( STATUS "CMAKE_CXX_FLAGS                 = ${CMAKE_CXX_FLAGS}" )
MESSAGE( STATUS "CMAKE_CXX_FLAGS_DEBUG           = ${CMAKE_CXX_FLAGS_DEBUG}" )
MESSAGE( STATUS "CMAKE_CXX_FLAGS_RELEASE         = ${CMAKE_CXX_FLAGS_RELEASE}" )
MESSAGE( STATUS "CMAKE_CXX_DEFINES_DEBUG         = ${CXX_DEFINES}" )
MESSAGE( STATUS "CMAKE_CXX_DEFINES_RELEASE       = ${CMAKE_CXX_DEFINES}" )
MESSAGE( STATUS "CMAKE_DEBUG_POSTFIX             = ${CMAKE_DEBUG_POSTFIX}" )
MESSAGE( STATUS "CMAKE_INCLUDE_PATH              = ${CMAKE_INCLUDE_PATH}" )
MESSAGE( STATUS "CMAKE_INSTALL_PREFIX            = ${CMAKE_INSTALL_PREFIX}" )
MESSAGE( STATUS "CMAKE_SHARED_LINKER_FLAGS       = ${CMAKE_SHARED_LINKER_FLAGS}" )
MESSAGE( STATUS "CMAKE_EXE_LINKER_FLAGS          = ${CMAKE_EXE_LINKER_FLAGS}" )
MESSAGE( STATUS "COMPILE_DEFINITIONS_DEBUG       = ${COMPILE_DEFINITIONS_DEBUG}" )
MESSAGE( STATUS "COMPILE_DEFINITIONS_RELEASE     = ${COMPILE_DEFINITIONS_RELEASE}" )
MESSAGE( STATUS "KAKADU_INCLUDE_DIR              = ${KAKADU_INCLUDE_DIR}")
MESSAGE( STATUS "KAKADU_ROOT_SRC                 = ${KAKADU_ROOT_SRC}")
MESSAGE( STATUS "KAKADU_AUX_LIBRARY              = ${KAKADU_AUX_LIBRARY}")
MESSAGE( STATUS "KAKADU_LIBRARY                  = ${KAKADU_LIBRARY}")
MESSAGE( STATUS "MPI_LIBRARY                     = ${MPI_LIBRARIES}" )
MESSAGE( STATUS "MPI_INCLUDE                     = ${MPI_INCLUDE_PATH}" )
MESSAGE( STATUS "OPENTHREADS_LIBRARY             = ${OPENTHREADS_LIBRARY}" )
MESSAGE( STATUS "OPENTHREADS_INCLUDE             = ${OPENTHREADS_INCLUDE_DIR}" )
MESSAGE( STATUS "OSSIM_COMPILE_WITH_FULL_WARNING = ${OSSIM_COMPILE_WITH_FULL_WARNING}" )
MESSAGE( STATUS "OSSIM_COMMON_COMPILER_FLAGS     = ${OSSIM_COMMON_COMPILER_FLAGS}" )
MESSAGE( STATUS "OSSIM_LIBRARY                   = ${OSSIM_LIBRARY}" )
MESSAGE( STATUS "OSSIM_INCLUDE_DIR               = ${OSSIM_INCLUDE_DIR}" )
MESSAGE( STATUS "TIFF_LIBRARY                    = ${TIFF_LIBRARIES}" )
MESSAGE( STATUS "TIFF_INCLUDE                    = ${TIFF_INCLUDE_DIR}" )
MESSAGE( STATUS "ZLIB_LIBRARY                    = ${ZLIB_LIBRARIES}" )
MESSAGE( STATUS "ZLIB_INCLUDE                    = ${ZLIB_INCLUDE_DIR}" )
MESSAGE( STATUS "BUILD_OMS                       = ${BUILD_OMS}" )
MESSAGE( STATUS "BUILD_OSSIM                     = ${BUILD_OSSIM}" )
MESSAGE( STATUS "BUILD_OSSIM_FREETYPE_SUPPORT    = ${BUILD_OSSIM_FREETYPE_SUPPORT}" )
MESSAGE( STATUS "BUILD_OSSIM_GUI                 = ${BUILD_OSSIM_GUI}" )
MESSAGE( STATUS "BUILD_OSSIMJNI                  = ${BUILD_OSSIMJNI}" )
MESSAGE( STATUS "BUILD_OSSIM_QT4                 = ${BUILD_OSSIM_QT4}" )
MESSAGE( STATUS "BUILD_OSSIM_MPI_SUPPORT         = ${BUILD_OSSIM_MPI_SUPPORT}" )
MESSAGE( STATUS "BUILD_OSSIM_PLANET              = ${BUILD_OSSIM_PLANET}" )
MESSAGE( STATUS "BUILD_OSSIMPLANETQT             = ${BUILD_OSSIMPLANETQT}" )
MESSAGE( STATUS "BUILD_OSSIM_VIDEO               = ${BUILD_OSSIM_VIDEO}" )
MESSAGE( STATUS "BUILD_OSSIM_WMS                 = ${BUILD_OSSIM_WMS}" )
MESSAGE( STATUS "BUILD_LIBRARY_DIR               = ${BUILD_LIBRARY_DIR}" )
MESSAGE( STATUS "BUILD_RUNTIME_DIR               = ${BUILD_RUNTIME_DIR}" )
MESSAGE( STATUS "BUILD_DSMG                      = ${BUILD_DSMG}" )
MESSAGE( STATUS "BUILD_MSP_PLUGIN                = ${BUILD_MSP_PLUGIN}" )
MESSAGE( STATUS "BUILD_CSM_PLUGIN                = ${BUILD_CSM_PLUGIN}" )
MESSAGE( STATUS "BUILD_OSSIM_FPGA                = ${BUILD_OSSIM_FPGA}" )
MESSAGE( STATUS "BUILD_OSSIM_DEEPCORE            = ${BUILD_OSSIM_DEEPCORE}" )
MESSAGE( STATUS "USE_OSSIM_JSONCPP               = ${USE_OSSIM_JSONCPP}" )
MESSAGE( STATUS "Use OSSIM_BUILD_ADDITIONAL_DIRECTORIES to add other cmake builds." )
