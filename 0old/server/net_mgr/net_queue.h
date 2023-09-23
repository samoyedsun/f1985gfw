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
        struct net_msg_t
        {
            struct net_msg_t * next;
            uint8_t type;
            uint16_t id;
            uint16_t size;
            char buffer[1];
        };

    public:
        net_msg_queue();
        ~net_msg_queue();

        static net_msg_t *create(uint16_t size);
        static void release(net_msg_t *msg_ptr);

        void enqueue(net_msg_t *msg_ptr, std::function<void()> after);
        net_msg_t* dequeue(std::function<void()> after);
        int32_t count();

    private:
        std::mutex	m_mutex;
        net_msg_t*	m_head_ptr;
        net_msg_t*	m_tail_ptr;
};

class net_session_queue
{
    public:
        struct net_session_t
        {
            void *owner_ptr;
            bool processing;
            struct net_session_t *next;
            net_msg_queue msg_queue;
        };

    public:
        net_session_queue();
        ~net_session_queue();
        
        static net_session_t *create();
        static void release(net_session_t *ptr);

        void enqueue(net_session_t *ptr);
        net_session_t *dequeue();
        int32_t count();
        
    private:
        std::mutex	m_mutex;
	    net_session_t *m_head_ptr;
	    net_session_t *m_tail_ptr;
};

#endif