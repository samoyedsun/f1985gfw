#ifndef _NET_QUEUE_H_
#define _NET_QUEUE_H_

#include <condition_variable>
#include <iostream>

class net_msg_queue
{
    public:
        typedef struct _net_msg_
        {
            struct _net_msg_ * next;
            uint32_t cid;
            uint16_t id;
            uint16_t size;
            char buffer[1];
        } net_msg_t;

    public:
        net_msg_queue()
        : m_head_ptr(NULL)
        , m_tail_ptr(NULL)
        {}
        ~net_msg_queue()
        {}

        net_msg_t *create_element(uint16_t size)
        {
            return (net_msg_t *)malloc(sizeof(net_msg_t) - 1 + size);
        }
        
        void init_element(net_msg_t *msg_ptr, uint32_t cid, uint16_t id, uint16_t size, const void *body)
        {
            msg_ptr->next = NULL;
            msg_ptr->cid = cid;
            msg_ptr->id = id;
            msg_ptr->size = size;
            if (body != NULL)
            {
                memcpy(msg_ptr->buffer, body, size);
            }
        }

        void release_element(net_msg_t *msg_ptr)
        {
            free(msg_ptr);
        }

        void enqueue(net_msg_t *msg_ptr)
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
        }

        net_msg_t* dequeue()
        {
            if (!m_head_ptr)
            {
                return NULL;
            }
            else
            {
                std::lock_guard<std::mutex> guard(m_mutex);
                net_msg_t* msg_ptr = m_head_ptr;
                m_head_ptr = NULL;
                m_tail_ptr = NULL;
                return msg_ptr;
            }
        }

        void clear()
        {
            net_msg_t *msg_ptr = NULL;
            std::lock_guard<std::mutex> guard(m_mutex);

            while (m_head_ptr)
            {
                msg_ptr = m_head_ptr;
                m_head_ptr = m_head_ptr->next;
                free(msg_ptr);
            }

            m_head_ptr = NULL;
            m_tail_ptr = NULL;
        }

    private:
        std::mutex	m_mutex;
        net_msg_t*	m_head_ptr;
        net_msg_t*	m_tail_ptr;
};


#endif