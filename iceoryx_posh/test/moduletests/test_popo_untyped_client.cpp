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

#include "iceoryx_posh/popo/untyped_client.hpp"
#include "iceoryx_posh/testing/mocks/chunk_mock.hpp"

#if __has_include("mocks/client_mock.hpp")
#include "mocks/client_mock.hpp"
#else
#include "iceoryx_posh/test/mocks/client_mock.hpp"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
using namespace ::testing;
using namespace iox::capro;
using namespace iox::popo;
using ::testing::_;

using TestUntypedClient = iox::popo::UntypedClientImpl<MockBaseClient>;

class UntypedClient_test : public Test
{
  public:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

  protected:
    ChunkMock<uint64_t, RequestHeader> requestMock;
    ChunkMock<uint64_t, ResponseHeader> responseMock;

    ServiceDescription sd{"oh", "captain", "my captain"};
    static constexpr uint64_t RESPONSE_QUEUE_CAPACITY{123U};
    ClientOptions options{RESPONSE_QUEUE_CAPACITY};
    TestUntypedClient sut{sd, options};
};

TEST_F(UntypedClient_test, ConstructorForwardsArgumentsToBaseClient)
{
    ::testing::Test::RecordProperty("TEST_ID", "9df896dd-90b2-4b8c-bf14-71c7ec9a467e");

    EXPECT_THAT(sut.serviceDescription, Eq(sd));
    EXPECT_THAT(sut.clientOptions, Eq(options));
}

TEST_F(UntypedClient_test, LoanCallsUnderlyingPortWithSuccessResult)
{
    ::testing::Test::RecordProperty("TEST_ID", "acb900fd-288f-4ef3-96b9-6843cd869893");

    constexpr uint64_t PAYLOAD_SIZE{8U};
    constexpr uint32_t PAYLOAD_ALIGNMENT{32U};
    const iox::expected<UsedChunk, AllocationError> allocateRequestResult =
        iox::ok<UsedChunk>(UsedChunk{requestMock.chunkHeader(), 0U});

    EXPECT_CALL(sut.mockPort, allocateRequest(PAYLOAD_SIZE, PAYLOAD_ALIGNMENT)).WillOnce(Return(allocateRequestResult));

    auto maybeLoanSample = sut.loan(PAYLOAD_SIZE, PAYLOAD_ALIGNMENT);
    ASSERT_FALSE(maybeLoanSample.has_error());
    EXPECT_THAT(maybeLoanSample->get(), Eq(requestMock.sample()));
}

TEST_F(UntypedClient_test, LoanCallsUnderlyingPortWithErrorResult)
{
    ::testing::Test::RecordProperty("TEST_ID", "905d8a67-1fde-4960-a95d-51c5ca4b6ed9");

    constexpr uint64_t PAYLOAD_SIZE{8U};
    constexpr uint32_t PAYLOAD_ALIGNMENT{32U};
    constexpr AllocationError ALLOCATION_ERROR{AllocationError::RUNNING_OUT_OF_CHUNKS};
    const iox::expected<UsedChunk, AllocationError> allocateRequestResult = iox::err(ALLOCATION_ERROR);

    EXPECT_CALL(sut.mockPort, allocateRequest(PAYLOAD_SIZE, PAYLOAD_ALIGNMENT)).WillOnce(Return(allocateRequestResult));

    auto loanResult = sut.loan(PAYLOAD_SIZE, PAYLOAD_ALIGNMENT);
    ASSERT_TRUE(loanResult.has_error());
    EXPECT_THAT(loanResult.error(), Eq(ALLOCATION_ERROR));
}

TEST_F(UntypedClient_test, ReleaseRequestWithValidPayloadPointerCallsUnderlyingPort)
{
    ::testing::Test::RecordProperty("TEST_ID", "8c235e87-b790-4a21-abba-19a279d76431");

    constexpr uint64_t PAYLOAD_SIZE{8U};
    constexpr uint32_t PAYLOAD_ALIGNMENT{32U};
    auto usedChunk = UsedChunk{requestMock.chunkHeader(), 0U};
    const iox::expected<UsedChunk, AllocationError> allocateRequestResult =
        iox::ok<UsedChunk>(usedChunk);

    EXPECT_CALL(sut.mockPort, allocateRequest(PAYLOAD_SIZE, PAYLOAD_ALIGNMENT)).WillOnce(Return(allocateRequestResult));

    auto maybeLoanSample = sut.loan(PAYLOAD_SIZE, PAYLOAD_ALIGNMENT);
    ASSERT_FALSE(maybeLoanSample.has_error());

    EXPECT_CALL(sut.mockPort, releaseRequest(usedChunk)).Times(1);
    // Request released implicitly by maybeLoanSample destructor.
}

