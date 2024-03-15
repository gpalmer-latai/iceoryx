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

#ifndef IOX_POSH_POPO_SERVER_IMPL_INL
#define IOX_POSH_POPO_SERVER_IMPL_INL

#include "iceoryx_posh/internal/popo/server_impl.hpp"

namespace iox
{
namespace popo
{
template <typename Req, typename Res, typename BaseServerT>
inline ServerImpl<Req, Res, BaseServerT>::ServerImpl(const capro::ServiceDescription& service,
                                                     const ServerOptions& serverOptions) noexcept
    : BaseServerT(service, serverOptions)
{
}

template <typename Req, typename Res, typename BaseServerT>
inline ServerImpl<Req, Res, BaseServerT>::~ServerImpl() noexcept
{
    BaseServerT::m_trigger.reset();
}

template <typename Req, typename Res, typename BaseServerT>
expected<Request<const Req>, ServerRequestResult> ServerImpl<Req, Res, BaseServerT>::take() noexcept
{
    auto maybeUsedChunk = port().getResponse();
    if (maybeUsedChunk.has_error())
    {
        return err(maybeUsedChunk.error());
    }
    auto& usedChunk = maybeUsedChunk.value();
    return ok(Request<const Req>{usedChunk, [this, usedChunk](Req*){
        this->port().releaseRequest(usedChunk);
    }});
}

template <typename Req, typename Res, typename BaseServerT>
expected<Response<Res>, AllocationError>
ServerImpl<Req, Res, BaseServerT>::loanUninitialized(const Request<const Req>& request) noexcept
{
    const auto* requestHeader = &request.getRequestHeader();
    auto maybeUsedChunk = port().allocateResponse(requestHeader, sizeof(Res), alignof(Res));
    if (maybeUsedChunk.has_error())
    {
        return err(maybeUsedChunk.error());
    }

    auto& usedChunk = maybeUsedChunk.value();
    return ok(Response<Req>{usedChunk, [this, usedChunk](Req*){
        this->port().releaseResponse(usedChunk);
    }, *this});
}

template <typename Req, typename Res, typename BaseServerT>
template <typename... Args>
expected<Response<Res>, AllocationError> ServerImpl<Req, Res, BaseServerT>::loan(const Request<const Req>& request,
                                                                                 Args&&... args) noexcept
{
    return std::move(loanUninitialized(request).and_then(
        [&](auto& response) { new (response.get()) Res(std::forward<Args>(args)...); }));
}

template <typename Req, typename Res, typename BaseServerT>
expected<void, ServerSendError> ServerImpl<Req, Res, BaseServerT>::send(Response<Res>&& response) noexcept
{
    // take the ownership of the chunk from the Request to transfer it to 'sendRequest'
    auto usedChunk = response.release();
    return port().sendResponse(usedChunk);
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_SERVER_IMPL_INL
