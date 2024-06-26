#pragma once
#include <google/protobuf/message_lite.h>
#include "net_worker.hpp"
#include "console_reader.hpp"

class msg_send_guard
{
public:
    msg_send_guard(int32_t pointer_id, uint16_t msg_id, net_worker& net_worker, google::protobuf::MessageLite& message)
        : m_pointer_id(pointer_id)
        , m_msg_id(msg_id)
        , m_worker_ptr(&net_worker)
        , m_message(message)
    {
    }
    ~msg_send_guard()
    {
        void* ptr = m_data_packet.write_data(nullptr, m_message.ByteSizeLong());
        if (m_message.SerializePartialToArray(ptr, m_message.ByteSizeLong()))
        {
            std::cout << "byte size:" << m_message.ByteSizeLong() << "\tsize:" << m_data_packet.size() << std::endl;
            m_worker_ptr->send(m_pointer_id, m_msg_id, m_data_packet.get_mem_ptr(), m_data_packet.size());
        }
    }

private:
    int32_t m_pointer_id;
    uint16_t m_msg_id;
    net_worker* m_worker_ptr;
    google::protobuf::MessageLite& m_message;
    data_packet m_data_packet;
};

#define SEND_GUARD(POINTER_ID, NET_WORKER, NAME) NAME reply; \
	msg_send_guard send_guard(POINTER_ID, EnumDefine::EMsgCmd::EMC_##NAME, NET_WORKER, reply)

#define RECV_PRASE(MSG_PTR, NAME, MSG_SIZE) \
	NAME msg; \
	if (!msg.ParsePartialFromArray(MSG_PTR, MSG_SIZE)) \
	{ return false; } 
