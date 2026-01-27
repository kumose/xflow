# Copyright (C) Kumo inc. and its affiliates.
# Author: Jeff.li lijippy@163.com
# All rights reserved.
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

configure_file(${PROJECT_SOURCE_DIR}/tests/config.h.in ${PROJECT_SOURCE_DIR}/tests/config.h @ONLY)

include(${PROJECT_SOURCE_DIR}/tests/doctest/doctest.cmake)

list(APPEND TF_UNITTESTS
        test_utility
        test_queue
        test_work_stealing
        #test_serializer
        test_basics
        test_asyncs
        test_dependent_asyncs
        test_subflows
        test_control_flow
        test_semaphores
        test_movable
        test_cancellation
        test_for_each
        test_reduce
        test_transform
        test_sort
        test_scan
        test_find
        test_modules
        test_traversals
        test_runtimes
        test_workers
        test_pipelines
        test_scalable_pipelines
        test_data_pipelines
)

# we only do exception tests if sanitizer is not enabled
string(FIND '${CMAKE_CXX_FLAGS}' "-fsanitize" sanitize)
#message("sanitize='${sanitize}'")

if(${sanitize} EQUAL -1)
    message(STATUS "Add test_exception to unit tests")
    list(APPEND TF_UNITTESTS test_exceptions)
endif()


foreach(unittest IN LISTS TF_UNITTESTS)
    add_executable(${unittest} ${unittest}.cpp)
    target_link_libraries(${unittest} ${PROJECT_NAME} ${ATOMIC_LIBRARY}  ${KMCMAKE_SYSTEM_DYLINK})
    target_include_directories(${unittest} PRIVATE ${PROJECT_SOURCE_DIR}/tests/doctest)
    doctest_discover_tests(${unittest})
endforeach()


