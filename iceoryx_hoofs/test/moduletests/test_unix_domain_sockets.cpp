// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_hoofs/internal/posix_wrapper/message_queue.hpp"
#include "iceoryx_hoofs/internal/posix_wrapper/unix_domain_socket.hpp"
#include "iceoryx_hoofs/platform/socket.hpp"
#include "iceoryx_hoofs/posix_wrapper/posix_call.hpp"

#include "test.hpp"

#include <atomic>
#include <chrono>
#include <thread>

namespace
{
using namespace ::testing;
using namespace ::testing::internal;
using namespace iox;
using namespace iox::posix;
using namespace iox::units::duration_literals;
using namespace iox::units;

using sendCall_t = std::function<cxx::expected<IpcChannelError>(const std::string&)>;
using receiveCall_t = std::function<cxx::expected<std::string, IpcChannelError>()>;

constexpr char goodName[] = "channel_test";

/// @req
/// @brief This test suite verifies the functionality which is specific to the UnixDomainSocket class
/// @pre server and client object are allocated and move to the member object of the fixture
/// @post StdErr is capture and outputed to StdCout
/// @note Most of the UnixDomainSocket functionality is tested in "IpcChannel_test"
class UnixDomainSocket_test : public Test
{
  public:
    void SetUp()
    {
        auto serverResult = UnixDomainSocket::create(
            goodName, IpcChannelSide::SERVER, UnixDomainSocket::MAX_MESSAGE_SIZE, MaxMsgNumber);
        ASSERT_THAT(serverResult.has_error(), Eq(false));
        server = std::move(serverResult.value());
        CaptureStderr();

        auto clientResult = UnixDomainSocket::create(
            goodName, IpcChannelSide::CLIENT, UnixDomainSocket::MAX_MESSAGE_SIZE, MaxMsgNumber);
        ASSERT_THAT(clientResult.has_error(), Eq(false));
        client = std::move(clientResult.value());
    }

    void TearDown()
    {
        std::string output = GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }

    ~UnixDomainSocket_test()
    {
    }

    void createTestSocket(const UnixDomainSocket::UdsName_t& name)
    {
        static constexpr int32_t ERROR_CODE = -1;
        struct sockaddr_un sockAddr;
        int32_t sockfd;

        memset(&sockAddr, 0, sizeof(sockAddr));
        sockAddr.sun_family = AF_LOCAL;
        strncpy(sockAddr.sun_path, name.c_str(), name.size());

        iox::posix::posixCall(iox_socket)(AF_LOCAL, SOCK_DGRAM, 0)
            .failureReturnValue(ERROR_CODE)
            .evaluate()
            .and_then([&](auto& r) { sockfd = r.value; })
            .or_else([](auto&) {
                std::cerr << "unable to create socket\n";
                std::terminate();
            });

        iox::posix::posixCall(iox_bind)(sockfd, reinterpret_cast<struct sockaddr*>(&sockAddr), sizeof(sockAddr))
            .failureReturnValue(ERROR_CODE)
            .evaluate()
            .or_else([](auto&) {
                std::cerr << "unable to bind socket\n";
                std::terminate();
            });
    }

    void signalThreadReady()
    {
        doWaitForThread.store(false, std::memory_order_relaxed);
    }

    void waitForThread()
    {
        while (doWaitForThread.load(std::memory_order_relaxed))
        {
            std::this_thread::yield();
        }
    }

