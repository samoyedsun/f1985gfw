#ifndef _NET_QUEUE_H_
#define _NET_QUEUE_H_

#include <iostream>
#include <mutex>
#include <functional>

class net_session_queue;
class net_msg_queue
{
    public:
        enum ENetMsgType
        {
            ENMT_None   = 0,
            ENMT_Read   = 1,
            ENMT_Write  = 2,
            ENMT_Max    = 3,
        };
        using net_msg_t = struct _net_msg_
        {
            struct _net_msg_ * next;
            uint8_t type;
            uint16_t id;
            uint16_t size;
            char buffer[1];
        };

    public:
        net_msg_queue()
        : m_head_ptr(nullptr)
        , m_tail_ptr(nullptr)
        {}
        ~net_msg_queue()
        {}

        static net_msg_t *create_element(uint16_t size)
        {
            return (net_msg_t *)malloc(sizeof(net_msg_t) - 1 + size);
        }
        
        static void init_element(net_msg_t *msg_ptr, uint8_t type, uint16_t id, uint16_t size, const void *body)
        {
            msg_ptr->next = nullptr;
            msg_ptr->type = type;
            msg_ptr->id = id;
            msg_ptr->size = size;
            if (body != nullptr)
            {
                memcpy(msg_ptr->buffer, body, size);
            }
        }

        static void release_element(net_msg_t *msg_ptr)
        {
            free(msg_ptr);
        }

        void enqueue(net_msg_t *msg_ptr, std::function<void()> after)
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

        net_msg_t* dequeue(std::function<void()> after)
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
            }
            else
            {
                after();
            }
            return ptr;
        }

        void clear()
        {
            net_msg_t *msg_ptr = nullptr;
            std::lock_guard<std::mutex> guard(m_mutex);

            while (m_head_ptr)
            {
                msg_ptr = m_head_ptr;
                m_head_ptr = m_head_ptr->next;
                free(msg_ptr);
            }

            m_head_ptr = nullptr;
            m_tail_ptr = nullptr;
        }

    private:
        std::mutex	m_mutex;
        net_msg_t*	m_head_ptr;
        net_msg_t*	m_tail_ptr;
};

class net_session_queue
{
    public:
        using net_session_t = struct _net_session_
        {
            void *owner_ptr;
            bool processing;
            struct _net_session_ *next;
            net_msg_queue msg_queue;
        };

    public:
        net_session_queue()
            : m_head_ptr(nullptr)
            , m_tail_ptr(nullptr)
        {
        }
        ~net_session_queue()
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

    public:
        static net_session_t *create()
        {
            net_session_t *ptr = new net_session_t;
            ptr->owner_ptr = nullptr;
            ptr->processing = false;
            ptr->next = nullptr;
            return ptr;
        }

        static void release(net_session_t *ptr)
        {
            delete ptr;
        }

        void enqueue(net_session_t *ptr)
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

        net_session_t *dequeue()
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
            }
            return ptr;
        }
        
    private:
        std::mutex	m_mutex;
	    net_session_t *m_head_ptr;
	    net_session_t *m_tail_ptr;
};

#endif