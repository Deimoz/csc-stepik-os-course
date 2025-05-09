#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>

void *my_buf;
uint8_t* buf_start;
uint8_t* buf_end;

std::size_t additional_size = sizeof(uint8_t) * 2 + sizeof(std::size_t) * 2;

void* set_bool_mask(void *block, std::size_t block_size, uint8_t bool_mask)
{
    uint8_t* block_addr = static_cast<uint8_t*>(block);
    uint8_t* block_end_addr = block_addr + block_size - sizeof(uint8_t);
    //std::cout << "Setting bit mask " << bool_mask << " for " << block << " at " << static_cast<void*>(block_addr) << " and " << static_cast<void*>(block_end_addr) << "\n";
    *block_addr = bool_mask;
    *block_end_addr = bool_mask;
}

void* set_data_to_block(void *p, std::size_t block_size, uint8_t bool_mask)
{
    set_bool_mask(p, block_size, bool_mask);
    uint8_t* p_addr = static_cast<uint8_t*>(p);
    uint8_t* p_end_addr = p_addr + block_size - sizeof(uint8_t);
    std::size_t* p_size_ptr = reinterpret_cast<std::size_t*>(p_addr + sizeof(uint8_t));
    std::size_t* p_size_end_ptr = reinterpret_cast<std::size_t*>(p_end_addr - sizeof(std::size_t));
    std::size_t size = block_size - additional_size;
    //std::cout << "Setting size " << size << " for " << p << " at " << static_cast<void*>(p_size_ptr) << " and " << static_cast<void*>(p_size_end_ptr) << "\n";
    *p_size_ptr = size;
    *p_size_end_ptr = size;
}

// Эта функция будет вызвана перед тем как вызывать myalloc и myfree
// используйте ее чтобы инициализировать ваш аллокатор перед началом
// работы.
//
// buf - указатель на участок логической памяти, который ваш аллокатор
//       должен распределять, все возвращаемые указатели должны быть
//       либо равны NULL, либо быть из этого участка памяти
// size - размер участка памяти, на который указывает buf
void mysetup(void *buf, std::size_t size)
{
    my_buf = buf;
    buf_start = static_cast<uint8_t*>(buf);
    buf_end = buf_start + size;
    set_data_to_block(buf, size, 0);
}

bool is_in_buf(void *p) {
    if (p == nullptr) {
        return false;
    }
    uint8_t* p_addr = static_cast<uint8_t*>(p);
    return p_addr >= buf_start && p_addr < buf_end;
}

bool is_empty(void *p) {
    if (p == nullptr) {
        return false;
    }
    uint8_t* p_addr = static_cast<uint8_t*>(p);
    return (*p_addr & 1) == 0;
}

std::size_t get_size(void *p) {
    if (p == nullptr) {
        return 0;
    }
    uint8_t* p_addr = static_cast<uint8_t*>(p);
    std::size_t* size_addr = reinterpret_cast<std::size_t*>(p_addr + sizeof(uint8_t));
    return *size_addr;
}

void* next_block(void *p) {
    if (p == nullptr) {
        return my_buf;
    }
    uint8_t* p_addr = static_cast<uint8_t*>(p);
    std::size_t p_size = get_size(p);
    return p_addr + p_size + additional_size;
}

// Функция аллокации
void *myalloc(std::size_t size)
{
    //std::cout << "Allocating size " << size << "\n";
    void* curr = nullptr;
    bool found = false;
    do {
        if (is_empty(curr) && get_size(curr) >= size) {
            found = true;
            break;
        }
        curr = next_block(curr);
    } while (curr != nullptr && is_in_buf(curr));
    
    if (!found) {
        //std::cout << "No suitable block found \n";
        return NULL;
    }
    
    std::size_t curr_size = get_size(curr);
    std::size_t full_block_size = size + additional_size;

    //std::cout << "Found block at " << curr << " with size " << curr_size << "\n";

    // cant split because no room for metadata
    if (curr_size <= full_block_size) {
        //std::cout << "Cannot split block " << curr << " with size " << curr_size << " for size " << size << "\n";
        set_bool_mask(curr, curr_size + additional_size, 1);
        return static_cast<uint8_t*>(curr) + sizeof(uint8_t) + sizeof(std::size_t);
    }
    
    // setting current block new size
    set_data_to_block(curr, full_block_size, 1);
    
    // splitting block and setting new meta for next block
    uint8_t* curr_addr = static_cast<uint8_t*>(curr);
    uint8_t* new_next_addr = curr_addr + full_block_size;
    //std::cout << "Splitting block starting at " << static_cast<void*>(new_next_addr) << "\n";
    set_data_to_block(new_next_addr, curr_size - full_block_size + additional_size, 0);

    return static_cast<uint8_t*>(curr) + sizeof(uint8_t) + sizeof(std::size_t);
}

