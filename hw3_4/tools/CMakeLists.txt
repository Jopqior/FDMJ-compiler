set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result")

set(Parser0 "Parser0")
add_executable(${Parser0} parser0.c)
target_link_libraries(${Parser0} PUBLIC
  frontend
  utils
)
install(TARGETS ${Parser0}
  RUNTIME DESTINATION bin
  COMPONENT ${Parser0})
