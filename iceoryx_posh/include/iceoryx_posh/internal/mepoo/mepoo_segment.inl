// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 - 2022 by Apex.AI Inc. All rights reserved.
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
#ifndef IOX_POSH_MEPOO_MEPOO_SEGMENT_INL
#define IOX_POSH_MEPOO_MEPOO_SEGMENT_INL

#include "iceoryx_posh/error_handling/error_handling.hpp"
#include "iceoryx_posh/internal/mepoo/mepoo_segment.hpp"
#include "iceoryx_posh/mepoo/memory_info.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iox/bump_allocator.hpp"
#include "iox/logging.hpp"
#include "iox/relative_pointer.hpp"

namespace iox
{
namespace mepoo
{
template <typename SharedMemoryObjectType, typename MemoryManagerType>
constexpr access_rights MePooSegment<SharedMemoryObjectType, MemoryManagerType>::SEGMENT_PERMISSIONS;

template <typename SharedMemoryObjectType, typename MemoryManagerType>
inline MePooSegment<SharedMemoryObjectType, MemoryManagerType>::MePooSegment(
    const ShmName_t& name,
    const MePooConfig& mempoolConfig,
    BumpAllocator& managementAllocator,
    const PosixGroup& readerGroup,
    const PosixGroup& writerGroup,
    const iox::mepoo::MemoryInfo& memoryInfo) noexcept
    : m_name(name)
    , m_readerGroup(readerGroup)
    , m_writerGroup(writerGroup)
    , m_memoryInfo(memoryInfo)
    , m_sharedMemoryObject(createSharedMemoryObject(mempoolConfig, name))
{
    using namespace detail;
    PosixAcl acl;
    if (!(readerGroup == writerGroup))
    {
        acl.addGroupPermission(PosixAcl::Permission::READ, readerGroup.getName());
    }
    acl.addGroupPermission(PosixAcl::Permission::READWRITE, writerGroup.getName());
    acl.addPermissionEntry(PosixAcl::Category::USER, PosixAcl::Permission::READWRITE);
    acl.addPermissionEntry(PosixAcl::Category::GROUP, PosixAcl::Permission::READWRITE);
    acl.addPermissionEntry(PosixAcl::Category::OTHERS, PosixAcl::Permission::NONE);

    if (!acl.writePermissionsToFile(m_sharedMemoryObject.getFileHandle()))
    {
        errorHandler(PoshError::MEPOO__SEGMENT_COULD_NOT_APPLY_POSIX_RIGHTS_TO_SHARED_MEMORY);
    }

    BumpAllocator allocator(m_sharedMemoryObject.getBaseAddress(),
                            m_sharedMemoryObject.get_size().expect("Failed to get SHM size."));
    m_memoryManager.configureMemoryManager(mempoolConfig, managementAllocator, allocator);
}

template <typename SharedMemoryObjectType, typename MemoryManagerType>
inline SharedMemoryObjectType MePooSegment<SharedMemoryObjectType, MemoryManagerType>::createSharedMemoryObject(
    const MePooConfig& mempoolConfig, const ShmName_t& name) noexcept
{
    return std::move(
        typename SharedMemoryObjectType::Builder()
            .name(name)
            .memorySizeInBytes(MemoryManager::requiredChunkMemorySize(mempoolConfig))
            .accessMode(AccessMode::READ_WRITE)
            .openMode(OpenMode::PURGE_AND_CREATE)
            .permissions(SEGMENT_PERMISSIONS)
            .create()
            .and_then([this](auto& sharedMemoryObject) {
                auto maybeSegmentId = iox::UntypedRelativePointer::registerPtr(
                    sharedMemoryObject.getBaseAddress(),
                    sharedMemoryObject.get_size().expect("Failed to get SHM size"));
                if (!maybeSegmentId.has_value())
                {
                    errorHandler(PoshError::MEPOO__SEGMENT_INSUFFICIENT_SEGMENT_IDS);
                }
                this->m_segmentId = static_cast<uint64_t>(maybeSegmentId.value());
                this->m_segmentSize = sharedMemoryObject.get_size().expect("Failed to get SHM size.");

                IOX_LOG(DEBUG,
                        "Roudi registered payload data segment " << iox::log::hex(sharedMemoryObject.getBaseAddress())
                                                                 << " with size " << m_segmentSize << " to id "
                                                                 << m_segmentId);
            })
            .or_else([](auto&) { errorHandler(PoshError::MEPOO__SEGMENT_UNABLE_TO_CREATE_SHARED_MEMORY_OBJECT); })
            .value());
}

template <typename SharedMemoryObjectType, typename MemoryManagerType>
inline PosixGroup MePooSegment<SharedMemoryObjectType, MemoryManagerType>::getWriterGroup() const noexcept
{
    return m_writerGroup;
}

template <typename SharedMemoryObjectType, typename MemoryManagerType>
inline PosixGroup MePooSegment<SharedMemoryObjectType, MemoryManagerType>::getReaderGroup() const noexcept
{
    return m_readerGroup;
}

template <typename SharedMemoryObjectType, typename MemoryManagerType>
inline MemoryManagerType& MePooSegment<SharedMemoryObjectType, MemoryManagerType>::getMemoryManager() noexcept
{
    return m_memoryManager;
}

template <typename SharedMemoryObjectType, typename MemoryManagerType>
inline const ShmName_t& MePooSegment<SharedMemoryObjectType, MemoryManagerType>::getSegmentName() const noexcept
{
    return m_name;
}

template <typename SharedMemoryObjectType, typename MemoryManagerType>
inline uint64_t MePooSegment<SharedMemoryObjectType, MemoryManagerType>::getSegmentId() const noexcept
{
    return m_segmentId;
}

template <typename SharedMemoryObjectType, typename MemoryManagerType>
inline uint64_t MePooSegment<SharedMemoryObjectType, MemoryManagerType>::getSegmentSize() const noexcept
{
    return m_segmentSize;
}

} // namespace mepoo
} // namespace iox

#endif // IOX_POSH_MEPOO_MEPOO_SEGMENT_INL
