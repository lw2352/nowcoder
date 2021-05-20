#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <mutex>
using namespace std;

//�ж�x�Ƿ���2�Ĵη�
#define is_power_of_2(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))
//ȡa��b����Сֵ
#define min(a, b) (((a) < (b)) ? (a) : (b))

struct ring_buffer
{
    void* buffer;//������
    uint32_t size;//��С
    uint32_t in;//���λ��
    uint32_t out;//����λ��
    mutex *f_lock;//������
};
//��ʼ��������
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
//�ͷŻ�����
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

//�������ĳ���
uint32_t __ring_buffer_len(const struct ring_buffer* ring_buf)
{
    return (ring_buf->in - ring_buf->out);
}

//�ӻ�������ȡ����
uint32_t __ring_buffer_get(struct ring_buffer* ring_buf, void* buffer, uint32_t size)
{
    assert(ring_buf || buffer);
    uint32_t len = 0;
    size = min(size, ring_buf->in - ring_buf->out);
    /* first get the data from fifo->out until the end of the buffer */
    //size������2���ݣ���kfifo->sizeȡģ�������ת��Ϊ�����㣬kfifo->in % kfifo->size
    len = min(size, ring_buf->size - (ring_buf->out & (ring_buf->size - 1)));
    memcpy(buffer, &(ring_buf->buffer) + (ring_buf->out & (ring_buf->size - 1)), len);
    /* then get the rest (if any) from the beginning of the buffer */
    memcpy(&buffer + len, ring_buf->buffer, size - len);
    ring_buf->out += size;
    return size;
}
//�򻺳����д������
uint32_t __ring_buffer_put(struct ring_buffer* ring_buf, void* buffer, uint32_t size)
{
    assert(ring_buf || buffer);
    uint32_t len = 0;
    //�ж�ʣ��ռ��ܷ����������
    size = min(size, ring_buf->size - ring_buf->in + ring_buf->out);
    /* first put the data starting from fifo->in to buffer end */
    //in��out����Ϊ�޷������ͣ���put��getʱ��in��out�������ӣ�
    //���ﵽ���ֵʱ�����������ʹ�ô�0��ʼ������ѭ��ʹ��
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
    //buffer��û������
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