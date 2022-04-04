add_executable(bkbslave 
    src/bkbslave.cxx
    )

target_include_directories(bkbmaster PUBLIC
    include
    )
