find_package(PythonInterp 3)

if (PYTHON_EXECUTABLE)
    #install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} install --prefix=${CMAKE_INSTALL_PREFIX} ${DISTUTILS_INSTALL_FLAGS})")

    # Determine installation path
    execute_process(COMMAND
        ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(prefix='${CMAKE_INSTALL_PREFIX}'))"
        OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)

    install(DIRECTORY manus DESTINATION ${PYTHON_SITE_PACKAGES})
    install(DIRECTORY manus_webshell DESTINATION ${PYTHON_SITE_PACKAGES})
	install(DIRECTORY manus_apps DESTINATION ${PYTHON_SITE_PACKAGES})
	install(DIRECTORY manus_starter DESTINATION ${PYTHON_SITE_PACKAGES})
	install(DIRECTORY ${CMAKE_BINARY_DIR}/python/manus DESTINATION ${PYTHON_SITE_PACKAGES})
endif()
