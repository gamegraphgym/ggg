# Doxygen docs configuration for Game Graph Gym
# - Always defines a 'docs' target to match README usage
# - Configures Doxygen using cmake/Doxyfile.in
# - Ensures doxygen-awesome-css is available (uses local copy if present,
#   otherwise downloads into the build tree)

# Find doxygen executable without failing configure if missing
find_program(DOXYGEN_EXECUTABLE doxygen)

if(NOT DOXYGEN_EXECUTABLE)
    # Create a target that fails with a helpful message
    add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E echo "Error: doxygen not found. Install it to build documentation."
        COMMAND ${CMAKE_COMMAND} -E false
        COMMENT "Doxygen documentation build failed - doxygen not installed"
    )
else()
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        set(DOCS_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)

        # Try to download doxygen-awesome theme if not present
        set(THEME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/doxygen-awesome)
        if(NOT EXISTS ${THEME_DIR}/doxygen-awesome.css)
            set(THEME_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen-awesome)
            if(NOT EXISTS ${THEME_DIR}/doxygen-awesome.css)
                file(DOWNLOAD
                    https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/main/doxygen-awesome.css
                    ${THEME_DIR}/doxygen-awesome.css
                    STATUS DOWNLOAD_STATUS
                )
                list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
                if(NOT STATUS_CODE EQUAL 0)
                    message(WARNING "Failed to download doxygen-awesome theme, falling back to default")
                    set(THEME_DIR "")
                endif()
            endif()
        endif()

        # Set theme paths for Doxyfile
        if(THEME_DIR)
            set(DOXYGEN_AWESOME_CSS "${THEME_DIR}/doxygen-awesome.css")
                set(DOXYGEN_AWESOME_SIDEBAR "${THEME_DIR}/doxygen-awesome-sidebar.css")
                # Download sidebar CSS if not present
                if(NOT EXISTS "${DOXYGEN_AWESOME_SIDEBAR}")
                    file(DOWNLOAD
                        https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/main/doxygen-awesome-sidebar-only.css
                        ${DOXYGEN_AWESOME_SIDEBAR}
                        STATUS SIDEBAR_DOWNLOAD_STATUS
                    )
                endif()
        else()
            set(DOXYGEN_AWESOME_CSS "")
            set(DOXYGEN_AWESOME_SIDEBAR "")
        endif()

        # Configure Doxyfile
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    # Collect all header files for dependency tracking
    # CONFIGURE_DEPENDS makes CMake re-check file list on each build
    file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    )
    
    # Collect markdown docs
    file(GLOB MARKDOWN_FILES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/*.md
    )

    # Create custom command that depends on source files
    add_custom_command(
        OUTPUT ${DOCS_OUTPUT_DIR}/html/index.html
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${DOXYGEN_OUT} ${HEADER_FILES} ${MARKDOWN_FILES}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )

    # Create the docs target
    add_custom_target(docs ALL
        DEPENDS ${DOCS_OUTPUT_DIR}/html/index.html
        COMMENT "Documentation built at ${DOCS_OUTPUT_DIR}/html/index.html"
    )
    
    message(STATUS "Doxygen found: ${DOXYGEN_EXECUTABLE}")
    message(STATUS "Documentation will be generated at: ${DOCS_OUTPUT_DIR}/html/index.html")
endif()
