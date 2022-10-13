#ifndef _NET_SESSION_QUEUE_H_
#define _NET_SESSION_QUEUE_H_

#include <mutex>

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

class net_session_queue
{
    public:
        typedef struct _net_session_
        {
            int cid;
            bool processing;
            struct _net_session_ *next;
            net_msg_queue msg_queue;
        } net_session_t;

    public:
        net_session_queue()
            : m_head(nullptr)
            , m_tail(nullptr)

        {
        }
        ~net_session_queue()
        {
            if (m_head == nullptr)
            {
                return;
            }
            while (m_head)
            {
                net_session_t *ptr = m_head->next;
                release(m_head);
                m_head = ptr;
            }
        }

    public:
        net_session_t *create(int cid)
        {
            net_session_t *ptr = new net_session_t;
            ptr->cid = cid;
            ptr->processing = false;
            ptr->next = nullptr;
            return ptr;
        }

        void release(net_session_t *ptr)
        {
            delete ptr;
        }

        void push(net_session_t *ptr)
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            
            if (m_tail == nullptr)
            {
                m_head = m_tail = ptr;
            }
            else
            {
                m_tail->next = ptr;
                m_tail = ptr;
            }
        }

        net_session_t *pop()
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            
            net_session_t *ptr = m_head;
            if (m_head != nullptr)
            {
                m_head = ptr->next;
                if (m_head == nullptr)
                {
                    m_tail = m_head;
                }
            }
            return ptr;
        }
        
    private:
        std::mutex	m_mutex;
	    net_session_t *m_head;
	    net_session_t *m_tail;
};

#endif