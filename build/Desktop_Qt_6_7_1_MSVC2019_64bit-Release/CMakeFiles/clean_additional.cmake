# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\Translate_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\Translate_autogen.dir\\ParseCache.txt"
  "Translate_autogen"
  )
endif()
