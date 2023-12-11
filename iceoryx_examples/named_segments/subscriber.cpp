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

//! [include]
#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iox/signal_watcher.hpp"
#include "iox/string.hpp"
//! [include]

#include <iostream>

using StringPayload = iox::string<400>;

iox::popo::Subscriber<StringPayload> create_subscriber(const iox::ShmName_t& segmentName)
{
    iox::popo::SubscriberOptions options;
    options.shmName = segmentName;
    return iox::popo::Subscriber<StringPayload>({"Examples", "NamedSegment", "StringPayload"}, options);
}

int main()
{
    //! [initialize runtime]
    constexpr char APP_NAME[] = "iox-cpp-subscriber-named-segments";
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);
    //! [initialize runtime]

    //! [initialize subscriber]
    // Change shm name to match against a different publisher.
    auto subscriber = create_subscriber("foo");
    // auto subscriber = create_subscriber("bar");
    // auto subscriber = create_subscriber("");
    //! [initialize subscriber]

    // run until interrupted by Ctrl-C
    while (!iox::hasTerminationRequested())
    {
        //! [receive]
        subscriber.take().and_then([&APP_NAME](const auto& sample) {
            std::cout << APP_NAME << " got message: " << *sample << std::endl;
        });

        //! [wait]
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //! [wait]
    }

    return (EXIT_SUCCESS);
}
