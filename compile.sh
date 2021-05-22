#!/usr/bin/env sh

# Copyright (C) 2021  Nicole Alassandro

# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.

# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.

set -e

APP_NAME="QuickRender"
APP_VERSION="1.0.0"

COMPILE_FLAGS=" "

# Debug
# COMPILE_FLAGS+="-g "
# COMPILE_FLAGS+="-fsanitize=undefined "
# COMPILE_FLAGS+="-fsanitize=address "
# COMPILE_FLAGS+="-fsanitize=thread "

# Release
COMPILE_FLAGS+="-DSDL_ASSERT_LEVEL=0"
COMPILE_FLAGS+="-O3 "

# COMPILE_FLAGS+="-v "
COMPILE_FLAGS+="-std=c11 "
COMPILE_FLAGS+="-pedantic "
COMPILE_FLAGS+="-Wall "
COMPILE_FLAGS+="-Wextra "
COMPILE_FLAGS+="-Wconversion "
COMPILE_FLAGS+="-Wno-unused-label "
COMPILE_FLAGS+="-Wno-unused-function "
COMPILE_FLAGS+="-Werror "
COMPILE_FLAGS+="-ferror-limit=10 "
COMPILE_FLAGS+="-DAPP_NAME=\"$APP_NAME\" "
COMPILE_FLAGS+="-mmacosx-version-min=10.14 "
COMPILE_FLAGS+="-F/Library/Frameworks "
COMPILE_FLAGS+="-framework SDL2 "

rm -rf "Build/"
mkdir "Build/"

cc \
    $COMPILE_FLAGS  \
    -o "Build/$APP_NAME"   \
    "Source/Main.c"

chmod +x "Build/$APP_NAME"
