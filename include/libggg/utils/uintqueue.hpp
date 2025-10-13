#pragma once

// Uintqueue utility moved from solvers/uintqueue.hpp into the public library
// Slight API-preserving wrapper (keeps name Uintqueue and behaviour).

#include <stdexcept>
#include <utility>

class Uintqueue {
  public:
    typedef unsigned int uint;

    Uintqueue() {
        pointer = 0;
        queue = nullptr;
    }

    explicit Uintqueue(uint capacity) {
        pointer = 0;
        queue = new uint[capacity];
        this->capacity = capacity;
    }

    ~Uintqueue() {
        delete[] queue;
    }

    inline bool nonempty() const { return (bool)pointer; }
    inline bool empty() const { return pointer == 0; }
    inline uint pop() { return (queue[--pointer]); }
    inline void pop2() { pointer -= 2; }
    inline void push(uint element) {
#ifndef NDEBUG
        if (pointer >= capacity) {
            throw std::runtime_error("Uintqueue: push() would exceed capacity");
        }
#endif
        queue[pointer++] = element;
    }
    inline uint &back() { return (queue[pointer - 1]); }
    inline uint &back2() { return (queue[pointer - 2]); }
    inline void clear() { pointer = 0; }
    inline uint &operator[](uint idx) { return (queue[idx]); }
    inline uint size() const { return (pointer); }
    inline void resize(uint capacity) {
        pointer = 0;
        if (queue != nullptr) {
            delete[] queue;
        }
        queue = new uint[capacity];
        this->capacity = capacity;
    }
    inline void swap(Uintqueue &other) {
        std::swap(queue, other.queue);
        std::swap(pointer, other.pointer);
        std::swap(capacity, other.capacity);
    }
    inline void swap_elements(uint idx1, uint idx2) { std::swap(queue[idx1], queue[idx2]); }

  protected:
    uint *queue = nullptr;
    int pointer = 0;
    uint capacity = 0;
};
