set(ATLAS_SVG_DIR "${PROJECT_SOURCE_DIR}/art")
set(ATLAS_PNG_DIR "${PROJECT_SOURCE_DIR}/res/atlas")
set(ATLAS_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/src")
set(ATLAS_HEADER "${ATLAS_GEN_DIR}/atlas.h")

add_executable(atlasgen main.c)
target_include_directories(atlasgen PRIVATE vendor)
target_link_libraries(atlasgen PRIVATE marrow printccy m)

file(GLOB ATLAS_SVGS CONFIGURE_DEPENDS "${ATLAS_SVG_DIR}/*.svg")

add_custom_command(
  OUTPUT  "${ATLAS_HEADER}"
  COMMAND "${CMAKE_COMMAND}" -E rm -rf "${ATLAS_PNG_DIR}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${ATLAS_PNG_DIR}" "${ATLAS_GEN_DIR}"
  COMMAND atlasgen "${ATLAS_PNG_DIR}" "${ATLAS_GEN_DIR}" ${ATLAS_SVGS}
  DEPENDS atlasgen ${ATLAS_SVGS}
  VERBATIM
)
add_custom_target(atlas_gen DEPENDS "${ATLAS_HEADER}")

add_library(atlas INTERFACE)
target_include_directories(atlas INTERFACE "${ATLAS_GEN_DIR}")
add_dependencies(atlas atlas_gen)
