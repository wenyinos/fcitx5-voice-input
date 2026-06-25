# - Try to find PipeWire
# Once done this will define
#  PIPEWIRE_FOUND - System has PipeWire
#  PIPEWIRE_INCLUDE_DIRS - The PipeWire include directories
#  PIPEWIRE_LIBRARIES - The libraries needed to use PipeWire
#
# Copyright (c) 2020, fcitx5-voice-input contributors

find_package(PkgConfig QUIET)
pkg_check_modules(PC_PIPEWIRE QUIET libpipewire-0.3)

find_path(PIPEWIRE_INCLUDE_DIRS
    NAMES pipewire/pipewire.h
    PATHS ${PC_PIPEWIRE_INCLUDE_DIRS}
)

find_library(PIPEWIRE_LIBRARIES
    NAMES pipewire-0.3
    PATHS ${PC_PIPEWIRE_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PipeWire DEFAULT_MSG
    PIPEWIRE_INCLUDE_DIRS PIPEWIRE_LIBRARIES)

mark_as_advanced(PIPEWIRE_INCLUDE_DIRS PIPEWIRE_LIBRARIES)
