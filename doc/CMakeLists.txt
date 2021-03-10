# @author  Barthélémy von Haller

find_package(Doxygen OPTIONAL_COMPONENTS dot)

if (DOXYGEN_FOUND)
  message(AAAAAAAAAA)

  # Configure the doxygen config file with current settings
  set(DOXYGEN_EXCLUDE "${CMAKE_SOURCE_DIR}/Framework/src/imgui")
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "README.md")
  set(DOXYGEN_MARKDOWN_SUPPORT "YES")
  set(DOXYGEN_GENERATE_TREEVIEW YES)
  set(DOXYGEN_HIDE_UNDOC_RELATIONS NO)
  set(DOXYGEN_HAVE_DOT YES)
  set(DOXYGEN_DOT_NUM_THREADS 4)
  set(DOXYGEN_UML_LOOK YES)
  set(DOXYGEN_UML_LIMIT_NUM_FIELDS 50)
  set(DOXYGEN_TEMPLATE_RELATIONS YES)
  set(DOXYGEN_DOT_IMAGE_FORMAT svg)
  set(DOXYGEN_INTERACTIVE_SVG YES)
  set(DOXYGEN_DOT_GRAPH_MAX_NODES 100)
  set(DOXYGEN_DOT_TRANSPARENT YES)
  set(DOXYGEN_INPUT_FILTER "${CMAKE_SOURCE_DIR}/doc/scripts/filter_for_doxygen.sh")
  set(DOXYGEN_FILTER_PATTERNS "*.md")

  # add target "doc"
  doxygen_add_docs(doc
#          ${CMAKE_SOURCE_DIR}/README.md
#          ${CMAKE_SOURCE_DIR}/Framework
#          ${CMAKE_SOURCE_DIR}/Modules/
#          ${CMAKE_SOURCE_DIR}/Framework/include
          ${CMAKE_SOURCE_DIR}/doc
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
          COMMENT "Generating doxygen documentation for ${PROJECT_NAME}"
  )
  message(AAAAAAbAAAA)

  # installation
  option(DOC_INSTALL "Install the documentation when calling \"make install\"" OFF)
  if(DOC_INSTALL)
    message(STATUS "Documentation will be installed but you *must* run `make doc`")
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc/${PROJECT_NAME} COMPONENT doc)
  endif(DOC_INSTALL)

endif(DOXYGEN_FOUND)
