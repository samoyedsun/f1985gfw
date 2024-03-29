#ifndef _NET_HTTP_H_
#define _NET_HTTP_H_

#include "fields_alloc.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <functional>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Return a reasonable mime type based on the extension of a file.
beast::string_view
mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if(pos == beast::string_view::npos)
            return beast::string_view{};
        return path.substr(pos);
    }();
    if(iequals(ext, ".htm"))  return "text/html";
    if(iequals(ext, ".html")) return "text/html";
    if(iequals(ext, ".php"))  return "text/html";
    if(iequals(ext, ".css"))  return "text/css";
    if(iequals(ext, ".txt"))  return "text/plain";
    if(iequals(ext, ".js"))   return "application/javascript";
    if(iequals(ext, ".json")) return "application/json";
    if(iequals(ext, ".xml"))  return "application/xml";
    if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if(iequals(ext, ".flv"))  return "video/x-flv";
    if(iequals(ext, ".png"))  return "image/png";
    if(iequals(ext, ".jpe"))  return "image/jpeg";
    if(iequals(ext, ".jpeg")) return "image/jpeg";
    if(iequals(ext, ".jpg"))  return "image/jpeg";
    if(iequals(ext, ".gif"))  return "image/gif";
    if(iequals(ext, ".bmp"))  return "image/bmp";
    if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if(iequals(ext, ".tiff")) return "image/tiff";
    if(iequals(ext, ".tif"))  return "image/tiff";
    if(iequals(ext, ".svg"))  return "image/svg+xml";
    if(iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

class net_http;
class http_worker
{
    using get_process_t = std::function<std::string(beast::string_view, std::string_view)>;
    using post_process_t = std::function<std::string(beast::string_view, std::string_view)>;

public:
    http_worker(http_worker const&) = delete;
    http_worker& operator=(http_worker const&) = delete;

    http_worker(tcp::acceptor& acceptor, const std::string& doc_root) :
        acceptor_(acceptor),
        doc_root_(doc_root)
    {
    }

    void set_get_process(get_process_t val)
    {
        get_process_ = val;
    }

    void set_post_process(post_process_t val)
    {
        post_process_ = val;
    }

    void start()
    {
        accept();
        check_deadline();
    }

private:
    using alloc_t = fields_alloc<char>;
    //using request_body_t = http::basic_dynamic_body<beast::flat_static_buffer<1024 * 1024>>;
    using request_body_t = http::string_body;
    
    get_process_t get_process_;
    post_process_t post_process_;

    // The acceptor used to listen for incoming connections.
    tcp::acceptor& acceptor_;

    // The path to the root of the document directory.
    std::string doc_root_;

    // The socket for the currently connected client.
    tcp::socket socket_{acceptor_.get_executor()};

    // The buffer for performing reads
    beast::flat_static_buffer<8192> buffer_;

    // The allocator used for the fields in the request and reply.
    alloc_t alloc_{8192};

    // The parser for reading the requests
    boost::optional<http::request_parser<request_body_t, alloc_t>> parser_;

    // The timer putting a time limit on requests.
    net::steady_timer request_deadline_{
        acceptor_.get_executor(), (std::chrono::steady_clock::time_point::max)()};

    // The string-based response message.
    boost::optional<http::response<http::string_body, http::basic_fields<alloc_t>>> string_response_;

    // The string-based response serializer.
    boost::optional<http::response_serializer<http::string_body, http::basic_fields<alloc_t>>> string_serializer_;

    // The file-based response message.
    boost::optional<http::response<http::file_body, http::basic_fields<alloc_t>>> file_response_;

    // The file-based response serializer.
    boost::optional<http::response_serializer<http::file_body, http::basic_fields<alloc_t>>> file_serializer_;

    void accept()
    {
        // Clean up any previous connection.
        beast::error_code ec;
        socket_.close(ec);
        buffer_.consume(buffer_.size());

        acceptor_.async_accept(
            socket_,
            [this](beast::error_code ec)
            {
                if (ec)
                {
                    accept();
                }
                else
                {
                    // Request must be fully processed within 60 seconds.
                    request_deadline_.expires_after(
                        std::chrono::seconds(60));

                    read_request();
                }
            });
    }

    void read_request()
    {
        // On each read the parser needs to be destroyed and
        // recreated. We store it in a boost::optional to
        // achieve that.
        //
        // Arguments passed to the parser constructor are
        // forwarded to the message object. A single argument
        // is forwarded to the body constructor.
        //
        // We construct the dynamic body with a 1MB limit
        // to prevent vulnerability to buffer attacks.
        //
        parser_.emplace(
            std::piecewise_construct,
            std::make_tuple(),
            std::make_tuple(alloc_));

        http::async_read(
            socket_,
            buffer_,
            *parser_,
            [this](beast::error_code ec, std::size_t)
            {
                if (ec)
                    accept();
                else
                    process_request(parser_->get());
            });
    }

    void process_request(http::request<request_body_t, http::basic_fields<alloc_t>> const& req)
    {
        switch (req.method())
        {
        case http::verb::get:
            on_get(req);
            break;
        case http::verb::post:
            on_post(req);
            break;

        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            send_bad_response(
                http::status::bad_request,
                "Invalid request-method '" + std::string(req.method_string()) + "'\r\n");
            break;
        }
    }

    void send_bad_response(
        http::status status,
        std::string const& error)
    {
        string_response_.emplace(
            std::piecewise_construct,
            std::make_tuple(),
            std::make_tuple(alloc_));

        string_response_->result(status);
        string_response_->keep_alive(false);
        string_response_->set(http::field::server, "Beast");
        string_response_->set(http::field::content_type, "text/plain");
        string_response_->body() = error;
        string_response_->prepare_payload();

        string_serializer_.emplace(*string_response_);

        http::async_write(
            socket_,
            *string_serializer_,
            [this](beast::error_code ec, std::size_t)
            {
                socket_.shutdown(tcp::socket::shutdown_send, ec);
                string_serializer_.reset();
                string_response_.reset();
                accept();
            });
    }

    void on_get(http::request<request_body_t, http::basic_fields<alloc_t>> const& req)
    {
	    beast::string_view path = req.target().substr(0, req.target().find_first_of('?'));
        std::string_view req_body = req.body(); // url param to json.
        std::string res_body = get_process_(path, req_body);
        
        string_response_.emplace(
            std::piecewise_construct,
            std::make_tuple(),
            std::make_tuple(alloc_));

        string_response_->result(http::status::ok);
        string_response_->keep_alive(false);
        string_response_->set(http::field::server, "Beast");
        string_response_->set(http::field::content_type, "application/json");
        string_response_->body() = res_body;
        string_response_->prepare_payload();

        string_serializer_.emplace(*string_response_);
        
        http::async_write(
            socket_,
            *string_serializer_,
            [this](beast::error_code ec, std::size_t)
            {
                socket_.shutdown(tcp::socket::shutdown_send, ec);
                string_serializer_.reset();
                string_response_.reset();
                accept();
            });
    }

    void on_post(http::request<request_body_t, http::basic_fields<alloc_t>> const& req)
    {
	    beast::string_view path = req.target().substr(0, req.target().find_first_of('?'));
        std::string_view req_body = req.body(); 
        std::string res_body = post_process_(path, req_body);

        string_response_.emplace(
            std::piecewise_construct,
            std::make_tuple(),
            std::make_tuple(alloc_));

        string_response_->result(http::status::ok);
        string_response_->keep_alive(false);
        string_response_->set(http::field::server, "Beast");
        string_response_->set(http::field::content_type, "application/json");
        string_response_->body() = res_body;
        string_response_->prepare_payload();

        string_serializer_.emplace(*string_response_);
        
        http::async_write(
            socket_,
            *string_serializer_,
            [this](beast::error_code ec, std::size_t)
            {
                socket_.shutdown(tcp::socket::shutdown_send, ec);
                string_serializer_.reset();
                string_response_.reset();
                accept();
            });
    }

    void send_file(beast::string_view target)
    {
        // Request path must be absolute and not contain "..".
        if (target.empty() || target[0] != '/' || target.find("..") != std::string::npos)
        {
            send_bad_response(
                http::status::not_found,
                "File not found\r\n");
            return;
        }

        std::string full_path = doc_root_;
        full_path.append(
            target.data(),
            target.size());

        http::file_body::value_type file;
        beast::error_code ec;
        file.open(
            full_path.c_str(),
            beast::file_mode::read,
            ec);
        if(ec)
        {
            send_bad_response(
                http::status::not_found,
                "File not found\r\n");
            return;
        }

        file_response_.emplace(
            std::piecewise_construct,
            std::make_tuple(),
            std::make_tuple(alloc_));

        file_response_->result(http::status::ok);
        file_response_->keep_alive(false);
        file_response_->set(http::field::server, "Beast");
        file_response_->set(http::field::content_type, mime_type(std::string(target)));
        file_response_->body() = std::move(file);
        file_response_->prepare_payload();

        file_serializer_.emplace(*file_response_);

        http::async_write(
            socket_,
            *file_serializer_,
            [this](beast::error_code ec, std::size_t)
            {
                socket_.shutdown(tcp::socket::shutdown_send, ec);
                file_serializer_.reset();
                file_response_.reset();
                accept();
            });
    }

    void check_deadline()
    {
        // The deadline may have moved, so check it has really passed.
        if (request_deadline_.expiry() <= std::chrono::steady_clock::now())
        {
            // Close socket to cancel any outstanding operation.
            socket_.close();

            // Sleep indefinitely until we're given a new deadline.
            request_deadline_.expires_at(
                (std::chrono::steady_clock::time_point::max)());
        }

        request_deadline_.async_wait(
            [this](beast::error_code)
            {
                check_deadline();
            });
    }
};

//------------------------------------------------------------------------------

class net_http
{
    public:
        using process_map_val_t = std::function<std::string(std::string_view)>;
        using process_map_t = std::map<beast::string_view, process_map_val_t>;

    public:
        net_http()
            : m_ioc{1}
            , m_acceptor(m_ioc)
        {
        }
        ~net_http()
        {
        }
        
    public:
		void startup(uint8_t workers, const std::string& address, uint16_t port, const std::string& doc_root)
        {
            tcp::endpoint endpoint(net::ip::make_address(address), port);
            boost::system::error_code ec;
            m_acceptor.open(endpoint.protocol(), ec);
            if (ec)
            {
                std::cout << "socket open error, " + ec.message() << std::endl;
                return;
            }
            m_acceptor.set_option(tcp::acceptor::reuse_address(true), ec);
            if (ec)
            {
                std::cout << "set_option reuse_address error, " + ec.message() << std::endl;
                return;
            }
            m_acceptor.bind(endpoint, ec);
            if (ec)
            {
                std::cout << "socket bind error, " + ec.message() << std::endl;
                return;
            }
            m_acceptor.listen(boost::asio::socket_base::max_connections, ec);
            if (ec)
            {
                std::cout << "socket listen error, " + ec.message() << std::endl;
                return;
            }

            for (int i = 0; i < workers; ++i)
            {
                m_workers.emplace_back(m_acceptor, doc_root);
                m_workers.back().set_get_process(std::bind(&net_http::dispatch_get, this, std::placeholders::_1, std::placeholders::_2));
                m_workers.back().set_post_process(std::bind(&net_http::dispatch_post, this, std::placeholders::_1, std::placeholders::_2));
                m_workers.back().start();
            }
            m_thread = std::make_shared<std::thread>([this]
            {
                m_ioc.run();
            });
        }

        void release()
        {
            m_thread->join();
        }

    public:
        void register_get(beast::string_view key, process_map_val_t val)
        {
            m_get_process_map.insert({key, val});
        }

        void register_post(beast::string_view key, process_map_val_t val)
        {
            m_post_process_map.insert({key, val});
        }

        std::string dispatch_get(beast::string_view key, std::string_view body)
        {
            auto target_iter = m_get_process_map.find(key);
            if (target_iter == m_get_process_map.end())
            {
                return "";	
            }
            return target_iter->second(body);
        }

        std::string dispatch_post(beast::string_view key, std::string_view body)
        {
            auto target_iter = m_post_process_map.find(key);
            if (target_iter == m_post_process_map.end())
            {
                return "";	
            }
            return target_iter->second(body);
        }

    private:
        net::io_context m_ioc;
        tcp::acceptor m_acceptor;
        std::list<http_worker> m_workers;
        std::shared_ptr<std::thread> m_thread;
        process_map_t m_get_process_map;
        process_map_t m_post_process_map;
};

#endif
