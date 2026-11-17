#pragma once
#include <stddef.h>

/**
 * @brief FIFO queue with a fixed capacity (ring buffer), no heap allocation.
 *
 * Replacement for std::queue / std::deque on small devices: the libstdc++ deque
 * allocates its map (8 pointers) and a first 512 byte block already in the
 * constructor - about 560 bytes of heap for an EMPTY queue. This queue needs
 * exactly N * sizeof(T) bytes inside the owning object.
 *
 * Not thread safe - lock outside, as with the std containers.
 *
 * @tparam T element type (copyable)
 * @tparam N capacity (max. number of elements)
 */
template<typename T, size_t N>
class CRingQueue {
    static_assert(N > 0, "CRingQueue needs a capacity > 0");

    private:
        T      m_tItems[N];
        size_t m_nHead  = 0;    // index of the first (oldest) element
        size_t m_nCount = 0;    // number of elements

    public:
        bool   empty()    const { return(m_nCount == 0); }
        bool   full()     const { return(m_nCount >= N); }
        size_t size()     const { return(m_nCount); }
        static constexpr size_t capacity() { return(N); }

        /// @brief Append an element - false if the queue is full (element not stored).
        bool push(const T &oItem) {
            if(full()) return(false);
            m_tItems[(m_nHead + m_nCount) % N] = oItem;
            m_nCount++;
            return(true);
        }
        bool push_back(const T &oItem) { return(push(oItem)); }

        /// @brief Oldest element - only valid if !empty().
        T & front() { return(m_tItems[m_nHead]); }

        /// @brief Remove the oldest element (no-op on an empty queue).
        void pop() {
            if(m_nCount == 0) return;
            m_tItems[m_nHead] = T();
            m_nHead = (m_nHead + 1) % N;
            m_nCount--;
        }
        void pop_front() { pop(); }

        /// @brief Element at position nIdx, counted from the oldest (0 <= nIdx < size()).
        T & operator[](size_t nIdx) { return(m_tItems[(m_nHead + nIdx) % N]); }

        void clear() {
            while(!empty()) pop();
            m_nHead = 0;
        }
};
