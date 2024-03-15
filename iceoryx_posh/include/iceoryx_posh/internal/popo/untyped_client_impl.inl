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

#ifndef IOX_POSH_POPO_UNTYPED_CLIENT_IMPL_INL
#define IOX_POSH_POPO_UNTYPED_CLIENT_IMPL_INL

#include "iceoryx_posh/internal/popo/untyped_client_impl.hpp"

namespace iox
{
namespace popo
{
template <typename BaseClientT>
UntypedClientImpl<BaseClientT>::UntypedClientImpl(const capro::ServiceDescription& service,
                                                  const ClientOptions& clientOptions) noexcept
    : BaseClientT(service, clientOptions)
{
}

template <typename BaseClientT>
UntypedClientImpl<BaseClientT>::~UntypedClientImpl() noexcept
{
    BaseClientT::m_trigger.reset();
}

template <typename BaseClientT>
expected<Request<void>, AllocationError> UntypedClientImpl<BaseClientT>::loan(const uint64_t payloadSize,
                                                                              const uint32_t payloadAlignment) noexcept
{
    auto maybeUsedChunk = port().allocateRequest(payloadSize, payloadAlignment);
    if (maybeUsedChunk.has_error())
    {
        return err(maybeUsedChunk.error());
    }
    auto& usedChunk = maybeUsedChunk.value();
    return ok(Request<void>{usedChunk, [this, usedChunk](void*){
        this->port().releaseRequest(usedChunk);
    }, *this});
}

template <typename BaseClientT>
expected<void, ClientSendError> UntypedClientImpl<BaseClientT>::send(Request<void>&& request) noexcept
{
    // take the ownership of the chunk from the Request to transfer it to 'sendRequest'
    auto usedChunk = request.release();
    return port().sendRequest(usedChunk);
}

template <typename BaseClientT>
expected<Response<const void>, ChunkReceiveResult> UntypedClientImpl<BaseClientT>::take() noexcept
{
    auto maybeUsedChunk = port().getResponse();
    if (maybeUsedChunk.has_error())
    {
        return err(maybeUsedChunk.error());
    }
    auto& usedChunk = maybeUsedChunk.value();
    return ok(Response<const void>{usedChunk, [this, usedChunk](void*){
        this->port().releaseResponse(usedChunk);
    }});
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_UNTYPED_CLIENT_IMPL_INL
