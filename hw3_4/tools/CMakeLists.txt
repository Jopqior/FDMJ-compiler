set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result")

set(main "main")
add_executable(${main} main.c)
target_link_libraries(${main} PUBLIC
  frontend
  utils
)
install(TARGETS ${main}
  RUNTIME DESTINATION bin
  COMPONENT ${main})
