#include "topic_data.hpp"

#include "iceoryx_dust/posix_wrapper/signal_watcher.hpp"
#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"

#include <chrono>
#include <iostream>

using LargeDataPublisher = iox::popo::Publisher<LargeData>;

constexpr char APP_NAME[] = "large-data-publisher";

void publish_message(LargeDataPublisher& publisher)
{
  publisher.loan()
    .and_then([&](auto& sample) {
      sample.publish();
    })
    .or_else([](auto& error) { std::cerr << "Unable to loan sample, error: " << error << std::endl; });
}

int main()
{
  iox::runtime::PoshRuntime::initRuntime(APP_NAME);

  iox::popo::PublisherOptions publisherOptions;
  publisherOptions.subscriberTooSlowPolicy = iox::popo::ConsumerTooSlowPolicy::DISCARD_OLDEST_DATA;
  LargeDataPublisher publisher({"", "", "LargeData"}, publisherOptions);

  while (!iox::posix::hasTerminationRequested())
  {
    publish_message(publisher);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return 0;
}
