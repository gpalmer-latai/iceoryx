# Cuda Pinning Memory Pools Demo

1. In one window run: `bazel run //iceoryx_examples/expose_mempool:roudi_with_pinned_pool`
2. In the other run: `bazel run //iceoryx_examples/expose_mempool:gpu_publisher`
3. Observe that we print the information about the exact memory pool that is compatible with the image type.
