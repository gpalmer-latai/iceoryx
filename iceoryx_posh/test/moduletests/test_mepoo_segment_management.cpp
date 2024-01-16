// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 - 2022 by Apex.AI Inc. All rights reserved.
// Copyright (c) 2024 by Latitude AI. All rights reserved.
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

#include "iceoryx_hoofs/testing/test_definitions.hpp"
#include "iceoryx_posh/error_handling/error_handling.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/mepoo/segment_manager.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_posh/mepoo/segment_config.hpp"
#include "iox/bump_allocator.hpp"
#include "iox/detail/serialization.hpp"
#include "iox/posix_group.hpp"
#include "iox/posix_shared_memory_object.hpp"
#include "iox/posix_user.hpp"
#include "test.hpp"


namespace
{
using namespace ::testing;
using namespace iox;
using namespace iox::mepoo;

class MePooSegmentMock
{
  public:
    MePooSegmentMock(const ShmName_t& name,
                     const MePooConfig& mempoolConfig [[maybe_unused]],
                     iox::BumpAllocator& managementAllocator [[maybe_unused]],
                     const PosixGroup& readerGroup [[maybe_unused]],
                     const PosixGroup& writerGroup [[maybe_unused]],
                     const MemoryInfo& memoryInfo [[maybe_unused]]) noexcept
    : m_name(name)
    {
    }

    const ShmName_t& getSegmentName() const noexcept
    {
        return m_name;
    }
    
  private:
    ShmName_t m_name;
};

class SegmentManager_test : public Test
{
  public:
    void SetUp() override
    {
    }
    void TearDown() override
    {
        iox::UntypedRelativePointer::unregisterAll();
    }

    MePooConfig getMempoolConfig()
    {
        MePooConfig config;
        config.addMemPool({128, 5});
        config.addMemPool({256, 7});
        return config;
    }

    SegmentConfig getSegmentConfig()
    {
        SegmentConfig config;
        config.m_sharedMemorySegments.emplace_back("iox_roudi_test2", "iox_roudi_test1", "iox_roudi_test2", mepooConfig);
        config.m_sharedMemorySegments.emplace_back("other_segment", "iox_roudi_test3", "iox_roudi_test2", mepooConfig);
        return config;
    }

    SegmentConfig getSegmentConfigWithSameNameEntries()
    {
        SegmentConfig config;
        config.m_sharedMemorySegments.emplace_back("duplicate_name", "iox_roudi_test1", "iox_roudi_test2", mepooConfig);
        config.m_sharedMemorySegments.emplace_back("duplicate_name", "iox_roudi_test2", "iox_roudi_test3", mepooConfig);
        return config;
    }

    SegmentConfig getSegmentConfigWithMaximumNumberOfSegements()
    {
        SegmentConfig config;
        for (uint64_t i = 0U; i < iox::MAX_SHM_SEGMENTS; ++i)
        {
            auto serial = Serialization::create("segment_name", i);
            config.m_sharedMemorySegments.emplace_back(ShmName_t(TruncateToCapacity, serial.toString().c_str()), "iox_roudi_test1", "iox_roudi_test1", mepooConfig);
        }
        return config;
    }

    static constexpr size_t MEM_SIZE{20000};
    char memory[MEM_SIZE];
    iox::BumpAllocator allocator{memory, MEM_SIZE};
    MePooConfig mepooConfig = getMempoolConfig();
    SegmentConfig segmentConfig = getSegmentConfig();

