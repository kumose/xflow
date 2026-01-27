#include <xflow/taskflow.hpp>
#include <xflow/cuda/cudaflow.hpp>

int main() {

  // CUDA version
  std::cout << "========================================\n"
            << "CUDA version: " 
            << xf::cuda_get_runtime_version() << '\n'
            << "CUDA driver version: " 
            << xf::cuda_get_driver_version() << '\n';

  // Number of CUDA devices
  auto num_cuda_devices = xf::cuda_get_num_devices();

  std::cout << "There are " << num_cuda_devices << " CUDA devices.\n";

  // Iterate each device and dump its property
  std::cout << "\nquerying device properties ...\n";
  for(size_t i = 0; i < num_cuda_devices; ++i) {
    std::cout << "CUDA device #" << i << '\n';
    xf::cuda_dump_device_property(std::cout, xf::cuda_get_device_property(i));
  }

  // we can also query each device property attribute by attribute
  std::cout << "\nquerying device attributes ...\n";
  for(size_t i = 0; i< num_cuda_devices; ++i) {
    std::cout << "CUDA device #" << i << '\n';
    std::cout << "Compute capability   : "
              << xf::cuda_get_device_compute_capability_major(i) << '.'
              << xf::cuda_get_device_compute_capability_minor(i) << '\n';
    std::cout << "max threads per block: "
              << xf::cuda_get_device_max_threads_per_block(i) << '\n'
              << "max x-dim   per block: "
              << xf::cuda_get_device_max_x_dim_per_block(i) << '\n'
              << "max y-dim   per block: "
              << xf::cuda_get_device_max_y_dim_per_block(i) << '\n'
              << "max z-dim   per block: "
              << xf::cuda_get_device_max_z_dim_per_block(i) << '\n'
              << "max x-dim   per grid : "
              << xf::cuda_get_device_max_x_dim_per_grid(i) << '\n'
              << "max y-dim   per grid : "
              << xf::cuda_get_device_max_y_dim_per_grid(i) << '\n'
              << "max z-dim   per grid : "
              << xf::cuda_get_device_max_z_dim_per_grid(i) << '\n'
              << "warp size            : "
              << xf::cuda_get_device_warp_size(i) << '\n'
              << "unified addressing?  : "
              << xf::cuda_get_device_unified_addressing(i) << '\n';
  }

  return 0;
}



