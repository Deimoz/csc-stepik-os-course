#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>

static size_t PAGE_SIZE = 4096;

/**
 * Эти две функции вы должны использовать для аллокации
 * и освобождения памяти в этом задании. Считайте, что
 * внутри они используют buddy аллокатор с размером
 * страницы равным 4096 байтам.
 **/

/**
 * Аллоцирует участок размером 4096 * 2^order байт,
 * выровненный на границу 4096 * 2^order байт. order
 * должен быть в интервале [0; 10] (обе границы
 * включительно), т. е. вы не можете аллоцировать больше
 * 4Mb за раз.
 **/
void *alloc_slab(int order)
{
    size_t size = PAGE_SIZE * (1 << order);
    return aligned_alloc(size, size);
}

/**
 * Освобождает участок ранее аллоцированный с помощью
 * функции alloc_slab.
 **/
void free_slab(void *slab) { std::free(slab); }

struct slab_object_header
{
    slab_object_header *next_object;
};

struct slab_header
{
    // Двусвязный список для эффективного удаления и добавления в разные списки slab-ов
    slab_header *next;
    slab_header *prev;
    slab_object_header *next_free_object;
    size_t free_slabs_count;
};

/**
 * Эта структура представляет аллокатор, вы можете менять
 * ее как вам удобно. Приведенные в ней поля и комментарии
 * просто дают общую идею того, что вам может понадобится
 * сохранить в этой структуре.
 **/
struct cache
{
    slab_header *empty_slabs;
    slab_header *active_slabs;
    slab_header *full_slabs;

    size_t object_size;  /* размер аллоцируемого объекта */
    int slab_order;      /* используемый размер SLAB-а */
    size_t slab_objects; /* количество объектов в одном SLAB-е */
    size_t slab_size;
};

size_t calc_slab_objects(size_t slab_size, size_t object_size)
{
    return (slab_size - sizeof(slab_header)) /
           (object_size + sizeof(slab_object_header));
}

int calc_slab_order(size_t object_size)
{
    size_t slab_size = PAGE_SIZE;
    for (int i = 0; i < 10; i++)
    {
        size_t objects_count = calc_slab_objects(slab_size, object_size);
        if (objects_count >= 10)
        {
            break;
        }
        slab_size *= 2;
    }
    return slab_size;
}

size_t calc_slab_size(int slab_order) { return PAGE_SIZE * (1 << slab_order); }

slab_header *get_slab_ptr(struct cache *cache, void *ptr)
{
    uintptr_t ptr_as_int = reinterpret_cast<uintptr_t>(ptr);
    ptr_as_int &= ~(cache->slab_size - 1);
    return reinterpret_cast<slab_header *>(ptr_as_int);
}

slab_header *alloc_new_slab(struct cache *cache)
{
    slab_header *slab = static_cast<slab_header *>(alloc_slab(cache->slab_order));
    slab->free_slabs_count = cache->slab_objects;
    slab->next = nullptr;
    slab->prev = nullptr;

    slab_object_header *curr_free_object = reinterpret_cast<slab_object_header *>(slab + 1);
    slab->next_free_object = curr_free_object;

    uint8_t *byte_addr = reinterpret_cast<uint8_t *>(curr_free_object);
    for (size_t i = 1; i < cache->slab_objects; i++)
    {
        byte_addr += sizeof(slab_object_header) + cache->object_size;
        slab_object_header *next_free_object = reinterpret_cast<slab_object_header *>(byte_addr);
        curr_free_object->next_object = next_free_object;
        curr_free_object = next_free_object;
    }

    return slab;
}

void remove_slab_from_list(slab_header *slab)
{
    if (slab->prev != nullptr)
    {
        slab->prev->next = slab->next;
    }
    if (slab->next != nullptr)
    {
        slab->next->prev = slab->prev;
    }
}

/**
 * Вставляет slab в связанный список перед next_slab.
 */
void add_slab_before_next_slab(slab_header *slab, slab_header *next_slab)
{
    slab->next = next_slab;
    slab->prev = nullptr;
    if (slab->next != nullptr)
    {
        slab->prev = slab->next->prev;
        slab->next->prev = slab;
    }
}

/**
 * Функция инициализации будет вызвана перед тем, как
 * использовать это кеширующий аллокатор для аллокации.
 * Параметры:
 *  - cache - структура, которую вы должны инициализировать
 *  - object_size - размер объектов, которые должен
 *    аллоцировать этот кеширующий аллокатор
 **/
void cache_setup(struct cache *cache, size_t object_size)
{
    cache->empty_slabs = nullptr;
    cache->active_slabs = nullptr;
    cache->full_slabs = nullptr;

    cache->object_size = object_size;
    cache->slab_order = calc_slab_order(cache->object_size);
    cache->slab_size = calc_slab_size(cache->slab_order);
    cache->slab_objects = calc_slab_objects(cache->slab_size, cache->object_size);
    std::cout << "Initialized cache: object_size = " << cache->object_size
              << ", slab_order = " << cache->slab_order
              << ", slab_size = " << cache->slab_size
              << ", slab_objects = " << cache->slab_objects
              << "\n";
}

/**
 * Освобождает все slab-ы в списке, начиная с переданного slab-а
 */
void free_cached_slabs(slab_header *slab)
{
    std::cout << "Freeing cached slabs starting from " << static_cast<void*>(slab) << "\n";
    slab_header *curr_slab = slab;
    while (curr_slab != nullptr)
    {
        slab_header *next_slab = curr_slab->next;
        curr_slab->next = nullptr;
        free_slab(curr_slab);
        std::cout << "Free " << static_cast<void*>(curr_slab) << "\n";
        curr_slab = next_slab;
    }
}

