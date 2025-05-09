#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>

struct Node {
    std::size_t size;
    bool is_empty;
    Node *next;
    Node *prev;
};

Node* head;

void join_node_with_next(Node* curr)
{
    if (curr->next == nullptr || !curr->next->is_empty) {
        return;
    }

    curr->size += curr->next->size + sizeof(Node);
    curr->next = curr->next->next;
    if (curr->next != nullptr) {
        curr->next->prev = curr;
    }
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
    head = static_cast<Node *>(buf);
    head->size = size - sizeof(Node);
    head->is_empty = true;
    head->next = nullptr;
    head->prev = nullptr;
}

// Функция аллокации
void *myalloc(std::size_t size)
{
    Node* curr = head;
    while (curr != nullptr) {
        if (curr->size >= size && curr->is_empty) {
            break;
        }
        curr = curr->next;
    }

    if (curr == nullptr) {
        return NULL;
    }

    // Размер текущего блока памяти больше, чем мы алоцируем + размер хедера, а значит есть место под новый блок
    if (curr->size > size + sizeof(Node)) {
        // Берем адрес начала текущего блока памяти и двигаем его на size байт
        Node* new_node = reinterpret_cast<Node *>(reinterpret_cast<uint8_t *>(curr + 1) + size);

        // Отделяем от текущего блока памяти новый свободный блок памяти
        // [block____________, next_____] -> [block_____, new_block_____, next_____]
        new_node->is_empty = true;
        new_node->size = curr->size - sizeof(Node) - size;
        new_node->next = curr->next;
        new_node->prev = curr;

        if (curr->next != nullptr) {
            curr->next->prev = new_node;
        }

        curr->size = size;
        curr->next = new_node;
    }

    // Занимаем текущий блок памяти и отдаем указатель на начало данных
    curr->is_empty = false;
    return curr + 1;
}

// Функция освобождения
void myfree(void *p)
{
    Node* curr = head;
    while (curr != nullptr && static_cast<void*>(curr + 1) != p) {
        curr = curr->next;
    }
    
    // Не нашли такой блок
    if (curr == nullptr) {
        return;
    }

    curr->is_empty = true;

    // Соединяем текущий блок памяти со следующим, если он пустой
    join_node_with_next(curr);
    if (curr->prev != nullptr && curr->prev->is_empty) {
        // Соединяем предыдущий блок памяти с текущим, если он пустой
        join_node_with_next(curr->prev);
    }
}

std::size_t get_size(void *p) {
    if (p == nullptr) {
        return 0;
    }
    Node* node = static_cast<Node *>(p);
    return (node - 1)->size;
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
    std::size_t allocated_addr_1_size = get_size(allocated_addr_1);
    std::cout << "My allocate 1 size: " << allocated_addr_1_size << "\n";

    void* allocated_addr_2 = myalloc(256);
    std::cout << "My allocate 2: " << allocated_addr_2 << "\n";
    std::size_t allocated_addr_2_size = get_size(allocated_addr_2);
    std::cout << "My allocate 2 size: " << allocated_addr_2_size << "\n";

    void* allocated_addr_3 = myalloc(128);
    std::cout << "My allocate 3: " << allocated_addr_3 << "\n";
    std::size_t allocated_addr_3_size = get_size(allocated_addr_3);
    std::cout << "My allocate 3 size: " << allocated_addr_3_size << "\n";

    void* allocated_addr_not_found = myalloc(128);
    std::cout << "My allocate not found (should be null): " << allocated_addr_not_found << "\n";

    void* allocated_addr_4 = myalloc(47);
    std::cout << "My allocate 4: " << allocated_addr_4 << "\n";
    std::size_t allocated_addr_4_size = get_size(allocated_addr_4);
    std::cout << "My allocate 4 size: " << allocated_addr_4_size << "\n";

    myfree(allocated_addr_2);
    std::size_t allocated_addr_1_size_after_2_free = get_size(allocated_addr_1);
    std::cout << "My allocate 1 size after allocate 2 free: " << allocated_addr_1_size_after_2_free << "\n";

    myfree(allocated_addr_3);
    std::size_t allocated_addr_1_size_after_3_free = get_size(allocated_addr_1);
    std::cout << "My allocate 1 size after allocate 3 free: " << allocated_addr_1_size_after_3_free << "\n";

    myfree(allocated_addr_1);
    std::size_t allocated_addr_1_size_after_1_free = get_size(allocated_addr_1);
    std::cout << "My allocate 1 size after allocate 2 and 3 free: " << allocated_addr_1_size_after_1_free << "\n";

    myfree(allocated_addr_4);
    std::size_t allocated_addr_1_size_after_4_free = get_size(allocated_addr_1);
    std::cout << "My allocate 1 size after allocate 4 free: " << allocated_addr_1_size_after_4_free << "\n";

    return 0;
}
