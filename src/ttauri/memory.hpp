// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "required.hpp"
#include "concepts"
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>

namespace tt {

template<typename T, typename U>
void memswap(T &dst, U &src) {
    static_assert(sizeof(T) == sizeof(U), "memswap requires both objects of equal size");
    std::byte tmp[sizeof(T)];
    memcpy(tmp, &src, sizeof(T));
    memcpy(&src, &dst, sizeof(U));
    memcpy(&dst, tmp, sizeof(T));
}

/** Copy an object to another memory locations.
 * This function will copy an object located in one memory location
 * to another through the use of placement-new.
 *
 * If you want to access the object in dst, you should use the return value.
 *
 * @param src An interator to an object.
 * @param dst A pointer to allocated memory.
 * @return The dst pointer with the new object who's lifetime was started.
 */
template<typename InputIt, typename T>
T *placement_copy(InputIt src, T *dst)
{
    tt_axiom(dst != nullptr);
    return new (dst) T(*src);
}

/** Copy objects into a memory location.
 * This function will placement_copy an array of objects
 * to a memory location.
 */
template<typename InputIt, typename T>
void placement_copy(InputIt src_first, InputIt src_last, T *dst_first)
{
    tt_axiom(src_first != dst_first);
    tt_axiom(src_last >= src_first);

    auto src = src_first;
    auto dst = dst_first;
    while (src != src_last) {
        placement_copy(src++, dst++);
    }
}

/** Move an object between two memory locations.
 * This function will move an object from one memory location
 * to another through the use of placement-new. The object
 * in the source memory location is destroyed.
 *
 * If you want to access the object in dst, you should use the return value.
 * It is undefined behavior when src and dst point to the same object.
 * It is undefined behavior if either or both src and dst are nullptr.
 * 
 * @param src A pointer to an object.
 * @param dst A pointer to allocated memory.
 * @return The dst pointer with the new object who's lifetime was started.
 */
template<typename T>
T *placement_move(T *src, T *dst)
{
    tt_axiom(src != nullptr);
    tt_axiom(dst != nullptr);

    auto dst_ = new (dst) T(std::move(*src));
    std::destroy_at(src);
    return dst_;
}

/** Move an objects between two memory locations.
 * This function will placement_move an array of objects
 * between two memory locations.
 * 
 * It is undefined behavior when src_first and dst_first are not part of the same array.
 * 
 * The objects may overlap: copying takes place as if the objects were copied
 * to a temporary object array and then the objects were copied from the array to dst.
 */
template<typename T>
void placement_move_within_array(T *src_first, T *src_last, T *dst_first)
{
    tt_axiom(src_last >= src_first);

    if (src_first < dst_first) {
        auto dst_last = dst_first + (src_last - src_first);

        auto src = src_last;
        auto dst = dst_last;
        while (src != src_first) {
            placement_move(--src, --dst);
        }

    } else if (src_first > dst_first) {
        auto src = src_first;
        auto dst = dst_first;
        while (src != src_last) {
            placement_move(src++, dst++);
        }

    } else {
        // When src_first and dst_first are equal then no movement is necessary.
        ;
    }
}

/** Move an objects between two memory locations.
 * This function will placement_move an array of objects
 * between two memory locations.
 *
 * WARNING: if moving objects within an array use `placement_move_within_array`
 * to handle overlapping regions.
 */
template<typename T>
void placement_move(T *src, T *src_last, T *dst)
{
    tt_axiom(src_last >= src);

    while (src != src_last) {
        placement_move(src++, dst++);
    }
}

template<typename T>
bool is_aligned(T* p){
    return (reinterpret_cast<ptrdiff_t>(p) % std::alignment_of<T>::value) == 0;
}

template<std::unsigned_integral T>
constexpr T floor(T value, T alignment) noexcept
{
    return (value / alignment) * alignment;
}

template<std::unsigned_integral T>
constexpr T ceil(T value, T alignment) noexcept
{
    return floor(value + (alignment - 1), alignment);
}

template<typename R, typename T>
R align(T ptr, size_t alignment) noexcept
{
    ttlet byteOffset = reinterpret_cast<ptrdiff_t>(ptr);
    ttlet alignedByteOffset = ((byteOffset + alignment - 1) / alignment) * alignment;

    return reinterpret_cast<R>(alignedByteOffset);
}

/*! Align an end iterator.
* This lowers the end interator so that it the last read is can be done fully.
*/
template<typename R, typename T>
inline R align_end(T ptr, size_t alignment) noexcept
{
    ttlet byteOffset = reinterpret_cast<ptrdiff_t>(ptr);
    ttlet alignedByteOffset = (byteOffset / alignment) * alignment;

    return reinterpret_cast<R>(alignedByteOffset);
}


template<typename T>
inline void cleanupWeakPointers(std::vector<std::weak_ptr<T>> &v) noexcept
{
    auto i = v.begin();
    while (i != v.end()) {
        if (i->expired()) {
            i = v.erase(i);
        } else {
            i++;
        }
    }
}

template<typename K, typename T>
inline void cleanupWeakPointers(std::unordered_map<K,std::weak_ptr<T>> &v) noexcept
{
    auto i = v.begin();
    while (i != v.end()) {
        if (i->second.expired()) {
            i = v.erase(i);
        } else {
            i++;
        }
    }
}

template<typename K, typename T>
inline void cleanupWeakPointers(std::unordered_map<K,std::vector<std::weak_ptr<T>>> &v) noexcept
{
    auto i = v.begin();
    while (i != v.end()) {
        cleanupWeakPointers(i->second);
        if (i->second.size() == 0) {
            i = v.erase(i);
        } else {
            i++;
        }
    }
}

template<typename Value, typename Map, typename Key, typename... Args>
inline std::shared_ptr<Value> try_make_shared(Map &map, Key key, Args... args) {
    std::shared_ptr<Value> value;

    ttlet i = map.find(key);
    if (i == map.end()) {
        value = std::make_shared<Value>(std::forward<Args>(args)...);
        map.insert_or_assign(key, value);
    }
    else {
        value = i->second;
    }
    return value;
}

}
