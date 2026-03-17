// This program demonstrates how to use xf::DataPipeline to create
// a pipeline with in-pipe data automatically managed by the Taskflow
// library.

#include <xflow/taskflow.hpp>
#include <xflow/algorithm/data_pipeline.hpp>

int main() {
    // dataflow => void -> int -> std::string -> float -> void
    xf::Taskflow taskflow("pipeline");
    xf::Executor executor;

    const size_t num_lines = 3;

    // create a pipeline graph
    xf::DataPipeline pl(num_lines,
                        xf::make_data_pipe<void, int>(xf::PipeType::SERIAL, [&](xf::Pipeflow &pf) {
                            if (pf.token() == 5) {
                                pf.stop();
                                return 0;
                            } else {
                                printf("first pipe returns %zu\n", pf.token());
                                return static_cast<int>(pf.token());
                            }
                        }),

                        xf::make_data_pipe<int, std::string>(xf::PipeType::SERIAL, [](int &input) {
                            printf("second pipe returns a strong of %d\n", input + 100);
                            return std::to_string(input + 100);
                        }),

                        xf::make_data_pipe<std::string, void>(xf::PipeType::SERIAL, [](std::string &input) {
                            printf("third pipe receives the input string %s\n", input.c_str());
                        })
    );

    // build the pipeline graph using composition
    taskflow.composed_of(pl).name("pipeline");

    // dump the pipeline graph structure (with composition)
    taskflow.dump(std::cout);

    // run the pipeline
    executor.run(taskflow).wait();

    return 0;
}
