#!/bin/sh
# Build and run the host tests for main/sky_core (no hardware).
set -e
cd "$(dirname "$0")"
clang++ -std=c++17 -Wall -Wextra -O1 -o /tmp/test_sky test_sky.cpp ../../main/sky_core.cpp
/tmp/test_sky
