find_path(VORBIS_INCLUDE_DIR
  NAMES
    vorbis/codec.h
  DOC "vorbis include directory")
mark_as_advanced(VORBIS_INCLUDE_DIR)

find_library(VORBIS_LIBRARY
  NAMES
    vorbis
  DOC "vorbis library")
mark_as_advanced(VORBIS_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VORBIS REQUIRED_VARS VORBIS_LIBRARY VORBIS_INCLUDE_DIR)

if (VORBIS_FOUND)
  set(VORBIS_LIBRARIES "${VORBIS_LIBRARY}")
  set(VORBIS_INCLUDE_DIRS "${VORBIS_INCLUDE_DIR}")

  if (NOT TARGET VORBIS::VORBIS)
    add_library(VORBIS::VORBIS UNKNOWN IMPORTED)
    set_target_properties(VORBIS::VORBIS PROPERTIES
      IMPORTED_LOCATION "${VORBIS_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${VORBIS_INCLUDE_DIR}")
  endif ()
endif ()
