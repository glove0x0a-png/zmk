zephyr_include_directories(include)

add_subdirectory(drivers)
add_subdirectory(lib)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_ZMK_AZ1UBALL az1uball.c)
