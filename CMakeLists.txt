CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

PROJECT(app-checker C)
SET(VERSION_MAJOR 0)
SET(VERSION "${VERSION_MAJOR}.1.0")

### Global setting ###

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(INCLUDEDIR "\${prefix}/include/app-checker")

# Build type : Release
IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF()
MESSAGE("Build type: ${CMAKE_BUILD_TYPE}")


# Set required packages
INCLUDE(FindPkgConfig)

pkg_check_modules(pkgs REQUIRED dlog glib-2.0)
pkg_check_modules(libpkgs REQUIRED dlog glib-2.0)

FIND_LIBRARY(LIB_DL dl)

FOREACH(flag ${libpkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

FOREACH(flag ${pkgs_CFLAGS})
	SET(TEST_CFLAGS "${TEST_CFLAGS} ${flag}")
ENDFOREACH(flag)

# Compiler flags
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/legacy)
SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -Wl,-zdefs" )
SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -fvisibility=hidden")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_C_FLAGS_RELEASE "-O2")

SET(CMAKE_SKIP_BUILD_RPATH true)

# Get uname value to set 'TARGET' definition 
# TODO: Is this needed?
FIND_PROGRAM(UNAME NAMES uname)
EXEC_PROGRAM("${UNAME}" ARGS "-m" OUTPUT_VARIABLE "ARCH")
IF("${ARCH}" STREQUAL "arm")
	ADD_DEFINITIONS("-DTARGET")
	MESSAGE("add -DTARGET")
ENDIF("${ARCH}" STREQUAL "arm")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS("-DLIBPREFIX=\"${LIB_INSTALL_DIR}\"")
# Linker flags
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")


### Build ###


# app-checker
add_library(app-checker SHARED
		src/ac_lib.c
		src/ac_sock.c
		)
target_link_libraries(app-checker ${libpkgs_LDFLAGS})
SET_TARGET_PROPERTIES(app-checker PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(app-checker PROPERTIES VERSION ${VERSION})

# 
SET(DAEMON_NAME "app-checker-server")
add_library(${DAEMON_NAME} SHARED
		src/ac_server.c
	src/ac_sock.c
		)

target_link_libraries(${DAEMON_NAME} ${pkgs_LDFLAGS} ${LIB_DL})
SET_TARGET_PROPERTIES(${DAEMON_NAME} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${DAEMON_NAME} PROPERTIES VERSION ${VERSION})

# pkgconfig file
CONFIGURE_FILE(app-checker.pc.in app-checker.pc @ONLY)
CONFIGURE_FILE(app-checker-server.pc.in app-checker-server.pc @ONLY)


MESSAGE("LIBDIR ${LIB_INSTALL_DIR}")
### Install ###
INSTALL(TARGETS app-checker DESTINATION ${LIB_INSTALL_DIR} COMPONENT RuntimeLibraries)
INSTALL(TARGETS ${DAEMON_NAME} DESTINATION ${LIB_INSTALL_DIR} COMPONENT RuntimeLibraries)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/app-checker.h DESTINATION include/app-checker)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/app-checker.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/app-checker-server.h DESTINATION include/app-checker)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/app-checker-server.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)




