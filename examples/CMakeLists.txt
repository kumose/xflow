#
# Copyright 2023 The titan-search Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#



list(APPEND TF_EXAMPLES
        simple
        attach_data
        async
        async_module
        runtime_async
        dependent_async
        observer
        subflow
        fibonacci
        condition
        multi_condition
        switch_case
        do_while_loop
        while_loop
        for_loop
        if_else
        nested_if_else
        visualization
        parallel_for
        parallel_sort
        parallel_reduce
        inclusive_scan
        exclusive_scan
        pipeline
        scalable_pipeline
        text_pipeline
        taskflow_pipeline
        parallel_graph_pipeline
        parallel_data_pipeline
        run
        corun
        runtime
        composition
        limited_concurrency
        cancel
        exception
        subflow_exception
        worker_interface
        task_visitor
)

foreach(example IN LISTS TF_EXAMPLES)
    kmcmake_cc_binary(
            NAME ${example}
            SOURCES ${example}.cpp
            LINKS
            ${KMCMAKE_DEPS_LINK}
            CXXOPTS
            ${USER_CXX_FLAGS}
    )
endforeach()