    const std::chrono::milliseconds WAIT_IN_MS{10};
    std::atomic_bool doWaitForThread{true};
    static constexpr uint64_t MaxMsgNumber = 10U;
    UnixDomainSocket server;
    UnixDomainSocket client;
};

constexpr uint64_t UnixDomainSocket_test::MaxMsgNumber;

TEST_F(UnixDomainSocket_test, UnlinkEmptySocketNameLeadsToInvalidChannelNameError)
{
    auto ret = UnixDomainSocket::unlinkIfExists(UnixDomainSocket::NoPathPrefix, "");
    ASSERT_TRUE(ret.has_error());
    EXPECT_THAT(ret.get_error(), Eq(IpcChannelError::INVALID_CHANNEL_NAME));
}

TEST_F(UnixDomainSocket_test, UnlinkEmptySocketNameWithPathPrefixLeadsToInvalidChannelNameError)
{
    auto ret = UnixDomainSocket::unlinkIfExists("");
    ASSERT_TRUE(ret.has_error());
    EXPECT_THAT(ret.get_error(), Eq(IpcChannelError::INVALID_CHANNEL_NAME));
}

TEST_F(UnixDomainSocket_test, UnlinkTooLongSocketNameWithPathPrefixLeadsToInvalidChannelNameError)
{
    UnixDomainSocket::UdsName_t longSocketName;
    for (uint64_t i = 0U; i < UnixDomainSocket::LONGEST_VALID_NAME - strlen(UnixDomainSocket::PATH_PREFIX) + 1; ++i)
    {
        longSocketName.append(cxx::TruncateToCapacity, "o");
    }
    auto ret = UnixDomainSocket::unlinkIfExists(longSocketName);
    ASSERT_TRUE(ret.has_error());
    EXPECT_THAT(ret.get_error(), Eq(IpcChannelError::INVALID_CHANNEL_NAME));
}

TEST_F(UnixDomainSocket_test, UnlinkExistingSocketLeadsIsSuccessful)
{
    UnixDomainSocket::UdsName_t socketFileName = UnixDomainSocket::PATH_PREFIX;
    socketFileName.append(cxx::TruncateToCapacity, "iceoryx-hoofs-moduletest.socket");
    createTestSocket(socketFileName);
    auto ret = UnixDomainSocket::unlinkIfExists(UnixDomainSocket::NoPathPrefix, socketFileName);
    EXPECT_FALSE(ret.has_error());
}

TEST_F(UnixDomainSocket_test, UnlinkExistingSocketWithPathPrefixLeadsIsSuccessful)
{
    UnixDomainSocket::UdsName_t socketFileName = "iceoryx-hoofs-moduletest.socket";
    UnixDomainSocket::UdsName_t socketFileNameWithPrefix = UnixDomainSocket::PATH_PREFIX;
    socketFileNameWithPrefix.append(cxx::TruncateToCapacity, socketFileName);
    createTestSocket(socketFileNameWithPrefix);
    auto ret = UnixDomainSocket::unlinkIfExists(socketFileName);
    EXPECT_FALSE(ret.has_error());
}

void sendOnServerLeadsToError(const sendCall_t& send)
{
    cxx::string<10> message{"Foo"};
    auto result = send(message);
    EXPECT_TRUE(result.has_error());
    ASSERT_THAT(result.get_error(), Eq(IpcChannelError::INTERNAL_LOGIC_ERROR));
}

TEST_F(UnixDomainSocket_test, TimedSendOnServerLeadsToError)
{
    sendOnServerLeadsToError([&](auto& msg) { return server.timedSend(msg, 1_ms); });
}

TEST_F(UnixDomainSocket_test, SendOnServerLeadsToError)
{
    sendOnServerLeadsToError([&](auto& msg) { return server.send(msg); });
}

void successfulSendAndReceive(const std::string& message, const sendCall_t& send, const receiveCall_t& receive)
{
    ASSERT_FALSE(send(message).has_error());

    auto receivedMessage = receive();
    ASSERT_FALSE(receivedMessage.has_error());
    EXPECT_EQ(message, *receivedMessage);
}

TEST_F(UnixDomainSocket_test, SuccessfulCommunicationOfNonEmptyMessageWithSendAndReceive)
{
    successfulSendAndReceive(
        "what's hypnotoads eye color?",
        [&](auto& msg) { return client.send(msg); },
        [&]() { return server.receive(); });
}

TEST_F(UnixDomainSocket_test, SuccessfulCommunicationOfNonEmptyMessageWithTimedSendAndReceive)
{
    successfulSendAndReceive(
        "the earth is a disc on the back of hypnotoad",
        [&](auto& msg) { return client.timedSend(msg, 1_ms); },
        [&]() { return server.receive(); });
}

TEST_F(UnixDomainSocket_test, SuccessfulCommunicationOfNonEmptyMessageWithTimedSendAndTimedReceive)
{
    successfulSendAndReceive(
        "it is not the sun that rises, it is hypnotoad who is opening its eyes",
        [&](auto& msg) { return client.timedSend(msg, 1_ms); },
        [&]() { return server.timedReceive(1_ms); });
}

TEST_F(UnixDomainSocket_test, SuccessfulCommunicationOfNonEmptyMessageWithSendAndTimedReceive)
{
    successfulSendAndReceive(
        "what is the most beautiful color in the world? it's hypnotoad.",
        [&](auto& msg) { return client.send(msg); },
        [&]() { return server.timedReceive(1_ms); });
}

TEST_F(UnixDomainSocket_test, SuccessfulCommunicationOfEmptyMessageWithSendAndReceive)
{
    successfulSendAndReceive(
        "", [&](auto& msg) { return client.send(msg); }, [&]() { return server.receive(); });
}

TEST_F(UnixDomainSocket_test, SuccessfulCommunicationOfEmptyMessageWithTimedSendAndReceive)
{
    successfulSendAndReceive(
        "", [&](auto& msg) { return client.timedSend(msg, 1_ms); }, [&]() { return server.receive(); });
}

TEST_F(UnixDomainSocket_test, SuccessfulCommunicationOfEmptyMessageWithTimedSendAndTimedReceive)
{
    successfulSendAndReceive(
        "", [&](auto& msg) { return client.timedSend(msg, 1_ms); }, [&]() { return server.timedReceive(1_ms); });
}

TEST_F(UnixDomainSocket_test, SuccessfulCommunicationOfEmptyMessageWithSendAndTimedReceive)
{
    successfulSendAndReceive(
        "", [&](auto& msg) { return client.send(msg); }, [&]() { return server.timedReceive(1_ms); });
}

TEST_F(UnixDomainSocket_test, SuccessfulCommunicationOfMaxLengthMessageWithSendAndReceive)
{
    successfulSendAndReceive(
        std::string(UnixDomainSocket::MAX_MESSAGE_SIZE, 'x'),
        [&](auto& msg) { return client.send(msg); },
        [&]() { return server.receive(); });
}

TEST_F(UnixDomainSocket_test, SuccessfulCommunicationOfMaxLengthMessageWithTimedSendAndReceive)
{
    successfulSendAndReceive(
        std::string(UnixDomainSocket::MAX_MESSAGE_SIZE, 'x'),
        [&](auto& msg) { return client.timedSend(msg, 1_ms); },
        [&]() { return server.receive(); });
}

TEST_F(UnixDomainSocket_test, SuccessfulCommunicationOfMaxLengthMessageWithTimedSendAndTimedReceive)
{
    successfulSendAndReceive(
        std::string(UnixDomainSocket::MAX_MESSAGE_SIZE, 'x'),
        [&](auto& msg) { return client.timedSend(msg, 1_ms); },
        [&]() { return server.timedReceive(1_ms); });
}

TEST_F(UnixDomainSocket_test, SuccessfulCommunicationOfMaxLengthMessageWithSendAndTimedReceive)
{
    successfulSendAndReceive(
        std::string(UnixDomainSocket::MAX_MESSAGE_SIZE, 'x'),
        [&](auto& msg) { return client.send(msg); },
        [&]() { return server.timedReceive(1_ms); });
}

void unableToSendTooLongMessage(const sendCall_t& send)
{
    std::string message(UnixDomainSocket::MAX_MESSAGE_SIZE + 1, 'x');
    auto result = send(message);
    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(result.get_error(), IpcChannelError::MESSAGE_TOO_LONG);
}

TEST_F(UnixDomainSocket_test, UnableToSendTooLongMessageWithSend)
{
    unableToSendTooLongMessage([&](auto& msg) { return client.send(msg); });
}

TEST_F(UnixDomainSocket_test, UnableToSendTooLongMessageWithTimedSend)
{
    unableToSendTooLongMessage([&](auto& msg) { return client.timedSend(msg, 1_ms); });
}

void receivingOnClientLeadsToError(const receiveCall_t& receive)
{
    auto result = receive();
    EXPECT_TRUE(result.has_error());
    ASSERT_THAT(result.get_error(), Eq(IpcChannelError::INTERNAL_LOGIC_ERROR));
}

TEST_F(UnixDomainSocket_test, ReceivingOnClientLeadsToErrorWithReceive)
{
    receivingOnClientLeadsToError([&] { return client.receive(); });
}

TEST_F(UnixDomainSocket_test, ReceivingOnClientLeadsToErrorWithTimedReceive)
{
    receivingOnClientLeadsToError([&] { return client.timedReceive(1_ms); });
}

// is not supported on mac os and behaves there like receive
#if !defined(__APPLE__)
TEST_F(UnixDomainSocket_test, TimedReceiveBlocks)
{
    auto start = std::chrono::steady_clock::now();
    auto msg = server.timedReceive(units::Duration::fromMilliseconds(WAIT_IN_MS.count()));
    auto end = std::chrono::steady_clock::now();
    EXPECT_THAT(end - start, Gt(WAIT_IN_MS));

    ASSERT_TRUE(msg.has_error());
    EXPECT_EQ(msg.get_error(), IpcChannelError::TIMEOUT);
}

TEST_F(UnixDomainSocket_test, TimedReceiveBlocksUntilMessageIsReceived)
{
    std::string message = "asdasda";
    std::thread waitThread([&] {
        this->signalThreadReady();
        auto start = std::chrono::steady_clock::now();
        auto msg = server.timedReceive(units::Duration::fromMilliseconds(WAIT_IN_MS.count() * 2));
        auto end = std::chrono::steady_clock::now();
        EXPECT_THAT(end - start, Gt(WAIT_IN_MS));

        ASSERT_FALSE(msg.has_error());
        EXPECT_EQ(*msg, message);
    });

    this->waitForThread();
    std::this_thread::sleep_for(WAIT_IN_MS);
    ASSERT_FALSE(client.send(message).has_error());
    waitThread.join();
}
#endif
} // namespace
