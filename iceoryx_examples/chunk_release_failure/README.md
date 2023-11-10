# Subscriber Chunk Release Failure

This example serves to expose a failure observed when reserving at least 4GiB of shared memory chunks and then attempting to free a chunk. 

The basic setup is as follows:
* There is a message of size 128 MiB in `topic_data.hpp` called `LargeData`.
* A simple publisher publishes the message every 10 milliseconds.
* A subscriber receives the message and moves it into a circular buffer with exactly 4GiB worth of message samples.
** Once the queue fills and an additional chunk is allocated and then pushed into the queue, the evicted chunk will fail to be released.
** If instead we reduce the circular buffer size by 1, the evicted chunk will be release without any issue.

## Running the example

### To demonstrate the failure

1. `bazel run -- //:roudi -c <PATH_TO_REPO>/iceoryx_examples/chunk_release_failure/roudi_config.toml`
2. `bazel run //iceoryx_examples/chunk_release_failure:subscriber`
3. `bazel run //iceoryx_examples/chunk_release_failure:publisher`
4. Observe the subscriber. Once it attempts to enqueue the 33rd sample you should see: 
```
2023-10-09 11:57:51.422 [Warn ]: ICEORYX error! POPO__CHUNK_RECEIVER_INVALID_CHUNK_TO_RELEASE_FROM_USER
subscriber: iceoryx_hoofs/source/error_handling/error_handler.cpp:45: static void iox::ErrorHandler::reactOnErrorLevel(iox::ErrorLevel, const char*): Assertion `false' failed.
```
and the process will crash.

### To demonstrate no failure when reducing chunk allocation below 4GiB

Change `FAILING_QUEUE_SIZE` in `subscriber.cpp:29` to `PASSING_QUEUE_SIZE` and run the example the same way.

## Theories about the cause of failure

The fact that the failure occurs when releasing a chunk at exactly the 4GiB mark screams uint32_t overflow, as 
4GiB corresponds to the max value of uint32_t in bytes. I suspect that some pointer arithmetic somewhere gets downcasted to a uint32_t and causes an invalid address to be accessed.

Adding to this suspicion is an error message I receive on the publisher side:
```
2023-11-10 18:22:05.900 [Error]: Condition: offset % m_chunkSize == 0 in void iox::mepoo::MemPool::freeChunk(const void*) is violated. (iceoryx_posh/source/mepoo/mem_pool.cpp:109)
2023-11-10 18:22:05.900 [Error]: ICEORYX error! EXPECTS_ENSURES_FAILED
publisher: iceoryx_hoofs/source/error_handling/error_handler.cpp:40: static void iox::ErrorHandler::reactOnErrorLevel(iox::ErrorLevel, const char*): Assertion `false' failed.
```

