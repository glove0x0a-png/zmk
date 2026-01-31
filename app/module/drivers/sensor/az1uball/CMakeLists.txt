zephyr_library()

# 正しい相対パスで include ディレクトリを追加
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../include)

# Add source file only if CONFIG_ZMK_AZ1UBALL is enabled
zephyr_library_sources_ifdef(CONFIG_ZMK_AZ1UBALL az1uball.c)

# 自身のディレクトリも追加（これはOK）
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