/**
 * Функция освобождения будет вызвана когда работа с
 * аллокатором будет закончена. Она должна освободить
 * всю память занятую аллокатором. Проверяющая система
 * будет считать ошибкой, если не вся память будет
 * освбождена.
 **/
void cache_release(struct cache *cache)
{
    free_cached_slabs(cache->empty_slabs);
    cache->empty_slabs = nullptr;
    free_cached_slabs(cache->active_slabs);
    cache->active_slabs = nullptr;
    free_cached_slabs(cache->full_slabs);
    cache->full_slabs = nullptr;
}

/**
 * Возвращает slab, из которого будет производиться аллокация.
 * В приоритете берется активный slab, после чего пытается вернуться empty_slab.
 * Если нет slab-ов, то будет создан новый.
 */
slab_header *get_slab_to_alloc(struct cache *cache)
{
    if (cache->active_slabs != nullptr)
    {
        std::cout << "Getting active slab for object allocation " << static_cast<void*>(cache->active_slabs) << "\n";
        return cache->active_slabs;
    }
    if (cache->empty_slabs != nullptr)
    {
        std::cout << "Getting empty slab for object allocation " << static_cast<void*>(cache->empty_slabs) << "\n";
        return cache->empty_slabs;
    }
    std::cout << "Allocating new slab for object allocation\n";
    return alloc_new_slab(cache);
}

/**
 * Функция аллокации памяти из кеширующего аллокатора.
 * Должна возвращать указатель на участок памяти размера
 * как минимум object_size байт (см cache_setup).
 * Гарантируется, что cache указывает на корректный
 * инициализированный аллокатор.
 **/
void *cache_alloc(struct cache *cache)
{
    std::cout << "Allocating new object\n";
    slab_header *slab = get_slab_to_alloc(cache);
    std::cout << "Allocated slab for object: " << static_cast<void*>(slab) << " with empty objects count " << slab->free_slabs_count << "\n";
    slab_object_header *free_object = slab->next_free_object;

    slab->next_free_object = free_object->next_object;
    bool was_empty_slab = slab->free_slabs_count == cache->slab_objects;

    if (was_empty_slab)
    {
        // Перестали быть пустыми
        remove_slab_from_list(slab);
        std::cout << "Removed slab from empty list\n";
        if (cache->empty_slabs == slab)
        {
            // мы первый элемент - двигаем список
            cache->empty_slabs = slab->next;
            std::cout << "Empty slab was start of list - remove from cached head\n";
        }
    }

    slab->free_slabs_count--;
    if (slab->free_slabs_count == 0)
    {
        std::cout << "Slab became full\n";
        // стали полностью заполненными
        if (!was_empty_slab)
        {
            // перестали быть частично заполнеными
            remove_slab_from_list(slab);
            std::cout << "Allocating slab from active list\n";
            if (cache->active_slabs == slab)
            {
                // мы первый элемент - двигаем список
                cache->active_slabs = slab->next;
                std::cout << "Active slab was start of list - remove from cached head\n";
            }
        }
        add_slab_before_next_slab(slab, cache->full_slabs);
        cache->full_slabs = slab;
    } else if (was_empty_slab) {
        // Стали именно активными
        add_slab_before_next_slab(slab, cache->active_slabs);
        cache->active_slabs = slab;
    }

    return free_object + 1;
}

/**
 * Функция освобождения памяти назад в кеширующий аллокатор.
 * Гарантируется, что ptr - указатель ранее возвращенный из
 * cache_alloc.
 **/
void cache_free(struct cache *cache, void *ptr)
{
    // ptr указывает на свободную память, значит нужно сдвинуться назад на размер
    // хедера
    slab_object_header *slab_object = static_cast<slab_object_header *>(ptr) - 1;

    slab_header *slab = get_slab_ptr(cache, ptr);
    slab_object->next_object = slab->next_free_object;
    slab->next_free_object = slab_object;

    bool was_full = slab->free_slabs_count == 0;
    slab->free_slabs_count++;

    if (was_full)
    {
        // были полностью заняты
        remove_slab_from_list(slab);
        if (cache->full_slabs == slab)
        {
            // мы первый элемент - двигаем список
            cache->full_slabs = slab->next;
        }
    }

    if (slab->free_slabs_count == cache->slab_objects)
    {
        // стали пустыми
        if (!was_full)
        {
            // перестали быть частично заполнеными
            remove_slab_from_list(slab);
            if (cache->active_slabs == slab)
            {
                // мы первый элемент - двигаем список
                cache->active_slabs = slab->next;
            }
        }
        add_slab_before_next_slab(slab, cache->empty_slabs);
        cache->empty_slabs = slab;
    } else if (was_full) {
        // Стали именно активными
        add_slab_before_next_slab(slab, cache->active_slabs);
        cache->active_slabs = slab;
    }
}

/**
 * Функция должна освободить все SLAB, которые не содержат
 * занятых объектов. Если SLAB не использовался для аллокации
 * объектов (например, если вы выделяли с помощью alloc_slab
 * память для внутренних нужд вашего алгоритма), то освбождать
 * его не обязательно.
 **/
void cache_shrink(struct cache *cache)
{
    free_cached_slabs(cache->empty_slabs);
    cache->empty_slabs = nullptr;
}

int main(int argc, char const *argv[])
{
    struct cache *cache = (struct cache *)std::malloc(sizeof(struct cache));
    cache_setup(cache, 512);

    cache_release(cache);

    return 0;
}
