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

#ifndef IOX_POSH_POPO_UNTYPED_SERVER_IMPL_INL
#define IOX_POSH_POPO_UNTYPED_SERVER_IMPL_INL

#include "iceoryx_posh/internal/popo/untyped_server_impl.hpp"

namespace iox
{
namespace popo
{
template <typename BaseServerT>
UntypedServerImpl<BaseServerT>::UntypedServerImpl(const capro::ServiceDescription& service,
                                                  const ServerOptions& serverOptions) noexcept
    : BaseServerT(service, serverOptions)
{
}

template <typename BaseServerT>
UntypedServerImpl<BaseServerT>::~UntypedServerImpl() noexcept
{
    BaseServerT::m_trigger.reset();
}

template <typename BaseServerT>
expected<Request<const void>, ServerRequestResult> UntypedServerImpl<BaseServerT>::take() noexcept
{
    auto maybeUsedChunk = port().getRequest();
    if (maybeUsedChunk.has_error())
    {
        return err(maybeUsedChunk.error());
    }
    auto& usedChunk = maybeUsedChunk.value();
    return ok(Request<const void>{usedChunk, [this, usedChunk](const void*){
        this->port().releaseRequest(usedChunk);
    }});
}

template <typename BaseServerT>
expected<Response<void>, AllocationError> UntypedServerImpl<BaseServerT>::loan(const Request<const void>& request,
                                                                      const uint64_t payloadSize,
                                                                      const uint32_t payloadAlignment) noexcept
{
    const auto* requestHeader = &request.getRequestHeader();
    auto maybeUsedChunk = port().allocateResponse(requestHeader, payloadSize, payloadAlignment);
    if (maybeUsedChunk.has_error())
    {
        return err(maybeUsedChunk.error());
    }

    auto& usedChunk = maybeUsedChunk.value();
    return ok(Response<void>{usedChunk, [this, usedChunk](void*){
        this->port().releaseResponse(usedChunk);
    }, *this});
}

template <typename BaseServerT>
expected<void, ServerSendError> UntypedServerImpl<BaseServerT>::send(Response<void>&& response) noexcept
{
    // take the ownership of the chunk from the Request to transfer it to 'sendRequest'
    auto usedChunk = response.release();
    return port().sendResponse(usedChunk);
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_UNTYPED_SERVER_IMPL_INL
