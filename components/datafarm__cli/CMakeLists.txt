# CMakeLists.txt for my_component

idf_component_register(SRCS "src/main.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES driver)  # List any dependencies here
