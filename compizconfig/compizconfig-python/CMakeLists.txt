# Hack
set (CMAKE_PROJECT_NAME compizconfig-python)

# Get arch
execute_process (COMMAND uname -p OUTPUT_VARIABLE PROC_ARCH)
string (REPLACE "\n" "" PROC_ARCH ${PROC_ARCH})

find_package (Python3 COMPONENTS Interpreter Development REQUIRED)

set (COMPIZCONFIG_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/compizconfig/libcompizconfig/include)

set (PY_VERSION "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
set (PY_BUILD_DIR lib.linux-${PROC_ARCH}-${PY_VERSION})
set (PY_COMPIZCONFIG_CYTHON ${CMAKE_CURRENT_SOURCE_DIR}/src/compizconfig.pyx)
set (COMPIZCONFIG_PYTHON_MODULE ${CMAKE_CURRENT_BINARY_DIR}/build/${PY_BUILD_DIR}/compizconfig.${Python3_SOABI}.so)

file (READ ${CMAKE_SOURCE_DIR}/VERSION COMPIZ_RELEASE_VERSION LIMIT 12 OFFSET 0)
string (STRIP ${COMPIZ_RELEASE_VERSION} COMPIZ_RELEASE_VERSION)
set (VERSION ${COMPIZ_RELEASE_VERSION})

add_custom_command (OUTPUT ${COMPIZCONFIG_PYTHON_MODULE}
                    COMMAND ${CMAKE_COMMAND} -E copy ${PY_COMPIZCONFIG_CYTHON} ${CMAKE_CURRENT_BINARY_DIR}
                    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/setup.py build --version=${VERSION}
                    DEPENDS ${PY_COMPIZCONFIG_CYTHON} compizconfig)

add_custom_target (compizconfig_python_module ALL
                   DEPENDS ${COMPIZCONFIG_PYTHON_MODULE})

set (UNINSTALL_COMMAND "${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/setup.py uninstall --prefix=${CMAKE_INSTALL_PREFIX} --version=${VERSION}")

compiz_add_code_to_uninstall_target (${UNINSTALL_COMMAND} ${CMAKE_CURRENT_BINARY_DIR})

install (CODE
	 "message (\"Installing python files\")
	  execute_process (COMMAND cmake -DPY_EXECUTABLE=${Python3_EXECUTABLE} -DSETUP=${CMAKE_CURRENT_SOURCE_DIR}/setup.py -DPREFIX=${CMAKE_INSTALL_PREFIX} -DWDIR=${CMAKE_CURRENT_BINARY_DIR} -DVERSION=${VERSION} -P ${compiz_SOURCE_DIR}/compizconfig/cmake/exec_setup_py_with_destdir.cmake)")

if (COMPIZ_BUILD_TESTING)

        find_program (BASH_EXECUTABLE bash)
	mark_as_advanced (FORCE Python3_EXECUTABLE)

	file (GLOB PYTHON_TESTS "tests/test_*.py")

        if (Python3_EXECUTABLE AND
            BASH_EXECUTABLE)
		set (MOCK_XML_SRC ${CMAKE_CURRENT_SOURCE_DIR}/tests/mock.xml)
		set (MOCK_XML_FILE ${CMAKE_CURRENT_BINARY_DIR}/mock.xml)

		add_custom_command (OUTPUT ${MOCK_XML_FILE}
				    COMMAND cp ${MOCK_XML_SRC}
					    ${MOCK_XML_FILE}
				    COMMENT "Copying mock.xml file into testing directory"
				    DEPENDS ${MOCK_XML_SRC})

		add_custom_target (compizconfig_python_test_mock_xml_file ALL DEPENDS
				   ${MOCK_XML_FILE})

		# For testing purposes we need the ini backend
		add_dependencies (compizconfig_python_module ini compizconfig_python_test_mock_xml_file)

		foreach (PYTHON_TEST_FILE ${PYTHON_TESTS})
			get_filename_component (PYTHON_TEST ${PYTHON_TEST_FILE} NAME_WE)
			message (STATUS "Adding test " ${PYTHON_TEST})
                        add_test (NAME CompizConfigPython.${PYTHON_TEST}
                                  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                                  COMMAND ${BASH_EXECUTABLE}
                                  ${CMAKE_SOURCE_DIR}/tests/util/run_with_ld_library_path.sh
                                  ${CMAKE_BINARY_DIR}/compizconfig/libcompizconfig/src
                                  ${Python3_EXECUTABLE} ${PYTHON_TEST_FILE})
		endforeach (PYTHON_TEST_FILE)
        endif (Python3_EXECUTABLE AND
               BASH_EXECUTABLE)

endif (COMPIZ_BUILD_TESTING)
