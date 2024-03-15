// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2022 by Apex.AI Inc. All rights reserved.
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

#ifndef IOX_POSH_POPO_UNTYPED_SUBSCRIBER_IMPL_HPP
#define IOX_POSH_POPO_UNTYPED_SUBSCRIBER_IMPL_HPP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/base_subscriber.hpp"
#include "iox/expected.hpp"
#include "iox/unique_ptr.hpp"

namespace iox
{
namespace popo
{
class Void
{
};

/// @brief The UntypedSubscriberImpl class implements the untyped subscriber API
/// @note Not intended for public usage! Use the 'UntypedSubscriber' instead!
template <typename BaseSubscriberType = BaseSubscriber<>>
class UntypedSubscriberImpl : public BaseSubscriberType
{
  public:
    using BaseSubscriber = BaseSubscriberType;
    using SelfType = UntypedSubscriberImpl<BaseSubscriberType>;

    explicit UntypedSubscriberImpl(const capro::ServiceDescription& service,
                                   const SubscriberOptions& subscriberOptions = SubscriberOptions());
    UntypedSubscriberImpl(const UntypedSubscriberImpl& other) = delete;
    UntypedSubscriberImpl& operator=(const UntypedSubscriberImpl&) = delete;
    UntypedSubscriberImpl(UntypedSubscriberImpl&& rhs) = delete;
    UntypedSubscriberImpl& operator=(UntypedSubscriberImpl&& rhs) = delete;
    virtual ~UntypedSubscriberImpl() noexcept;

    ///
    /// @brief Take the chunk from the top of the receive queue.
    /// @return Either a sample or a ChunkReceiveResult
    /// @details The sample takes care of the cleanup. Don't store the raw pointer to the content of the sample, but
    /// always the whole sample.
    ///
    expected<Sample<void>, ChunkReceiveResult> take() noexcept;

  protected:
    using PortType = typename BaseSubscriberType::PortType;
    using BaseSubscriber::port;

    UntypedSubscriberImpl(PortType&& port) noexcept;
};

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/untyped_subscriber_impl.inl"

#endif // IOX_POSH_POPO_UNTYPED_SUBSCRIBER_IMPL_HPP
