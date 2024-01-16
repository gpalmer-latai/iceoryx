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
#ifndef IOX_POSH_MEPOO_SEGMENT_MANAGER_HPP
#define IOX_POSH_MEPOO_SEGMENT_MANAGER_HPP

#include "iceoryx_posh/iceoryx_posh_config.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"
#include "iceoryx_posh/internal/mepoo/mepoo_segment.hpp"
#include "iceoryx_posh/mepoo/segment_config.hpp"
#include "iox/bump_allocator.hpp"
#include "iox/optional.hpp"
#include "iox/posix_user.hpp"
#include "iox/string.hpp"
#include "iox/vector.hpp"
#include "iox/expected.hpp"

namespace iox
{
namespace roudi
{
template <typename MemoryManager, typename SegmentManager, typename PublisherPort>
class MemPoolIntrospection;
}

namespace mepoo
{
template <typename SegmentType = MePooSegment<>>
class SegmentManager
{
  public:
    SegmentManager(const SegmentConfig& segmentConfig, BumpAllocator* managementAllocator) noexcept;
    ~SegmentManager() noexcept = default;

    SegmentManager(const SegmentManager& rhs) = delete;
    SegmentManager(SegmentManager&& rhs) = delete;

    SegmentManager& operator=(const SegmentManager& rhs) = delete;
    SegmentManager& operator=(SegmentManager&& rhs) = delete;

    struct SegmentMapping
    {
      public:
        SegmentMapping(const ShmName_t& name,
                       uint64_t size,
                       bool isWritable,
                       uint64_t segmentId,
                       const iox::mepoo::MemoryInfo& memoryInfo = iox::mepoo::MemoryInfo()) noexcept
            : m_name(name)
            , m_size(size)
            , m_isWritable(isWritable)
            , m_segmentId(segmentId)
            , m_memoryInfo(memoryInfo)

        {
        }

        ShmName_t m_name{""};
        uint64_t m_size{0};
        bool m_isWritable{false};
        uint64_t m_segmentId{0};
        iox::mepoo::MemoryInfo m_memoryInfo; // we can specify additional info about a segments memory here
    };

    struct SegmentUserInformation
    {
        MemoryManager& m_memoryManager;
        uint64_t m_segmentID;
    };

    using SegmentMappingContainer = vector<SegmentMapping, MAX_SHM_SEGMENTS>;

    SegmentMappingContainer getSegmentMappings(const PosixUser& user) noexcept;

    enum class SegmentLookupError
    {
      NoSegmentFound,
      NoWriteAccess,
    };

    /// @brief Get the information for the requested segment to which the user has write access.
    /// @param[in] name Name of the segment. 
    ///                    If empty, each user group name will be tried instead until a match is found.
    /// @param[in] user Posix user information. 
    ///                 Used to determine write access and as a fallback for selecting a segment if no name is provided.
    /// @return Information about the requested shared memory segment or a SegmentLookupError.
    expected<SegmentUserInformation, SegmentLookupError> getSegmentInformationWithWriteAccessForUser(const ShmName_t& name, const PosixUser& user) noexcept;
    
    /// @brief Find the segment whose name implicitly matches the user's write-access permissions.
    /// @details This reflects the legacy behavior where producers could only use the unique segment
    ///          to which they have write access. These segments had no name specified and would be matched
    ///          solely based on their access rights. 
    ///          Now, segments with no names are given a default name of the RouDi process user group and this is 
    ///          used instead to perform a match, producing the same semantic behavior.
    /// @param[in] user Posix user information.
    /// @return Information about the requested shared memory segment if one is found or a SegmentLookupError.
    expected<SegmentUserInformation, SegmentLookupError> getSegmentInformationWithWriteAccessForUser(const PosixUser& user) noexcept;

    static uint64_t requiredManagementMemorySize(const SegmentConfig& config) noexcept;
    static uint64_t requiredChunkMemorySize(const SegmentConfig& config) noexcept;
    static uint64_t requiredFullMemorySize(const SegmentConfig& config) noexcept;

  private:
    void createSegment(const SegmentConfig::SegmentEntry& segmentEntry) noexcept;

  private:
    template <typename MemoryManger, typename SegmentManager, typename PublisherPort>
    friend class roudi::MemPoolIntrospection;

    BumpAllocator* m_managementAllocator;
    vector<SegmentType, MAX_SHM_SEGMENTS> m_segmentContainer;
    bool m_createInterfaceEnabled{true};
};


} // namespace mepoo
} // namespace iox

#include "iceoryx_posh/internal/mepoo/segment_manager.inl"

#endif // IOX_POSH_MEPOO_SEGMENT_MANAGER_HPP
