zephyr_library()
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}../../../../include)

# Add source file only if CONFIG_AZ1UBALL is enabled
zephyr_library_sources_ifdef(CONFIG_ZMK_AZ1UBALL az1uball.c)

# Add include directories
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
