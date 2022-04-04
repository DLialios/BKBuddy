add_executable(bkbmaster 
    src/bkbmaster.cxx
    )

target_link_libraries(bkbmaster PUBLIC
    X11
    )

target_include_directories(bkbmaster PUBLIC
    include
    )
