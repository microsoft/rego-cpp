# build the documentation from the C++ comments
configure_file( 
  Doxyfile.in 
  ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY 
)

configure_file(regocpp-logo-48.png ${CMAKE_CURRENT_BINARY_DIR}/regocpp-logo-48.png COPYONLY)

add_custom_target( regocpp_doc
  ALL
  ${DOXYGEN_EXECUTABLE} 
  ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating API documentation with Doxygen" VERBATIM
)
