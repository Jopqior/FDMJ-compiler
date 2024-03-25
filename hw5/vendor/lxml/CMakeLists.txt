set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-to-int-cast")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result")
aux_source_directory(. LXML_SRCS)
add_library(lxml OBJECT ${LXML_SRCS})