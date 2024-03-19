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

#ifndef IOX_POSH_POPO_USED_CHUNK_LIST_INL
#define IOX_POSH_POPO_USED_CHUNK_LIST_INL

namespace iox
{
namespace popo
{

inline bool operator==(const UsedChunk& lhs, const UsedChunk& rhs)
{
    return static_cast<const mepoo::ChunkHeader*>(lhs.chunkHeader) == static_cast<const mepoo::ChunkHeader*>(rhs.chunkHeader)
           && lhs.listIndex == rhs.listIndex;
}

template <uint32_t Capacity>
constexpr typename UsedChunkList<Capacity>::DataElement_t UsedChunkList<Capacity>::DATA_ELEMENT_LOGICAL_NULLPTR;

template <uint32_t Capacity>
UsedChunkList<Capacity>::UsedChunkList() noexcept
{
    static_assert(sizeof(DataElement_t) <= 8U, "The size of the data element type must not exceed 64 bit!");
    static_assert(std::is_trivially_copyable<DataElement_t>::value,
                  "The data element type must be trivially copyable!");

    init();
}

template <uint32_t Capacity>
expected<UsedChunk, UsedChunkInsertError> UsedChunkList<Capacity>::insert(mepoo::SharedChunk chunk) noexcept
{
    uint32_t index{std::numeric_limits<uint32_t>::max()};
    bool hasFreeSpace = m_freeList.pop(index);
    if (hasFreeSpace)
    {
        IOX_ASSERT(index < Capacity, "Oops, the free list returned an index it is not supposed to");
        m_listData[index] = DataElement_t(chunk);
        m_synchronizer.clear(std::memory_order_release);
        return ok(UsedChunk{chunk.getChunkHeader(), index});
    }
    else
    {
        return err(UsedChunkInsertError::NO_FREE_SPACE);
    }
}

template <uint32_t Capacity>
expected<mepoo::SharedChunk, UsedChunkRemoveError> UsedChunkList<Capacity>::remove(const UsedChunk usedChunk) noexcept
{
    // TODO: Should this be an assertion? What about the other error cases?
    if (usedChunk.listIndex >= Capacity)
    {
        return err(UsedChunkRemoveError::INVALID_INDEX);
    }
    auto& chunkRef = m_listData[usedChunk.listIndex];
    if (chunkRef.isLogicalNullptr())
    {
        return err(UsedChunkRemoveError::CHUNK_ALREADY_FREED);
    }
    if (chunkRef.getChunkHeader() != usedChunk.chunkHeader)
    {
        return err(UsedChunkRemoveError::WRONG_CHUNK_REFERENCED);
    }
    
    auto chunk = chunkRef.releaseToSharedChunk();
    m_freeList.push(usedChunk.listIndex);
    m_synchronizer.clear(std::memory_order_release);
    return ok(std::move(chunk));
}

template <uint32_t Capacity>
void UsedChunkList<Capacity>::cleanup() noexcept
{
    m_synchronizer.test_and_set(std::memory_order_acquire);

    for (auto& data : m_listData)
    {
        if (!data.isLogicalNullptr())
        {
            // release ownership by creating a SharedChunk
            data.releaseToSharedChunk();
        }
    }

    init(); // just to save us from the future self
}

template <uint32_t Capacity>
void UsedChunkList<Capacity>::init() noexcept
{
    // Free list returns indexes within the range [0, capacity] (inclusive)
    // So we need to subtract one from that capacity.
    m_freeList.init(reinterpret_cast<freeList_t::Index_t*>(m_freeListStorage), Capacity);
    m_synchronizer.clear(std::memory_order_release);
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_USED_CHUNK_LIST_INL
