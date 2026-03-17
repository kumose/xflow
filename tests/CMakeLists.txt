# Copyright (C) Kumo inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
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


