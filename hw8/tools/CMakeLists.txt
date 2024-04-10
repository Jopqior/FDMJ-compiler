set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result")

set(LIBS 
  frontend
  optimizer
  utils
  vendor_lxml
)
# add_dependencies(${LIBS} vendor_libsysy)

set(main "main")
add_executable(${main} main.c)
target_link_libraries(${main} PUBLIC ${LIBS})