// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
// Copyright (c) 2023 by Mathias Kraus <elboberido@m-hias.de>. All rights reserved.
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
#ifndef IOX_POSH_MEPOO_SEGMENT_MANAGER_INL
#define IOX_POSH_MEPOO_SEGMENT_MANAGER_INL

#include "iceoryx_posh/error_handling/error_handling.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/mepoo/segment_manager.hpp"

namespace iox
{
namespace mepoo
{
template <typename SegmentType>
inline SegmentManager<SegmentType>::SegmentManager(const SegmentConfig& segmentConfig,
                                                   BumpAllocator* managementAllocator) noexcept
    : m_managementAllocator(managementAllocator)
{
    IOX_EXPECTS(segmentConfig.m_sharedMemorySegments.capacity() <= m_segmentContainer.capacity());
    for (const auto& segmentEntry : segmentConfig.m_sharedMemorySegments)
    {
        createSegment(segmentEntry);
    }
}

template <typename SegmentType>
inline void SegmentManager<SegmentType>::createSegment(const SegmentConfig::SegmentEntry& segmentEntry) noexcept
{
    auto readerGroup = PosixGroup(segmentEntry.m_readerGroup);
    auto writerGroup = PosixGroup(segmentEntry.m_writerGroup);
    m_segmentContainer.emplace_back(
        segmentEntry.m_shmName, segmentEntry.m_mempoolConfig, *m_managementAllocator, readerGroup, writerGroup, segmentEntry.m_memoryInfo);
}

template <typename SegmentType>
inline typename SegmentManager<SegmentType>::SegmentMappingContainer
SegmentManager<SegmentType>::getSegmentMappings(const PosixUser& user) noexcept
{
    // get all the groups the user is in
    auto groupContainer = user.getGroups();

    SegmentManager::SegmentMappingContainer mappingContainer;

    // with the groups we can get all the segments (read or write) for the user
    for (const auto& groupID : groupContainer)
    {
        for (const auto& segment : m_segmentContainer)
        {
            if (segment.getWriterGroup() == groupID)
            {
                mappingContainer.emplace_back(
                    segment.getShmName(),
                    segment.getSegmentSize(),
                    true,
                    segment.getSegmentId());
            }
        }
    }

    for (const auto& groupID : groupContainer)
    {
        for (const auto& segment : m_segmentContainer)
        {
            // only add segments which are not yet added as writer
            if (segment.getReaderGroup() == groupID
                && std::find_if(mappingContainer.begin(), mappingContainer.end(), [&](const SegmentMapping& mapping) {
                       return mapping.m_segmentId == segment.getSegmentId();
                   }) == mappingContainer.end())
            {
                mappingContainer.emplace_back(
                    segment.getWriterGroup().getName(), segment.getSegmentSize(), false, segment.getSegmentId());
            }
        }
    }

    return mappingContainer;
}

template <typename SegmentType>
inline typename SegmentManager<SegmentType>::SegmentUserInformation
SegmentManager<SegmentType>::getSegmentInformation(const ShmName_t& shmName, const PosixUser& user) noexcept
{
    auto groupContainer = user.getGroups();

    SegmentUserInformation segmentInfo{nullopt_t(), 0u};

    // First search for a segment with a matching name
    for (auto& segment : m_segmentContainer)
    {
        if (segment.getShmName() == shmName)
        {
            //Verify that the user has write access to this segment.
            for (const auto& groupID : groupContainer)
            {
                if (segment.getWriterGroup() == groupID)
                {
                    segmentInfo.m_memoryManager = segment.getMemoryManager();
                    segmentInfo.m_segmentID = segment.getSegmentId();
                    return segmentInfo;
                }
            }
            // None was found so we return the default info.
            return segmentInfo;
        }
    }
    
    // Fall back to searching for a writable segment whose name corresponds to one of the user groups.
    // Note that this heuristic is based on the default behavior of RouDi to assign the name to the 
    // writer group if no name is provided.
    for (const auto& groupID : groupContainer)
    {
        for (auto& segment : m_segmentContainer)
        {
            if (segment.getShmName() == groupID.getName() && segment.getWriterGroup() == groupID)
            {
                segmentInfo.m_memoryManager = segment.getMemoryManager();
                segmentInfo.m_segmentID = segment.getSegmentId();
                return segmentInfo;
            }
        }
    }

    return segmentInfo;
}

template <typename SegmentType>
uint64_t SegmentManager<SegmentType>::requiredManagementMemorySize(const SegmentConfig& config) noexcept
{
    uint64_t memorySize{0u};
    for (auto segment : config.m_sharedMemorySegments)
    {
        memorySize += MemoryManager::requiredManagementMemorySize(segment.m_mempoolConfig);
    }
    return memorySize;
}

template <typename SegmentType>
uint64_t SegmentManager<SegmentType>::requiredChunkMemorySize(const SegmentConfig& config) noexcept
{
    uint64_t memorySize{0u};
    for (auto segment : config.m_sharedMemorySegments)
    {
        memorySize += MemoryManager::requiredChunkMemorySize(segment.m_mempoolConfig);
    }
    return memorySize;
}

template <typename SegmentType>
uint64_t SegmentManager<SegmentType>::requiredFullMemorySize(const SegmentConfig& config) noexcept
{
    return requiredManagementMemorySize(config) + requiredChunkMemorySize(config);
}

} // namespace mepoo
} // namespace iox

#endif // IOX_POSH_MEPOO_SEGMENT_MANAGER_INL
