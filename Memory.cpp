#pragma once

#include "Types.h"
#include <algorithm>
#include <corecrt_malloc.h>

#ifndef max
#define max(a, b) a > b? a: b
#endif

#define byteOffset(ptr, count) ((void*)(((u8*)ptr) + count))

const umm MEMORY_ALIGNMENT = 4;
const umm MIN_BLOCK_SIZE = 1024 * 1024;

struct MemoryBlock {
    MemoryBlock* next;
    void* head;
    umm size;
    umm free;
};

const umm MemoryBlockDataOffset = sizeof(MemoryBlock);

struct MemoryArena {
    MemoryBlock* first;
    MemoryBlock* last;
};

MemoryBlock*
memoryArenaAllocateBlock(umm size) {
    size = max(MIN_BLOCK_SIZE, size);
    void* data = malloc(size);

    auto* block = (MemoryBlock*)data;
    block->next = nullptr;
    block->head = byteOffset(data, MemoryBlockDataOffset);
    block->size = size;
    block->free = size - MemoryBlockDataOffset;

    return block;
}

bool
memoryArenaTryAllocateFromBlock(MemoryBlock* block, umm size, void** data) {
    if (size % MEMORY_ALIGNMENT != 0) size = ((size / MEMORY_ALIGNMENT) + 1) * MEMORY_ALIGNMENT;
    if (block->free < size) return false;

    *data = block->head;
    block->head = byteOffset(block->head, size);
    block->free -= size;

    return true;
}

void*
memoryArenaAllocate(MemoryArena* arena, umm size) {
    if (arena->first == nullptr) {
        arena->first = memoryArenaAllocateBlock(size);
        arena->last = arena->first;
    }

    void* data = nullptr;

    if (!memoryArenaTryAllocateFromBlock(arena->last, size, &data)) {
        MemoryBlock* newBlock = memoryArenaAllocateBlock(size);
        arena->last->next = newBlock;
        memoryArenaTryAllocateFromBlock(arena->last, size, &data);
    }

    return data;
}

void
memoryArenaClear(MemoryArena* arena) {
    MemoryBlock* block = arena->first;

    do {
        MemoryBlock* next = block->next;
        free(block);
        block = next;
    } while (block != nullptr);

    arena->first = nullptr;
    arena->last = nullptr;
}
