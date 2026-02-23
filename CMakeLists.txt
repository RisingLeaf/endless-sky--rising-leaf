cmake_minimum_required(VERSION 3.19...4.0)

include(CMakeDependentOption)
cmake_dependent_option(ES_STEAM "Build the game for the Steam Linux runtime" OFF UNIX OFF)
cmake_dependent_option(ES_CREATE_BUNDLE "Create a Bundle instead of an executable. Not suitable for development purposes." OFF APPLE OFF)

set(CMAKE_CONFIGURATION_TYPES "Debug" "Release" CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 23 CACHE STRING "")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

project("Endless Sky" VERSION 0.10.17
	DESCRIPTION "Space exploration, trading, and combat game."
	HOMEPAGE_URL https://endless-sky.github.io/
	LANGUAGES CXX)

add_subdirectory(dependencies/SDL3)
add_subdirectory(dependencies/libavif)
add_subdirectory(dependencies/libjpeg-turbo)
add_subdirectory(dependencies/libpng)
add_subdirectory(dependencies/openal-soft)
find_package(Threads REQUIRED)
add_subdirectory(dependencies/flac)

add_subdirectory(dependencies/miniz)
add_subdirectory(dependencies/miniaudio)

if(MINGW AND WIN32)
	get_filename_component(PARENT_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
	get_filename_component(MINGW_RUNTIME_DIR "${PARENT_DIR}" DIRECTORY)

	foreach(lib "stdc++-6" "winpthread-1" "gcc_s_seh-1" "gcc_s_dw2-1")
		file(GLOB_RECURSE FILE_PATH "${MINGW_RUNTIME_DIR}/lib${lib}.dll")
		if(FILE_PATH)
			list(APPEND MINGW_RUNTIME "${FILE_PATH}")
		endif()
	endforeach()
endif()

add_subdirectory(source)

add_library(ExternalLibraries INTERFACE)
target_link_libraries(EndlessSkyLib PUBLIC ExternalLibraries)

target_link_libraries(ExternalLibraries INTERFACE SDL3::SDL3 avif FLAC::FLAC++ miniz OpenAL miniaudio libjpeg_turbo Threads::Threads png_static)


if(MINGW)
	target_link_libraries(ExternalLibraries INTERFACE mingw32)
endif()

if(WIN32)
	target_link_libraries(ExternalLibraries INTERFACE rpcrt4 Winmm)
elseif(ES_LINK_LIBATOMIC)
	target_link_libraries(ExternalLibraries INTERFACE pthread atomic)
else()
	target_link_libraries(ExternalLibraries INTERFACE pthread)
endif()

if(APPLE)
else()
	find_library(UUID_LIB uuid REQUIRED)
	target_link_libraries(ExternalLibraries INTERFACE "${UUID_LIB}")
endif()

include(CTest)
if(BUILD_TESTING)
	add_subdirectory(tests)
endif()

if(APPLE AND ES_CREATE_BUNDLE)
	add_executable(EndlessSky MACOSX_BUNDLE source/main.cpp)

	# MacOS bundles are a bit special and need every resource specified when
	# creating the executable.
	# NOTE: This means that a reconfigure is needed if any assets are added/removed!
	foreach(folder "data" "images" "shaders" "sounds")
		file(GLOB_RECURSE RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${folder}/*")
		target_sources(EndlessSky PRIVATE "${RESOURCES}")

		foreach(FILE ${RESOURCES})
			# Get the relative path from the root folder to the current file.
			file(RELATIVE_PATH NEW_FILE "${CMAKE_CURRENT_SOURCE_DIR}" "${FILE}")
			# Get the parent directory for the new location.
			get_filename_component(FILE_PATH "${NEW_FILE}" DIRECTORY)

			# Resources belong under Resources/.
			set_source_files_properties("${FILE}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/${FILE_PATH}")
		endforeach()
	endforeach()

	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/icons/endless-sky.icns"
		COMMAND iconutil -c icns -o icons/endless-sky.icns icons/endless-sky.iconset
		DEPENDS icons/endless-sky.iconset/icon_16x16.png icons/endless-sky.iconset/icon_16x16@2x.png
			icons/endless-sky.iconset/icon_32x32.png icons/endless-sky.iconset/icon_32x32@2x.png
			icons/endless-sky.iconset/icon_128x128.png icons/endless-sky.iconset/icon_128x128@2x.png
			icons/endless-sky.iconset/icon_256x256.png icons/endless-sky.iconset/icon_256x256@2x.png
			icons/endless-sky.iconset/icon_512x512.png icons/endless-sky.iconset/icon_512x512@2x.png
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" VERBATIM)

	foreach(file "license.txt" "keys.txt" "credits.txt" "copyright" "changelog" "icons/endless-sky.icns")
		target_sources(EndlessSky PRIVATE ${file})
		set_source_files_properties(${file} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/")
	endforeach()

	set_target_properties(EndlessSky PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/resources/EndlessSky-Info.plist"
		XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME endless-sky)
	set(OUTPUT_NAME "Endless Sky")
	set_target_properties(EndlessSky PROPERTIES OUTPUT_NAME ${OUTPUT_NAME})
elseif(WIN32)
	add_executable(EndlessSky WIN32 source/main.cpp source/windows/WinApp.rc)
	set_target_properties(EndlessSky PROPERTIES OUTPUT_NAME "Endless Sky")
	if(MINGW)
		target_link_options(EndlessSky PRIVATE "-Wl,--major-subsystem-version,5,--minor-subsystem-version,1")
	endif()
	if(ES_LARGE_ADDRESS_AWARE)
		target_link_options(EndlessSky PRIVATE "-Wl,--large-address-aware")
	endif()
else()
	add_executable(EndlessSky source/main.cpp)
	set_target_properties(EndlessSky PROPERTIES OUTPUT_NAME "endless-sky")
endif()

target_link_libraries(EndlessSky PRIVATE ExternalLibraries EndlessSkyLib)

add_subdirectory(shaders)
add_dependencies(EndlessSky Shaders)

if(MINGW AND WIN32)
	foreach(FILE_PATH ${MINGW_RUNTIME})
		add_custom_command(TARGET EndlessSky POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${FILE_PATH}" "$<TARGET_FILE_DIR:EndlessSky>"
			COMMAND_EXPAND_LISTS VERBATIM)

		# Add an install rule for this DLLs, so that it is also included when installing.
		install(FILES ${FILE_PATH} DESTINATION .)
	endforeach()
endif()

if(APPLE)
	install(TARGETS EndlessSky CONFIGURATIONS Release BUNDLE DESTINATION .)
elseif(WIN32)
	install(TARGETS EndlessSky CONFIGURATIONS Release RUNTIME DESTINATION .)

	install(DIRECTORY data DESTINATION .)
	install(DIRECTORY images DESTINATION .)
	install(DIRECTORY shaders DESTINATION .)
	install(DIRECTORY sounds DESTINATION .)
	install(FILES credits.txt DESTINATION .)
	install(FILES keys.txt DESTINATION .)
	install(FILES copyright DESTINATION .)
	install(FILES changelog DESTINATION .)
	install(FILES license.txt DESTINATION .)

	set(CPACK_PACKAGE_VENDOR "Endless Sky")
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "Endless Sky")
	set(CPACK_GENERATOR "NSIS")
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/license.txt")
	set(CPACK_NSIS_MENU_LINKS
		"Endless Sky.exe" "Endless Sky"
		"https://github.com/endless-sky/endless-sky/wiki/PlayersManual" "Player's Manual")
	set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/WinApp.ico")
	set(CPACK_NSIS_INSTALLED_ICON_NAME "Endless Sky.exe")
	set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_CURRENT_SOURCE_DIR}\\\\installer\\\\welcome.bmp")
	set(CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP "${CMAKE_CURRENT_SOURCE_DIR}\\\\installer\\\\welcome.bmp")
	set(CPACK_NSIS_MUI_HEADERIMAGE "${CMAKE_CURRENT_SOURCE_DIR}\\\\installer\\\\header.bmp")
	set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS
		"RMDir /r $INSTDIR")
	include(CPack)
elseif(UNIX)
	install(TARGETS EndlessSky CONFIGURATIONS Release RUNTIME DESTINATION games)

	install(FILES io.github.endless_sky.endless_sky.desktop DESTINATION share/applications)

	install(FILES io.github.endless_sky.endless_sky.appdata.xml DESTINATION share/metainfo)

	# Most Ubuntu apps supply 16, 22, 24, 32, 48, and 256, and sometimes others.
	foreach(size "16x16" "22x22" "24x24" "32x32" "48x48" "128x128" "256x256" "512x512")
		install(FILES "icons/icon_${size}.png" DESTINATION "share/icons/hicolor/${size}/apps"
			RENAME endless-sky.png)
	endforeach()

	add_custom_command(
		OUTPUT endless-sky.6.gz
		COMMAND gzip -c endless-sky.6 > ${CMAKE_CURRENT_BINARY_DIR}/endless-sky.6.gz
		DEPENDS endless-sky.6
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		VERBATIM)
	target_sources(EndlessSky PRIVATE endless-sky.6.gz)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/endless-sky.6.gz DESTINATION share/man/man6)

	install(DIRECTORY data DESTINATION share/games/endless-sky)
	install(DIRECTORY images DESTINATION share/games/endless-sky)
	install(DIRECTORY shaders DESTINATION share/games/endless-sky)
	install(DIRECTORY sounds DESTINATION share/games/endless-sky)
	install(FILES credits.txt DESTINATION share/games/endless-sky)
	install(FILES keys.txt DESTINATION share/games/endless-sky)
	install(FILES copyright DESTINATION share/doc/endless-sky)
	install(FILES changelog DESTINATION share/doc/endless-sky)
	install(FILES license.txt DESTINATION share/doc/endless-sky)
endif()
