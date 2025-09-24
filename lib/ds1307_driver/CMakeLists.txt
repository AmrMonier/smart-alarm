FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_LIST_DIR}/*.*)
idf_component_register(SRCS ${app_sources})