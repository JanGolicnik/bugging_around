cmake_minimum_required(VERSION 3.21)
project(buggin LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

include(FetchContent)

set(THIRD_PARTY_DEPS marrow printccy ripple SDL3)

set(FETCH_MARROW_REPO    "https://github.com/JanGolicnik/marrow.git")
set(FETCH_MARROW_TAG     "master")
set(FETCH_PRINTCCY_REPO  "https://github.com/JanGolicnik/printccy.git")
set(FETCH_PRINTCCY_TAG   "master")
set(FETCH_RIPPLE_REPO    "https://github.com/JanGolicnik/ripple.git")
set(FETCH_RIPPLE_TAG     "master")
set(FETCH_SDL3_REPO      "https://github.com/libsdl-org/SDL.git")
set(FETCH_SDL3_TAG       "release-3.4.18")

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

foreach(dep IN LISTS THIRD_PARTY_DEPS)
    string(TOUPPER "${dep}" dep_upper)
    set("${dep_upper}_SOURCE_DIR" CACHE PATH "local path to ${dep}")
endforeach()

set(WEBGPU_BUILD_FROM_SOURCE TRUE)

foreach(dep IN LISTS THIRD_PARTY_DEPS)
    string(TOUPPER "${dep}" dep_upper)

    if(${dep_upper}_SOURCE_DIR)
        message(STATUS "Local ${${dep_upper}_SOURCE_DIR}")
        FetchContent_Declare(
            ${dep}
            SOURCE_DIR "${${dep_upper}_SOURCE_DIR}"
        )
    else()
        message(STATUS "Fetch ${FETCH_${dep_upper}_REPO} ${FETCH_${dep_upper}_TAG}")
        FetchContent_Declare(
            ${dep}
            GIT_REPOSITORY "${FETCH_${dep_upper}_REPO}"
            GIT_TAG        "${FETCH_${dep_upper}_TAG}"
            GIT_SHALLOW    TRUE
        )
    endif()
endforeach()

FetchContent_MakeAvailable(${THIRD_PARTY_DEPS})

add_subdirectory(webgpu)

set(SOURCES main.c)

add_executable(${PROJECT_NAME} ${SOURCES})

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
	set_target_properties(${PROJECT_NAME} PROPERTIES
		MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_GUI_IDENTIFIER "com.jangolicnik.buggin"
		XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.jangolicnik.buggin"
		MACOSX_BUNDLE_BUNDLE_NAME "buggin"
		MACOSX_BUNDLE_BUNDLE_VERSION "1"
		MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
		XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
		XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
		XCODE_EMBED_FRAMEWORKS webgpu_dawn
		XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY ON
		XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
	)
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        marrow
        printccy
        ripple
        webgpu
        SDL3::SDL3
)

target_compile_options(${PROJECT_NAME} PRIVATE
  -g
  -pedantic
  -Wall
  -Wextra
  -Werror
  -Wno-missing-field-initializers
  -Wno-unused-parameter
  -Wno-unused-function
  -Wno-misleading-indentation
  -Wno-bitfield-constant-conversion
  -Wno-language-extension-token
  -Wno-gnu-zero-variadic-macro-arguments
)

if(APPLE)
	target_compile_options(${PROJECT_NAME} PRIVATE -Wno-conversion -Wno-gnu-folding-constant)
endif()

target_copy_webgpu_binaries(${PROJECT_NAME})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:${PROJECT_NAME}>/res"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/res"
)