    using SUT = SegmentManager<>;
    std::unique_ptr<SUT> createSut()
    {
        return std::make_unique<SUT>(segmentConfig, &allocator);
    }
};

TEST_F(SegmentManager_test, getSegmentMappingsForReadUser)
{
    ::testing::Test::RecordProperty("TEST_ID", "a3af818a-c119-4182-948a-1e709c29d97f");
    GTEST_SKIP_FOR_ADDITIONAL_USER() << "This test requires the -DTEST_WITH_ADDITIONAL_USER=ON cmake argument";

    auto sut = createSut();
    auto mapping = sut->getSegmentMappings(PosixUser{"iox_roudi_test1"});
    ASSERT_THAT(mapping.size(), Eq(1u));
    EXPECT_THAT(mapping[0].m_isWritable, Eq(false));
}

TEST_F(SegmentManager_test, getSegmentMappingsForWriteUser)
{
    ::testing::Test::RecordProperty("TEST_ID", "5e5d3128-5e4b-41e9-b541-ba9dc0bd57e3");
    GTEST_SKIP_FOR_ADDITIONAL_USER() << "This test requires the -DTEST_WITH_ADDITIONAL_USER=ON cmake argument";

    auto sut = createSut();
    auto mapping = sut->getSegmentMappings(PosixUser{"iox_roudi_test2"});
    ASSERT_THAT(mapping.size(), Eq(2u));
    EXPECT_THAT(mapping[0].m_isWritable == mapping[1].m_isWritable, Eq(true));
}

TEST_F(SegmentManager_test, getSegmentMappingsEmptyForNonRegisteredUser)
{
    ::testing::Test::RecordProperty("TEST_ID", "7cf9a658-bb2d-444f-af67-0355e8f45ea2");
    GTEST_SKIP_FOR_ADDITIONAL_USER() << "This test requires the -DTEST_WITH_ADDITIONAL_USER=ON cmake argument";

    auto sut = createSut();
    auto mapping = sut->getSegmentMappings(PosixUser{"roudi_test4"});
    ASSERT_THAT(mapping.size(), Eq(0u));
}

TEST_F(SegmentManager_test, getSegmentMappingsEmptyForNonExistingUser)
{
    ::testing::Test::RecordProperty("TEST_ID", "ca869fa8-49cd-43bc-8d72-4024b45f1f5f");
    GTEST_SKIP_FOR_ADDITIONAL_USER() << "This test requires the -DTEST_WITH_ADDITIONAL_USER=ON cmake argument";

    auto sut = createSut();
    auto mapping = sut->getSegmentMappings(PosixUser{"no_user"});
    ASSERT_THAT(mapping.size(), Eq(0u));
}

TEST_F(SegmentManager_test, getMemoryManagerForUserWithWriteUser)
{
    ::testing::Test::RecordProperty("TEST_ID", "2fd4262a-20d2-4631-9b63-610944f28120");
    GTEST_SKIP_FOR_ADDITIONAL_USER() << "This test requires the -DTEST_WITH_ADDITIONAL_USER=ON cmake argument";

    auto sut = createSut();
    auto maybeSegment = sut->getSegmentInformationWithWriteAccessForUser(PosixUser{"iox_roudi_test2"});
    ASSERT_TRUE(maybeSegment.has_value());
    auto& memoryManager = maybeSegment->m_memoryManager;
    ASSERT_THAT(memoryManager.getNumberOfMemPools(), Eq(2u));

    auto poolInfo1 = memoryManager.getMemPoolInfo(0);
    auto poolInfo2 = memoryManager.getMemPoolInfo(1);
    EXPECT_THAT(poolInfo1.m_numChunks, Eq(5u));
    EXPECT_THAT(poolInfo2.m_numChunks, Eq(7u));
}

TEST_F(SegmentManager_test, getMemoryManagerForNamedSegmentWithWriteAccess)
{
    ::testing::Test::RecordProperty("TEST_ID", "f9bf7e03-8c76-4a62-9f01-b408997e29b9");
    GTEST_SKIP_FOR_ADDITIONAL_USER() << "This test requires the -DTEST_WITH_ADDITIONAL_USER=ON cmake argument";

    auto sut = createSut();
    auto maybeSegment = sut->getSegmentInformationWithWriteAccessForUser("other_segment", PosixUser{"iox_roudi_test2"});
    ASSERT_TRUE(maybeSegment.has_value());
    auto& memoryManager = maybeSegment->m_memoryManager;
    ASSERT_THAT(memoryManager.getNumberOfMemPools(), Eq(2u));

    auto poolInfo1 = memoryManager.getMemPoolInfo(0);
    auto poolInfo2 = memoryManager.getMemPoolInfo(1);
    EXPECT_THAT(poolInfo1.m_numChunks, Eq(5u));
    EXPECT_THAT(poolInfo2.m_numChunks, Eq(7u));
}

TEST_F(SegmentManager_test, namedAndDefaultWriteAccessSegmentAreUnique)
{
    ::testing::Test::RecordProperty("TEST_ID", "3a7182ed-a64f-4ffe-ba7f-ba5968431eea");
    GTEST_SKIP_FOR_ADDITIONAL_USER() << "This test requires the -DTEST_WITH_ADDITIONAL_USER=ON cmake argument";

    auto sut = createSut();

    auto maybeNamedSegment = sut->getSegmentInformationWithWriteAccessForUser("other_segment", PosixUser{"iox_roudi_test2"});
    ASSERT_TRUE(maybeNamedSegment.has_value());

    auto maybeDefaultSegment = sut->getSegmentInformationWithWriteAccessForUser(PosixUser{"iox_roudi_test2"});
    ASSERT_TRUE(maybeDefaultSegment.has_value());

    EXPECT_NE(maybeNamedSegment->m_segmentID, maybeDefaultSegment->m_segmentID);
}

TEST_F(SegmentManager_test, getMemoryManagerForUserFailWithReadOnlyUser)
{
    ::testing::Test::RecordProperty("TEST_ID", "9d7c18fd-b8db-425a-830d-22c781091244");
    GTEST_SKIP_FOR_ADDITIONAL_USER() << "This test requires the -DTEST_WITH_ADDITIONAL_USER=ON cmake argument";

    auto sut = createSut();
    EXPECT_TRUE(
        sut->getSegmentInformationWithWriteAccessForUser(ShmName_t{"other_segment"}, PosixUser{"iox_roudi_test1"}).error() == SUT::SegmentLookupError::NoWriteAccess);
}

TEST_F(SegmentManager_test, getMemoryManagerForUserFailWithNonExistingUser)
{
    ::testing::Test::RecordProperty("TEST_ID", "bff18ab5-89ff-45e0-97ea-5409169ddf9a");
    GTEST_SKIP_FOR_ADDITIONAL_USER() << "This test requires the -DTEST_WITH_ADDITIONAL_USER=ON cmake argument";

    auto sut = createSut();
    // The user does not exist so no groups are checked for a matching segment.
    EXPECT_TRUE(sut->getSegmentInformationWithWriteAccessForUser(PosixUser{"no_user"}).error() == SUT::SegmentLookupError::NoSegmentFound);
}

TEST_F(SegmentManager_test, getMemoryManagerForUserFailWithNoMatchingSegment)
{
    ::testing::Test::RecordProperty("TEST_ID", "b44a49e9-6e3f-44ac-94d4-cce7e5198fca");
    GTEST_SKIP_FOR_ADDITIONAL_USER() << "This test requires the -DTEST_WITH_ADDITIONAL_USER=ON cmake argument";

    auto sut = createSut();
    // The user exists but none of the groups has a matching name.
    EXPECT_TRUE(sut->getSegmentInformationWithWriteAccessForUser(PosixUser{"iox_roudi_test1"}).error() == SUT::SegmentLookupError::NoSegmentFound);
}

TEST_F(SegmentManager_test, addingMoreThanOneEntryWithSameNameFails)
{
    ::testing::Test::RecordProperty("TEST_ID", "7ab2a78a-c9de-4e1d-a2da-6381f9a5a730");
    GTEST_SKIP_FOR_ADDITIONAL_USER() << "This test requires the -DTEST_WITH_ADDITIONAL_USER=ON cmake argument";

    SegmentConfig segmentConfig = getSegmentConfigWithSameNameEntries();

    iox::optional<iox::PoshError> detectedError;
    auto errorHandlerGuard = iox::ErrorHandlerMock::setTemporaryErrorHandler<iox::PoshError>(
        [&](const iox::PoshError error, const iox::ErrorLevel errorLevel) {
            detectedError.emplace(error);
            EXPECT_THAT(errorLevel, Eq(iox::ErrorLevel::FATAL));
        });

    SUT sut{segmentConfig, &allocator};

    ASSERT_TRUE(detectedError.has_value());
    EXPECT_THAT(detectedError.value(), Eq(iox::PoshError::MEPOO__MULTIPLE_SEGMENT_CONFIG_ENTRIES_WITH_SAME_NAME));
}

TEST_F(SegmentManager_test, addingMaximumNumberOfSegmentsWorks)
{
    ::testing::Test::RecordProperty("TEST_ID", "79db009a-da1a-4140-b375-f174af615d54");
    GTEST_SKIP_FOR_ADDITIONAL_USER() << "This test requires the -DTEST_WITH_ADDITIONAL_USER=ON cmake argument";

    SegmentConfig segmentConfig = getSegmentConfigWithMaximumNumberOfSegements();
    SegmentManager<MePooSegmentMock> sut{segmentConfig, &allocator};
}

} // namespace
