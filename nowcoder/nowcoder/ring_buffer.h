#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <mutex>
using namespace std;

//判断x是否是2的次方
#define is_power_of_2(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))
//取a和b中最小值
#define min(a, b) (((a) < (b)) ? (a) : (b))

struct ring_buffer
{
    void* buffer;//缓冲区
    uint32_t size;//大小
    uint32_t in;//入口位置
    uint32_t out;//出口位置
    mutex *f_lock;//互斥锁
};
//初始化缓冲区
struct ring_buffer* ring_buffer_init(void* buffer, uint32_t size, mutex* f_lock)
{
    assert(buffer);
    struct ring_buffer* ring_buf = NULL;
    if (!is_power_of_2(size))
    {
        fprintf(stderr, "size must be power of 2.\n");
        return ring_buf;
    }
    ring_buf = (struct ring_buffer*)malloc(sizeof(struct ring_buffer));
    if (!ring_buf)
    {
        fprintf(stderr, "Failed to malloc memory.");
        return ring_buf;
    }
    memset(ring_buf, 0, sizeof(struct ring_buffer));
    ring_buf->buffer = buffer;
    ring_buf->size = size;
    ring_buf->in = 0;
    ring_buf->out = 0;
    ring_buf->f_lock = f_lock;
    return ring_buf;
}
//释放缓冲区
void ring_buffer_free(struct ring_buffer* ring_buf)
{
    if (ring_buf)
    {
        if (ring_buf->buffer)
        {
            free(ring_buf->buffer);
            ring_buf->buffer = NULL;
        }
        free(ring_buf);
        ring_buf = NULL;
    }
}

//缓冲区的长度
uint32_t __ring_buffer_len(const struct ring_buffer* ring_buf)
{
    return (ring_buf->in - ring_buf->out);
}

//从缓冲区中取数据
uint32_t __ring_buffer_get(struct ring_buffer* ring_buf, void* buffer, uint32_t size)
{
    assert(ring_buf || buffer);
    uint32_t len = 0;
    size = min(size, ring_buf->in - ring_buf->out);
    /* first get the data from fifo->out until the end of the buffer */
    //size参数是2的幂，对kfifo->size取模运算可以转化为与运算，kfifo->in % kfifo->size
    len = min(size, ring_buf->size - (ring_buf->out & (ring_buf->size - 1)));
    memcpy(buffer, &(ring_buf->buffer) + (ring_buf->out & (ring_buf->size - 1)), len);
    /* then get the rest (if any) from the beginning of the buffer */
    memcpy(&buffer + len, ring_buf->buffer, size - len);
    ring_buf->out += size;
    return size;
}
//向缓冲区中存放数据
uint32_t __ring_buffer_put(struct ring_buffer* ring_buf, void* buffer, uint32_t size)
{
    assert(ring_buf || buffer);
    uint32_t len = 0;
    //判断剩余空间能否存入新数据
    size = min(size, ring_buf->size - ring_buf->in + ring_buf->out);
    /* first put the data starting from fifo->in to buffer end */
    //in和out定义为无符号类型，在put和get时，in和out都是增加，
    //当达到最大值时，产生溢出，使得从0开始，进行循环使用
    len = min(size, ring_buf->size - (ring_buf->in & (ring_buf->size - 1)));
    memcpy(&(ring_buf->buffer) + (ring_buf->in & (ring_buf->size - 1)), buffer, len);
    /* then put the rest (if any) at the beginning of the buffer */
    memcpy(ring_buf->buffer, &buffer + len, size - len);
    ring_buf->in += size;
    return size;
}

uint32_t ring_buffer_len(const struct ring_buffer* ring_buf)
{
    uint32_t len = 0;
    ring_buf->f_lock->lock();
    len = __ring_buffer_len(ring_buf);
    ring_buf->f_lock->unlock();
    return len;
}

uint32_t ring_buffer_get(struct ring_buffer* ring_buf, void* buffer, uint32_t size)
{
    uint32_t ret;
    ring_buf->f_lock->lock();
    ret = __ring_buffer_get(ring_buf, buffer, size);
    //buffer中没有数据
    if (ring_buf->in == ring_buf->out)
        ring_buf->in = ring_buf->out = 0;
    ring_buf->f_lock->unlock();
    return ret;
}

uint32_t ring_buffer_put(struct ring_buffer* ring_buf, void* buffer, uint32_t size)
{
    uint32_t ret;
    ring_buf->f_lock->lock();
    ret = __ring_buffer_put(ring_buf, buffer, size);
    ring_buf->f_lock->unlock();
    return ret;
}