// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
// Copyright (c) 2023 by Mathias Kraus <elboberido@m-hias.de>. All rights reserved.
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
    if (std::find_if(
            m_segmentContainer.begin(),
            m_segmentContainer.end(), 
            [&segmentEntry](const SegmentType& segment){ return segment.getSegmentName() == segmentEntry.m_name;}
        ) != m_segmentContainer.end())
    {
        errorHandler(PoshError::MEPOO__MULTIPLE_SEGMENT_CONFIG_ENTRIES_WITH_SAME_NAME);
        return;
    }
    auto readerGroup = PosixGroup(segmentEntry.m_readerGroup);
    auto writerGroup = PosixGroup(segmentEntry.m_writerGroup);
    m_segmentContainer.emplace_back(
        segmentEntry.m_name, segmentEntry.m_mempoolConfig, *m_managementAllocator, readerGroup, writerGroup, segmentEntry.m_memoryInfo);
}

template <typename SegmentType>
inline typename SegmentManager<SegmentType>::SegmentMappingContainer
SegmentManager<SegmentType>::getSegmentMappings(const PosixUser& user) noexcept
{
    // get all the groups the user is in
    auto groupContainer = user.getGroups();

    SegmentManager::SegmentMappingContainer mappingContainer;

    for (const auto& segment : m_segmentContainer)
    {
        for (const auto& groupID : groupContainer)
        {
            const bool isWritable = (segment.getWriterGroup() == groupID);
            const bool isReadable = (segment.getReaderGroup() == groupID);
            if (isWritable || isReadable)
            {
                mappingContainer.emplace_back(segment.getSegmentName(), segment.getSegmentSize(), isWritable, segment.getSegmentId());
                break;
            }
        }
    }

    return mappingContainer;
}

template <typename SegmentType>
inline expected<typename SegmentManager<SegmentType>::SegmentUserInformation, 
         typename SegmentManager<SegmentType>::SegmentLookupError> 
SegmentManager<SegmentType>::getSegmentInformationWithWriteAccessForUser(const ShmName_t& name, const PosixUser& user) noexcept
{
    for (auto& segment : m_segmentContainer)
    {
        if (segment.getSegmentName() == name)
        {
            // Verify that the user has write access to this segment.
            for (const auto& groupID : user.getGroups())
            {
                if (segment.getWriterGroup() == groupID)
                {
                    return ok(SegmentUserInformation{segment.getMemoryManager(), segment.getSegmentId()});
                }
            }
            return err(SegmentLookupError::NoWriteAccess);
        }
    }

    return getSegmentInformationWithWriteAccessForUser(user);
}

template <typename SegmentType>
inline expected<typename SegmentManager<SegmentType>::SegmentUserInformation, 
         typename SegmentManager<SegmentType>::SegmentLookupError> 
SegmentManager<SegmentType>::getSegmentInformationWithWriteAccessForUser(const PosixUser& user) noexcept
{
    // with the groups we can search for the writable segment of this user
    for (const auto& groupID : user.getGroups())
    {
        for (auto& segment : m_segmentContainer)
        {
            if (segment.getSegmentName() == groupID.getName() && segment.getWriterGroup() == groupID)
            {
                return ok(SegmentUserInformation{segment.getMemoryManager(), segment.getSegmentId()});
            }
        }
    }

    return err(SegmentLookupError::NoSegmentFound);
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