TEST_F(UntypedClient_test, SendWithValidPayloadPointerCallsUnderlyingPort)
{
    ::testing::Test::RecordProperty("TEST_ID", "74d86b31-24a8-409e-8b85-7b9ec1c7ad3d");

    constexpr uint64_t PAYLOAD_SIZE{8U};
    constexpr uint32_t PAYLOAD_ALIGNMENT{32U};
    auto usedChunk = UsedChunk{requestMock.chunkHeader(), 0U};
    const iox::expected<UsedChunk, AllocationError> allocateRequestResult =
        iox::ok<UsedChunk>(usedChunk);

    EXPECT_CALL(sut.mockPort, allocateRequest(PAYLOAD_SIZE, PAYLOAD_ALIGNMENT)).WillOnce(Return(allocateRequestResult));

    auto maybeLoanSample = sut.loan(PAYLOAD_SIZE, PAYLOAD_ALIGNMENT);
    ASSERT_FALSE(maybeLoanSample.has_error());

    EXPECT_CALL(sut.mockPort, sendRequest(usedChunk)).WillOnce(Return(iox::ok()));
    EXPECT_CALL(sut.mockPort, releaseRequest(_)).Times(0);

    maybeLoanSample->send()
        .and_then([&]() { GTEST_SUCCEED() << "Request successfully sent"; })
        .or_else([&](auto error) { GTEST_FAIL() << "Expected request to be sent but got error: " << error; });
}

TEST_F(UntypedClient_test, TakeCallsUnderlyingPortWithSuccessResult)
{
    ::testing::Test::RecordProperty("TEST_ID", "9ca260e9-89bb-48aa-8504-0375e35eef9f");

    auto usedChunk = UsedChunk{responseMock.chunkHeader(), 0U};
    const iox::expected<UsedChunk, ChunkReceiveResult> getResponseResult =
        iox::ok<UsedChunk>(usedChunk);

    EXPECT_CALL(sut.mockPort, getResponse()).WillOnce(Return(getResponseResult));

    auto takeResult = sut.take();
    ASSERT_FALSE(takeResult.has_error());
    EXPECT_THAT(takeResult->get(), Eq(responseMock.sample()));
}

TEST_F(UntypedClient_test, TakeCallsUnderlyingPortWithErrorResult)
{
    ::testing::Test::RecordProperty("TEST_ID", "ff524011-3a79-4960-9379-571e2eb87b16");

    constexpr ChunkReceiveResult CHUNK_RECEIVE_RESULT{ChunkReceiveResult::TOO_MANY_CHUNKS_HELD_IN_PARALLEL};
    const iox::expected<UsedChunk, ChunkReceiveResult> getResponseResult = iox::err(CHUNK_RECEIVE_RESULT);

    EXPECT_CALL(sut.mockPort, getResponse()).WillOnce(Return(getResponseResult));

    auto takeResult = sut.take();
    ASSERT_TRUE(takeResult.has_error());
    EXPECT_THAT(takeResult.error(), Eq(CHUNK_RECEIVE_RESULT));
}

TEST_F(UntypedClient_test, ReleaseResponseWithValidPayloadPointerCallsUnderlyingPort)
{
    ::testing::Test::RecordProperty("TEST_ID", "7fd4433f-5894-430a-9c06-60ff39572920");

    auto usedChunk = UsedChunk{responseMock.chunkHeader(), 0U};
    const iox::expected<UsedChunk, ChunkReceiveResult> getResponseResult =
        iox::ok<UsedChunk>(usedChunk);

    EXPECT_CALL(sut.mockPort, getResponse()).WillOnce(Return(getResponseResult));

    auto takeResult = sut.take();
    ASSERT_FALSE(takeResult.has_error());

    EXPECT_CALL(sut.mockPort, releaseResponse(usedChunk)).Times(1);
    // Response released implicitly by takeResult destructor.
}

} // namespace
