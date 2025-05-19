#include "src/memory.h"
#include <gtest/gtest.h>

namespace allocator {

struct DestructibleClassA {
    DestructibleClassA(int32_t value, std::vector<int32_t>& destruction_stack) : _value(value), _destruction_stack(destruction_stack) {}
    virtual ~DestructibleClassA() {
        _destruction_stack.push_back(_value);
    }

private:
    int32_t _value;
    std::vector<int32_t>& _destruction_stack;
};

struct DestructibleClassB {
    DestructibleClassB(int32_t value, std::vector<int32_t>& destruction_stack) : _value(value), _destruction_stack(destruction_stack) {}
    virtual ~DestructibleClassB() {
        _destruction_stack.push_back(_value);
    }

private:
    int32_t _value;
    std::vector<int32_t>& _destruction_stack;
};

struct DestructibleClassAB : private DestructibleClassA, private DestructibleClassB {
    DestructibleClassAB(int32_t value, int32_t value_a, int32_t value_b, std::vector<int32_t>& destruction_stack)
        : DestructibleClassA(value_a, destruction_stack), DestructibleClassB(value_b, destruction_stack), _value(value), _destruction_stack(destruction_stack) {
    }
    ~DestructibleClassAB() {
        _destruction_stack.push_back(_value);
    }

private:
    int32_t _value;
    std::vector<int32_t>& _destruction_stack;
};

TEST(MemoryDestructorTest, DoesNotDestructNewObjects) {
    in_place_memory memory;
    std::vector<int32_t> destruction_stack;

    const auto* value = memory.allocate<DestructibleClassA>(42, destruction_stack);

    ASSERT_EQ(destruction_stack.size(), 0);
}


TEST(MemoryDestructorTest, DestructsSimpleObject) {
    in_place_memory memory;
    std::vector<int32_t> destruction_stack;

    const auto* value = memory.allocate<DestructibleClassA>(42, destruction_stack);
    memory.deallocate(value);

    ASSERT_EQ(destruction_stack, std::vector<int32_t>({ 42 }));
}

TEST(MemoryDestructorTest, DestructsMultipleObjects) {
    in_place_memory memory;
    std::vector<int32_t> destruction_stack;

    const auto* value1 = memory.allocate<DestructibleClassA>(42, destruction_stack);
    const auto* value2 = memory.allocate<DestructibleClassB>(43, destruction_stack);
    memory.deallocate(value1);
    memory.deallocate(value2);

    ASSERT_EQ(destruction_stack, std::vector<int32_t>({ 42, 43 }));
}

TEST(MemoryDestructorTest, DestructsDerivedObject) {
    in_place_memory memory;
    std::vector<int32_t> destruction_stack;

    const auto* value = memory.allocate<DestructibleClassAB>(44, 45, 46, destruction_stack);
    memory.deallocate(value);

    ASSERT_EQ(destruction_stack, std::vector<int32_t>({ 44, 46, 45 }));
}

TEST(MemoryDestructorTest, DestructsDerivedObjectByFirstBaseClassPointer) {
    in_place_memory memory;
    std::vector<int32_t> destruction_stack;

    const auto* value = memory.allocate<DestructibleClassAB>(44, 45, 46, destruction_stack);
    const auto* base_a = reinterpret_cast<const DestructibleClassA*>(value);
    memory.deallocate(base_a);

    ASSERT_EQ(destruction_stack, std::vector<int32_t>({ 44, 46, 45 }));
}

TEST(MemoryDestructorTest, DestructsDerivedObjectBySecondBaseClassPointer) {
    in_place_memory memory;
    std::vector<int32_t> destruction_stack;

    const auto* value = memory.allocate<DestructibleClassAB>(44, 45, 46, destruction_stack);
    const auto* base_b = reinterpret_cast<const DestructibleClassB*>(value);
    memory.deallocate(base_b);

    ASSERT_EQ(destruction_stack, std::vector<int32_t>({ 44, 46, 45 }));
}

}