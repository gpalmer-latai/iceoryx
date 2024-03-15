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

#ifndef IOX_POSH_POPO_UNTYPED_SERVER_IMPL_HPP
#define IOX_POSH_POPO_UNTYPED_SERVER_IMPL_HPP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/internal/popo/base_server.hpp"
#include "iceoryx_posh/popo/server_options.hpp"

namespace iox
{
namespace popo
{
/// @brief The UntypedServerImpl class implements the untyped server API
/// @note Not intended for public usage! Use the 'UntypedServer' instead!
template <typename BaseServerT = BaseServer<>>
class UntypedServerImpl : public BaseServerT
{
  public:
    explicit UntypedServerImpl(const capro::ServiceDescription& service,
                               const ServerOptions& serverOptions = {}) noexcept;
    virtual ~UntypedServerImpl() noexcept;

    UntypedServerImpl(const UntypedServerImpl&) = delete;
    UntypedServerImpl(UntypedServerImpl&&) = delete;
    UntypedServerImpl& operator=(const UntypedServerImpl&) = delete;
    UntypedServerImpl& operator=(UntypedServerImpl&&) = delete;

    /// @brief Take the Request from the top of the receive queue.
    /// @return Either a Request or a ServerRequestResult.
    /// @details The Request takes care of the cleanup. Don't store the raw pointer to the content of the Request, but
    /// always the whole Request.
    expected<Request<const void>, ServerRequestResult> take() noexcept;

    /// @brief Get a response chunk from loaned shared memory.
    /// @param[in] request The request to which the Response belongs to, to determine where to send the response
    /// @param payloadSize The expected payload size of the chunk.
    /// @param payloadAlignment The expected payload alignment of the chunk.
    /// @return An instance of the Response that resides in shared memory or an error if unable to allocate memory to
    /// loan.
    /// @details The loaned Response is automatically released when it goes out of scope.
    expected<Response<void>, AllocationError> loan(const Request<const void>& request,
                                          const uint64_t payloadSize,
                                          const uint32_t payloadAlignment) noexcept;

    /// @brief Sends the given Response and then releases its loan.
    /// @param response to send.
    /// @return Error if sending was not successful
    expected<void, ServerSendError> send(Response<void>&& response) noexcept;

  protected:
    using BaseServerT::port;
};
} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/untyped_server_impl.inl"

#endif // IOX_POSH_POPO_UNTYPED_SERVER_IMPL_HPP
