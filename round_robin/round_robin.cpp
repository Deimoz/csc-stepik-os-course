#include <cstddef>
#include <cstdint>
#include <cstdlib>

struct Node
{
    int thread_id;
    Node* next;
};

Node* queue_head;
Node* queue_tail;
int current_time = 0;
int max_timeslice;

void add_thread_to_tail(int thread_id)
{
    if (queue_head == nullptr) {
        queue_head = new Node();
        queue_tail = queue_head;
    } else {
        queue_tail->next = new Node();
        queue_tail = queue_tail->next;
    }
    queue_tail->thread_id = thread_id;
    queue_tail->next = nullptr;
}

int remove_thread_from_head() {
    current_time = 0;
    if (queue_head == nullptr) {
        return -1;
    }

    int thread_id = queue_head->thread_id;
    queue_head = queue_head->next;
    return thread_id;
}

/**
 * Функция будет вызвана перед каждым тестом, если вы
 * используете глобальные и/или статические переменные
 * не полагайтесь на то, что они заполнены 0 - в них
 * могут храниться значения оставшиеся от предыдущих
 * тестов.
 *
 * timeslice - квант времени, который нужно использовать.
 * Поток смещается с CPU, если пока он занимал CPU функция
 * timer_tick была вызвана timeslice раз.
 **/
void scheduler_setup(int timeslice)
{
    max_timeslice = timeslice;
    queue_head = nullptr;
    queue_tail = nullptr;
}

/**
 * Функция вызывается, когда создается новый поток управления.
 * thread_id - идентификатор этого потока и гарантируется, что
 * никакие два потока не могут иметь одинаковый идентификатор.
 **/
void new_thread(int thread_id)
{
    add_thread_to_tail(thread_id);
}

/**
 * Функция вызывается, когда поток, исполняющийся на CPU,
 * завершается. Завершится может только поток, который сейчас
 * исполняется, поэтому thread_id не передается. CPU должен
 * быть отдан другому потоку, если есть активный
 * (незаблокированный и незавершившийся) поток.
 **/
void exit_thread()
{
    remove_thread_from_head();
}

/**
 * Функция вызывается, когда поток, исполняющийся на CPU,
 * блокируется. Заблокироваться может только поток, который
 * сейчас исполняется, поэтому thread_id не передается. CPU
 * должен быть отдан другому активному потоку, если таковой
 * имеется.
 **/
void block_thread()
{
    remove_thread_from_head();
}

/**
 * Функция вызывается, когда один из заблокированных потоков
 * разблокируется. Гарантируется, что thread_id - идентификатор
 * ранее заблокированного потока.
 **/
void wake_thread(int thread_id)
{
    add_thread_to_tail(thread_id);
}

/**
 * Ваш таймер. Вызывается каждый раз, когда проходит единица
 * времени.
 **/
void timer_tick()
{
    if (queue_head == nullptr) {
        return;
    }

    current_time++;
    if (current_time == max_timeslice) {
        add_thread_to_tail(remove_thread_from_head());
    }
}

/**
 * Функция должна возвращать идентификатор потока, который в
 * данный момент занимает CPU, или -1 если такого потока нет.
 * Единственная ситуация, когда функция может вернуть -1, это
 * когда нет ни одного активного потока (все созданные потоки
 * либо уже завершены, либо заблокированы).
 **/
int current_thread()
{
    if (queue_head == nullptr) {
        return -1;
    
    }
    return queue_head->thread_id;
}
