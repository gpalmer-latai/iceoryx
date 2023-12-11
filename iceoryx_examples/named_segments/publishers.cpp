// Copyright (c) 2023 by Latitude AI All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

//! [include sig watcher]
#include "iox/signal_watcher.hpp"
//! [include sig watcher]

//! [include]
#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iox/string.hpp"
//! [include]

#include <iostream>


using StringPayload = iox::string<400>;

iox::popo::Publisher<StringPayload> create_publisher(const iox::ShmName_t& segmentName)
{
    iox::popo::PublisherOptions options;
    options.shmName = segmentName;
    return iox::popo::Publisher<StringPayload>({"Examples", "NamedSegment", "StringPayload"}, options);
}

int main()
{
    //! [initialize runtime]
    constexpr char APP_NAME[] = "iox-cpp-publisher-named-segments";
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);
    //! [initialize runtime]

    //! [create publishers]
    auto foo_publisher = create_publisher("foo");
    auto bar_publisher = create_publisher("bar");
    auto default_publisher = create_publisher("");
    //! [create publishers]

    uint64_t ct = 0UL;
    //! [wait for term]
    while (!iox::hasTerminationRequested())
    //! [wait for term]
    {
        ++ct;

        // Publish a message unique to each publisher.
        foo_publisher.publishCopyOf("Hello from the Foo Publisher!").expect("failed to publish bar");
        bar_publisher.publishCopyOf("Hello from the Bar Publisher!").expect("failed to publish foo");
        default_publisher.publishCopyOf("Hello from the Default Publisher!").expect("failed to publish default");

        std::cout << APP_NAME << " Published round " << ct << " of hello messages" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
