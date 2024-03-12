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
#ifndef IOX_POSH_POPO_USED_CHUNK_LIST_HPP
#define IOX_POSH_POPO_USED_CHUNK_LIST_HPP

#include "iox/detail/mpmc_loffli.hpp"
#include "iceoryx_posh/internal/mepoo/shared_chunk.hpp"
#include "iceoryx_posh/internal/mepoo/shm_safe_unmanaged_chunk.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"

#include "iox/not_null.hpp"
#include "iox/expected.hpp"

#include <atomic>
#include <cstdint>

namespace iox
{
namespace popo
{
enum class UsedChunkInsertError
{
  NO_FREE_SPACE,
};

enum class UsedChunkRemoveError
{
  CHUNK_ALREADY_FREED,
  INVALID_INDEX,
  WRONG_CHUNK_REFERENCED,
};

/// @brief Encapsulates information identifying a chunk stored in the UsedChunkList.
struct UsedChunk
{
  not_null<const mepoo::ChunkHeader*> chunkHeader;
  uint32_t listIndex;
};

/// @brief This class is used to keep track of the chunks currently in use by the application.
///        In case the application terminates while holding chunks, this list is used by RouDi to retain ownership of
///        the chunks and prevent a chunk leak.
///        In order to always be able to access the used chunks, neither a vector or list can be used, because these
///        container could be corrupted when the application dies in the wrong moment.
///        To be able to do the cleanup, RouDi needs to be able to access the list with the used chunk under all
///        circumstances. This is achieved by storing the ChunkManagement pointer in an array which can always be
///        accessed. Additionally, the type stored is this array must be less or equal to 64 bit in order to write it
///        within one clock cycle to prevent torn writes, which would corrupt the list and could potentially crash
///        RouDi.
template <uint32_t Capacity>
class UsedChunkList
{
    static_assert(Capacity > 0, "UsedChunkList Capacity must be larger than 0!");

  public:
    using freeList_t = concurrent::MpmcLoFFLi;
    
    /// @brief Constructs a default UsedChunkList
    UsedChunkList() noexcept;

    /// @brief Inserts a SharedChunk into the list
    /// @param[in] chunk to store in the list
    /// @return A UsedChunk referencing insertion location if successful, 
    ///         otherwise a UsedChunkInsertError indicating the cause of failure.
    /// @note only from runtime context
    expected<UsedChunk, UsedChunkInsertError> insert(mepoo::SharedChunk chunk) noexcept;

    /// @brief Removes a chunk from the list
    /// @param[in] usedChunk Referencing insertion location of the SharedChunk to remove.
    /// @return Removed chunk if successful, otherwise a UsedChunkRemoveError indicating the cause of failure.
    /// @note only from runtime context
    expected<mepoo::SharedChunk, UsedChunkRemoveError> remove(const UsedChunk usedChunk) noexcept;

    /// @brief Cleans up all the remaining chunks from the list.
    /// @note from RouDi context once the applications walked the plank. It is unsafe to call this if the application is
    /// still running.
    void cleanup() noexcept;

  private:
    void init() noexcept;

  private:
    static constexpr uint32_t INVALID_INDEX{Capacity};

    using DataElement_t = mepoo::ShmSafeUnmanagedChunk;
    static constexpr DataElement_t DATA_ELEMENT_LOGICAL_NULLPTR{};

  private:
    std::atomic_flag m_synchronizer = ATOMIC_FLAG_INIT;
    DataElement_t m_listData[Capacity];
    uint32_t m_freeListStorage[Capacity]; 
    freeList_t m_freeList;
};

} // namespace popo
} // namespace iox

#include "used_chunk_list.inl"

#endif // IOX_POSH_POPO_USED_CHUNK_LIST_HPP
