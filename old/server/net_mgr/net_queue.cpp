#include "net_queue.h"


net_msg_queue::net_msg_queue()
    : m_head_ptr(nullptr)
    , m_tail_ptr(nullptr)
{

}

net_msg_queue::~net_msg_queue()
{

}

net_msg_queue::net_msg_t *net_msg_queue::create(uint16_t size)
{
    net_msg_t *ptr = (net_msg_t *)malloc(sizeof(net_msg_t) - 1 + size);
    ptr->next = nullptr;
    ptr->type = 0;
    ptr->id = 0;
    ptr->size = size;
    return ptr;
}

void net_msg_queue::release(net_msg_t *msg_ptr)
{
    free(msg_ptr);
}

void net_msg_queue::enqueue(net_msg_t *msg_ptr, std::function<void()> after)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    
    if (m_tail_ptr)
    {
        m_tail_ptr->next = msg_ptr;
        m_tail_ptr = msg_ptr;
    }
    else
    {
        m_head_ptr = m_tail_ptr = msg_ptr;
    }

    after();
}

net_msg_queue::net_msg_t* net_msg_queue::dequeue(std::function<void()> after)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    
    net_msg_t *ptr = m_head_ptr;
    if (m_head_ptr != nullptr)
    {
        m_head_ptr = ptr->next;
        if (m_head_ptr == nullptr)
        {
            m_tail_ptr = m_head_ptr;
        }
        ptr->next = nullptr;
    }
    else
    {
        after();
    }
    return ptr;
}


int32_t net_msg_queue::count()
{
    int32_t count = 0;
    std::lock_guard<std::mutex> guard(m_mutex);
    
    net_msg_t *msg_ptr = m_head_ptr;
    while (msg_ptr)
    {
        ++count;
        msg_ptr = msg_ptr->next;
    }
    return count;
}

net_session_queue::net_session_queue()
    : m_head_ptr(nullptr)
    , m_tail_ptr(nullptr)
{
}

net_session_queue::~net_session_queue()
{
    if (m_head_ptr == nullptr)
    {
        return;
    }
    while (m_head_ptr)
    {
        net_session_t *ptr = m_head_ptr->next;
        release(m_head_ptr);
        m_head_ptr = ptr;
    }
}

net_session_queue::net_session_t *net_session_queue::create()
{
    net_session_t *ptr = new net_session_t;
    ptr->owner_ptr = nullptr;
    ptr->processing = false;
    ptr->next = nullptr;
    return ptr;
}

void net_session_queue::release(net_session_t *ptr)
{
    delete ptr;
}

void net_session_queue::enqueue(net_session_t *ptr)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    
    if (m_tail_ptr == nullptr)
    {
        m_head_ptr = m_tail_ptr = ptr;
    }
    else
    {
        m_tail_ptr->next = ptr;
        m_tail_ptr = ptr;
    }
}

net_session_queue::net_session_t *net_session_queue::dequeue()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    
    net_session_t *ptr = m_head_ptr;
    if (m_head_ptr != nullptr)
    {
        m_head_ptr = ptr->next;
        if (m_head_ptr == nullptr)
        {
            m_tail_ptr = m_head_ptr;
        }
        ptr->next = nullptr;
    }
    return ptr;
}

int32_t net_session_queue::count()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    
    int32_t count = 0;
    net_session_t *msg_ptr = m_head_ptr;
    while (msg_ptr)
    {
        ++count;
        msg_ptr = msg_ptr->next;
    }
    return count;
}