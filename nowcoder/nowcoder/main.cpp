#include "ring_buffer.h"
#include <iostream>
#include <thread>
#include <windows.h>

#include <mutex>
#define BUFFER_SIZE  1024 * 1024

using namespace std;

typedef struct student_info
{
    uint64_t stu_id;
    uint32_t age;
    uint32_t score;
}student_info;


void print_student_info(const student_info* stu_info)
{
    assert(stu_info);
    printf("id:%lu\t", stu_info->stu_id);
    printf("age:%u\t", stu_info->age);
    printf("score:%u\n", stu_info->score);
}

student_info* get_student_info(time_t timer)
{
    student_info* stu_info = (student_info*)malloc(sizeof(student_info));
    if (!stu_info)
    {
        fprintf(stderr, "Failed to malloc memory.\n");
        return NULL;
    }
    srand(timer);//给rand()提供种子seed,
    //如果srand每次输入的数值是一样的，那么每次运行产生的随机数也是一样的
    stu_info->stu_id = 10000 + rand() % 9999;
    stu_info->age = rand() % 30;
    stu_info->score = rand() % 101;
    print_student_info(stu_info);
    return stu_info;
}

//消费者线程
void* consumer_proc(void* arg)
{
    struct ring_buffer* ring_buf = (struct ring_buffer*)arg;
    student_info stu_info;
    while (1)
    {
        Sleep(2*1000);
        printf("------------------------------------------\n");
        printf("get a student info from ring buffer.\n");
        ring_buffer_get(ring_buf, (void*)&stu_info, sizeof(student_info));
        printf("ring buffer length: %u\n", ring_buffer_len(ring_buf));
        print_student_info(&stu_info);
        printf("------------------------------------------\n");
    }
    return (void*)ring_buf;
}

//生产者线程
void* producer_proc(void* arg)
{
    time_t cur_time;
    struct ring_buffer* ring_buf = (struct ring_buffer*)arg;
    while (1)
    {
        time(&cur_time);
        srand(cur_time);
        int seed = rand() % 11111;
        printf("******************************************\n");
        student_info* stu_info = get_student_info(cur_time + seed);
        printf("put a student info to ring buffer.\n");
        ring_buffer_put(ring_buf, (void*)stu_info, sizeof(student_info));
        printf("ring buffer length: %u\n", ring_buffer_len(ring_buf));
        printf("******************************************\n");
        Sleep(1*1000);
    }
    return (void*)ring_buf;
}

int main()
{
    void* buffer = NULL;
    uint32_t size = 0;
    struct ring_buffer* ring_buf = NULL;

    mutex f_lock;

    buffer = (void*)malloc(BUFFER_SIZE);
    if (!buffer)
    {
        fprintf(stderr, "Failed to malloc memory.\n");
        return -1;
    }
    size = BUFFER_SIZE;
    ring_buf = ring_buffer_init(buffer, size, &f_lock);
    if (!ring_buf)
    {
        fprintf(stderr, "Failed to init ring buffer.\n");
        return -1;
    }
#if 0
    student_info* stu_info = get_student_info(638946124);
    ring_buffer_put(ring_buf, (void*)stu_info, sizeof(student_info));
    stu_info = get_student_info(976686464);
    ring_buffer_put(ring_buf, (void*)stu_info, sizeof(student_info));
    ring_buffer_get(ring_buf, (void*)stu_info, sizeof(student_info));
    print_student_info(stu_info);
#endif
    printf("multi thread test.......\n");
    thread P(producer_proc,(void*)ring_buf);
    thread C(consumer_proc, (void*)ring_buf);
    P.join();
    C.join();
    ring_buffer_free(ring_buf);
    return 0;
}