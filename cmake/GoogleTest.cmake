find_library (GTEST_LIBRARY gtest)
find_library (GTEST_MAIN_LIBRARY gtest_main)
find_library (GMOCK_LIBRARY gmock)
find_library (GMOCK_MAIN_LIBRARY gmock_main)

if (GTEST_LIBRARY AND GTEST_MAIN_LIBRARY AND GMOCK_LIBRARY AND GMOCK_MAIN_LIBRARY)

    find_package (Threads REQUIRED)

    set (GTEST_BOTH_LIBRARIES ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

    add_definitions (-DGTEST_USE_OWN_TR1_TUPLE=0)

    set (GTEST_FOUND TRUE)
    set (GOOGLE_TEST_AND_MOCK_FOUND TRUE)

else ()

    message ("Google Test not found - cannot build tests!")

    set (GTEST_FOUND FALSE)
    set (GOOGLE_TEST_AND_MOCK_FOUND FALSE)

endif ()
