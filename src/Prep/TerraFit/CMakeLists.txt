
add_executable(terrafit terrafit.cc)

target_link_libraries(terrafit 
    terragear
    Terra
    ${ZLIB_LIBRARY}
	${SIMGEAR_CORE_LIBRARIES}
	${SIMGEAR_CORE_LIBRARY_DEPENDENCIES})
	
install(TARGETS terrafit RUNTIME DESTINATION bin)
