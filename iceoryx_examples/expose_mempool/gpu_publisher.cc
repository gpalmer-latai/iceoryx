#include "iceoryx_examples/expose_mempool/image.hh"

#include <iceoryx_dust/posix_wrapper/signal_watcher.hpp>
#include <iceoryx_posh/popo/publisher.hpp>
#include <iceoryx_posh/runtime/posh_runtime.hpp>
#include <iox/relative_pointer.hpp>

#include <iostream>

constexpr char APP_NAME[] = "gpu_publisher";

int main()
{
  iox::runtime::PoshRuntime::initRuntime(APP_NAME);

  iox::popo::Publisher<ImageData> publisher({"CudaPinned", "", "Image"});

  auto mem_pool_info = publisher.getMemPoolInfo();
  if (mem_pool_info.m_basePtr.get() == nullptr)
  {
    std::cerr << "Failed to get MemPoolInfo" << std::endl;
  }
  std::cout << "Used Chunks: " << mem_pool_info.m_usedChunks << std::endl;
  std::cout << "Min Free Chunks: " << mem_pool_info.m_minFreeChunks << std::endl;
  std::cout << "Num Chunks: " << mem_pool_info.m_numChunks << std::endl;
  std::cout << "Chunk Size: " << mem_pool_info.m_chunkSize << std::endl;
  std::cout << "Base Ptr: " << mem_pool_info.m_basePtr.get() << std::endl;

  // TODO: Call cudaHostRegister with pointer and size to allow GPU access to shared memory.
  // https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__MEMORY.html#group__CUDART__MEMORY_1ge8d5c17670f16ac4fc8fcb4181cb490c

  // TODO: Use GPU acceleration to populate the message, and then publish it.
  return 0;
}
