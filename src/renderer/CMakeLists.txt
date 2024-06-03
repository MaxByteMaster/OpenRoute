add_executable(Renderer
    Renderer.h Renderer.cpp
)

set_target_properties(Renderer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/renderer
)

target_link_libraries(Renderer PRIVATE Qt6::Widgets mapnik icuuc)

set(OPENSTREETMAP_CARTO_DIR ${CMAKE_SOURCE_DIR}/../lib/openstreetmap-carto)
set(OPENSTREETMAP_CARTO_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/renderer/openstreetmap-carto)

# Copy icon catalog into bin catalog
add_custom_command(TARGET Renderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${OPENSTREETMAP_CARTO_DIR}
    ${OPENSTREETMAP_CARTO_OUTPUT_DIR}
)
