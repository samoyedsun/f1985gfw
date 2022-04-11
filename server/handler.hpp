#ifndef _HANDLER_H_
#define _HANDLER_H_
#include <iostream>
#include <functional>

class base_handler
{
	public:
		virtual ~base_handler(){}
		virtual void operator()(uint32_t, std::string &) {}
		virtual void operator()(uint32_t) {}
		virtual void operator()(uint32_t, uint16_t, char *, uint16_t) {}
		virtual void operator()(std::string) {}
};

class type1_handler : public base_handler
{
	public:
		using type1_t = std::function<void(uint32_t, std::string &)>;
		type1_handler(type1_t cb)
			: m_cb(cb)
		{
			
		}
		void operator()(uint32_t cid, std::string &remote_ep_data)
		{
			m_cb(cid, remote_ep_data);
		}

	private:
		type1_t m_cb;
};

class type2_handler : public base_handler
{
	public:
		using type2_t = std::function<void(uint32_t)>;
		type2_handler(type2_t cb)
			: m_cb(cb)
		{

		}
		void operator()(uint32_t cid)
		{
			m_cb(cid);
		}
	
	private:
		type2_t m_cb;
};

class type3_handler : public base_handler
{
	public:
		using type3_t = std::function<void(uint32_t, uint16_t, char *, uint16_t)>;
		type3_handler(type3_t cb)
			: m_cb(cb)
		{
		}
		void operator()(uint32_t cid, uint16_t id, char *buffer, uint16_t size)
		{
			m_cb(cid, id, buffer, size);
		}
	
	private:
		type3_t m_cb;
};

class type4_handler : public base_handler
{
	public:
		using type4_t = std::function<void(std::string)>;
		type4_handler(type4_t cb)
			: m_cb(cb)
		{
		}
		void operator()(std::string name)
		{
			m_cb(name);
		}
	
	private:
		type4_t m_cb;
};

class logic_handler
{
	public:
		logic_handler()
		{
			m_handler = NULL;
		}
		logic_handler(type1_handler::type1_t handler)
		{
			m_handler = new type1_handler(handler);
		}
		logic_handler(type2_handler::type2_t handler)
		{
			m_handler = new type2_handler(handler);
		}
		logic_handler(type3_handler::type3_t handler)
		{
			m_handler = new type3_handler(handler);
		}
		logic_handler(type4_handler::type4_t handler)
		{
			m_handler = new type4_handler(handler);
		}
		~logic_handler()
		{
			delete m_handler;
		}

		void operator()(uint32_t cid, std::string &remote_ep_data)
		{
			(*m_handler)(cid, remote_ep_data);
		}
		void operator()(uint32_t cid)
		{
			(*m_handler)(cid);
		}
		void operator()(uint32_t cid, uint16_t id, char *buffer, uint16_t size)
		{
			(*m_handler)(cid, id, buffer, size);
		}
		void operator()(std::string name)
		{
			(*m_handler)(name);
		}
		
	private:
		base_handler *m_handler;
};
#endif