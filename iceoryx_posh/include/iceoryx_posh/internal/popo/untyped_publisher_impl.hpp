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

#ifndef IOX_POSH_POPO_UNTYPED_PUBLISHER_IMPL_HPP
#define IOX_POSH_POPO_UNTYPED_PUBLISHER_IMPL_HPP

#include "iceoryx_posh/internal/popo/base_publisher.hpp"
#include "iceoryx_posh/popo/sample.hpp"

namespace iox
{
namespace popo
{
/// @brief The UntypedPublisherImpl class implements the untyped publisher API
/// @note Not intended for public usage! Use the 'UntypedPublisher' instead!
template <typename BasePublisherType = BasePublisher<>>
class UntypedPublisherImpl : public BasePublisherType
{
  public:
    explicit UntypedPublisherImpl(const capro::ServiceDescription& service,
                                  const PublisherOptions& publisherOptions = PublisherOptions());

    virtual ~UntypedPublisherImpl() = default;

    UntypedPublisherImpl(const UntypedPublisherImpl& other) = delete;
    UntypedPublisherImpl& operator=(const UntypedPublisherImpl&) = delete;
    UntypedPublisherImpl(UntypedPublisherImpl&& rhs) noexcept = delete;
    UntypedPublisherImpl& operator=(UntypedPublisherImpl&& rhs) noexcept = delete;

    ///
    /// @brief Get a chunk from loaned shared memory.
    /// @param usePayloadSize The expected user-payload size of the chunk.
    /// @param userPayloadAlignment The expected user-payload alignment of the chunk.
    /// @return An instance of the sample that resides in shared memory or an error if unable ot allocate memory to
    /// loan.
    /// @details The loaned sample is automatically released when it goes out of scope.
    /// @note An AllocationError occurs if no chunk is available in the shared memory.
    expected<Sample<void>, AllocationError>
    loan(const uint64_t userPayloadSize,
         const uint32_t userPayloadAlignment = iox::CHUNK_DEFAULT_USER_PAYLOAD_ALIGNMENT,
         const uint32_t userHeaderSize = iox::CHUNK_NO_USER_HEADER_SIZE,
         const uint32_t userHeaderAlignment = iox::CHUNK_NO_USER_HEADER_ALIGNMENT) noexcept;

    ///
    /// @brief publish Publishes the given sample and then releases its loan.
    /// @param sample The sample to publish.
    /// @note Ownership of the userPayload is transferred to this function. 
    ///       Accessing the underlying pointer after publication is UB.
    ///
    void publish(Sample<void>&& sample) noexcept;

  protected:
    using PortType = typename BasePublisherType::PortType;
    using BasePublisherType::port;

    UntypedPublisherImpl(PortType&& port) noexcept;
};

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/untyped_publisher_impl.inl"

#endif // IOX_POSH_POPO_UNTYPED_PUBLISHER_IMPL_HPP
