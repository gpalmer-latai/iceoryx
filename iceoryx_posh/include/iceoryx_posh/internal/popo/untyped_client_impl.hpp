// Copyright (c) 2022 by Apex.AI Inc. All rights reserved.
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

#ifndef IOX_POSH_POPO_UNTYPED_CLIENT_IMPL_HPP
#define IOX_POSH_POPO_UNTYPED_CLIENT_IMPL_HPP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/internal/popo/base_client.hpp"
#include "iceoryx_posh/popo/client_options.hpp"
#include "iceoryx_posh/popo/trigger_handle.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"

namespace iox
{
namespace popo
{
/// @brief The UntypedClientImpl class implements the untyped client API
/// @note Not intended for public usage! Use the 'UntypedClient' instead!
template <typename BaseClientT = BaseClient<>>
class UntypedClientImpl : public BaseClientT
{
  public:
    explicit UntypedClientImpl(const capro::ServiceDescription& service,
                               const ClientOptions& clientOptions = {}) noexcept;
    virtual ~UntypedClientImpl() noexcept;

    UntypedClientImpl(const UntypedClientImpl&) = delete;
    UntypedClientImpl(UntypedClientImpl&&) = delete;
    UntypedClientImpl& operator=(const UntypedClientImpl&) = delete;
    UntypedClientImpl& operator=(UntypedClientImpl&&) = delete;

    /// @brief Get a request chunk from loaned shared memory.
    /// @param payloadSize The expected payload size of the chunk.
    /// @param payloadAlignment The expected payload alignment of the chunk.
    /// @return An instance of the Request that resides in shared memory or an error if unable to allocate memory to
    /// loan.
    /// @details The loaned Request is automatically released when it goes out of scope.
    expected<Request<void>, AllocationError> loan(const uint64_t payloadSize, const uint32_t payloadAlignment) noexcept;

    /// @brief Sends the given Request and then releases its loan.
    /// @param request to send.
    /// @return Error if sending was not successful
    expected<void, ClientSendError> send(Request<void>&& request) noexcept;

    /// @brief Take the Response from the top of the receive queue.
    /// @return Either a Response or a ChunkReceiveResult.
    /// @details The Response takes care of the cleanup. Don't store the raw pointer to the content of the Response, but
    /// always the whole Response.
    expected<Response<const void>, ChunkReceiveResult> take() noexcept;

  protected:
    using BaseClientT::port;
};
} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/untyped_client_impl.inl"

#endif // IOX_POSH_POPO_UNTYPED_CLIENT_IMPL_HPP