// Функция освобождения
void myfree(void *p)
{
    if (p == nullptr) {
        return;
    }
    uint8_t* p_block_addr = static_cast<uint8_t*>(p) - sizeof(uint8_t) - sizeof(std::size_t);
    std::size_t size = get_size(p_block_addr);
    //std::cout << "Freeing " << p << " with block starting at " << static_cast<void*>(p_block_addr) << " with size " << size << "\n";
    set_bool_mask(p_block_addr, size + additional_size, 0);

    if (buf_start < p_block_addr && is_empty(p_block_addr - sizeof(uint8_t))) {
        std::size_t prev_size = *reinterpret_cast<std::size_t*>(p_block_addr - sizeof(uint8_t) - sizeof(std::size_t));
        //std::cout << "Previous block for " << static_cast<void*>(p_block_addr) << " with size " << prev_size << " is free\n";
        uint8_t* prev_addr = p_block_addr - prev_size - additional_size;
        set_data_to_block(prev_addr, prev_size + size + additional_size * 2, 0);
        p_block_addr = reinterpret_cast<uint8_t*>(prev_addr);
        size = prev_size + size + additional_size;
        //std::cout << "New address is " << static_cast<void*>(p_block_addr) << " with size " << size << "\n";
    }
    
    uint8_t* p_end_addr = p_block_addr + size + additional_size;
    if (p_end_addr < buf_end && is_empty(p_end_addr)) {
        std::size_t next_size = *reinterpret_cast<std::size_t*>(p_end_addr + sizeof(uint8_t));
        //std::cout << "Next block for " << static_cast<void*>(p_block_addr) << " with size " << next_size << " is free\n";
        set_data_to_block(p_block_addr, next_size + size + additional_size * 2, 0);
    }
}

int main(int argc, char const *argv[])
{
    std::size_t size = 1024;
    void* buf = malloc(size);
    std::cout << "Buffer start: " << buf << "\n";
    uint8_t* buff_end_addr = static_cast<uint8_t*>(buf) + size;
    std::cout << "Buffer end: " << static_cast<void*>(buff_end_addr) << "\n";

    mysetup(buf, size);

    void* allocated_addr_1 = myalloc(512);
    std::cout << "My allocate 1: " << allocated_addr_1 << "\n";
    std::uint8_t* block_1_addr = static_cast<uint8_t*>(allocated_addr_1) - sizeof(uint8_t) - sizeof(std::size_t);
    std::cout << "Allocate 1 block addr: " << static_cast<void*>(block_1_addr) << "\n";
    std::size_t allocated_addr_1_size = get_size(block_1_addr);
    std::cout << "My allocate 1 size: " << allocated_addr_1_size << "\n";

    void* allocated_addr_2 = myalloc(256);
    std::cout << "My allocate 2: " << allocated_addr_2 << "\n";
    std::uint8_t* block_2_addr = static_cast<uint8_t*>(allocated_addr_2) - sizeof(uint8_t) - sizeof(std::size_t);
    std::cout << "Allocate 2 block addr: " << static_cast<void*>(block_2_addr) << "\n";
    std::size_t allocated_addr_2_size = get_size(block_2_addr);
    std::cout << "My allocate 2 size: " << allocated_addr_2_size << "\n";

    void* allocated_addr_3 = myalloc(128);
    std::cout << "My allocate 3: " << allocated_addr_3 << "\n";
    std::uint8_t* block_3_addr = static_cast<uint8_t*>(allocated_addr_3) - sizeof(uint8_t) - sizeof(std::size_t);
    std::cout << "Allocate 3 block addr: " << static_cast<void*>(block_3_addr) << "\n";
    std::size_t allocated_addr_3_size = get_size(block_3_addr);
    std::cout << "My allocate 3 size: " << allocated_addr_3_size << "\n";

    void* allocated_addr_not_found = myalloc(57);
    std::cout << "My allocate not found (should be null): " << allocated_addr_not_found << "\n";

    void* allocated_addr_4 = myalloc(47);
    std::cout << "My allocate 4: " << allocated_addr_4 << "\n";
    std::uint8_t* block_4_addr = static_cast<uint8_t*>(allocated_addr_4) - sizeof(uint8_t) - sizeof(std::size_t);
    std::cout << "Allocate 4 block addr: " << static_cast<void*>(block_4_addr) << "\n";
    std::size_t allocated_addr_4_size = get_size(block_4_addr);
    std::cout << "My allocate 4 size: " << allocated_addr_4_size << "\n";

    myfree(allocated_addr_2);
    std::size_t allocated_addr_1_size_after_2_free = get_size(block_1_addr);
    std::cout << "My allocate 1 size after allocate 2 free: " << allocated_addr_1_size_after_2_free << "\n";

    myfree(allocated_addr_3);
    std::size_t allocated_addr_2_size_after_3_free = get_size(block_2_addr);
    std::cout << "My allocate 2 size after allocate 3 free: " << allocated_addr_2_size_after_3_free << "\n";

    myfree(allocated_addr_1);
    std::size_t allocated_addr_1_size_after_1_free = get_size(block_1_addr);
    std::cout << "My allocate 1 size after allocate 2 and 3 free: " << allocated_addr_1_size_after_1_free << "\n";

    myfree(allocated_addr_4);
    std::size_t allocated_addr_1_size_after_4_free = get_size(block_1_addr);
    std::cout << "My allocate 1 size after allocate 4 free: " << allocated_addr_1_size_after_4_free << "\n";

    return 0;
}
