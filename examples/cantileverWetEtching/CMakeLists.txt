cmake_minimum_required(VERSION 3.4)

project("cantileverWetEtching")

add_executable(${PROJECT_NAME} ${PROJECT_NAME}.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${VIENNAPS_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${VIENNAPS_LIBRARIES})
configure_file(cantileverWetEtching.py ${CMAKE_CURRENT_BINARY_DIR}/cantileverWetEtching.py COPYONLY)
configure_file(cantilever_mask.gds ${CMAKE_CURRENT_BINARY_DIR}/cantilever_mask.gds COPYONLY)

add_dependencies(buildExamples ${PROJECT_NAME})
