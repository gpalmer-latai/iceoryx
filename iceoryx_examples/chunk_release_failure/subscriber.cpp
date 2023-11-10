#include "topic_data.hpp"

#include "iceoryx_dust/posix_wrapper/signal_watcher.hpp"
#include "iceoryx_posh/popo/listener.hpp"
#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_hoofs/concurrent/lockfree_queue.hpp"

#include <iostream>
#include <memory>

using LargeDataSubscriber = iox::popo::Subscriber<LargeData>;
using LargeDataSample = iox::popo::Sample<const LargeData>;

constexpr char APP_NAME[] = "large-data-subscriber";
constexpr size_t FOUR_GiB = 1024UL * 1024 * 1024 * 4;
constexpr size_t LARGE_DATA_SIZE = 1024 * 1024 * 128;
constexpr size_t FAILING_QUEUE_SIZE = FOUR_GiB / LARGE_DATA_SIZE;
constexpr size_t PASSING_QUEUE_SIZE = FAILING_QUEUE_SIZE - 1;

void process_message(LargeDataSubscriber* largeDataSubscriber)
{
  largeDataSubscriber->take()
    .and_then([](LargeDataSample& sample) {

        static iox::concurrent::LockFreeQueue<
            std::unique_ptr<LargeDataSample>, FAILING_QUEUE_SIZE> chunkHoggingQueue;

        static size_t counter{0};
        std::cout << "Enqueuing message " << counter++ << std::endl;

        chunkHoggingQueue.push(std::make_unique<LargeDataSample>(std::move(sample)));
    })
    .or_else([](auto& result) {
      if (result != iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE)
      {
        std::cout << "Error receiving chunk." << std::endl;
      }
    });
}

int main()
{
  iox::runtime::PoshRuntime::initRuntime(APP_NAME);

  iox::popo::SubscriberOptions subscriberOptions;
  subscriberOptions.queueCapacity = 11U;
  subscriberOptions.queueFullPolicy = iox::popo::QueueFullPolicy::DISCARD_OLDEST_DATA;
  LargeDataSubscriber largeDataSubscriber({"", "", "LargeData"}, subscriberOptions);

  iox::popo::Listener listener{};

  listener.attachEvent(
    largeDataSubscriber,
    iox::popo::SubscriberEvent::DATA_RECEIVED,
    iox::popo::createNotificationCallback(process_message));

  iox::posix::waitForTerminationRequest();

  listener.detachEvent(largeDataSubscriber, iox::popo::SubscriberEvent::DATA_RECEIVED);

  return (EXIT_SUCCESS);
}
