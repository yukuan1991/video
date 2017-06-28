#include "net_utils.h"
#include <regex>
#include <boost/format.hpp>
#include <memory>
#include <stdio.h>
#include <QString>
#include <boost/lexical_cast.hpp>
#include <list>




using void_smart_ptr = std::unique_ptr<void, std::function<void(void*)>>;

//int http_download_with_progress (std::string str_url, std::string save_path, const progress_response& res)
//{
//    return 0;
//    std::unique_ptr<FILE, std::function<void (FILE*)>> fp {fopen (save_path.data (), "wb"), fclose};
//    unsigned long long ul_current_byte = 0;
//    unsigned long long ul_total_byte = 0;
//
//    char sz_buffer_size [MAX_LONGLONG_BUFFER];
//    unsigned long int buffer_length = sizeof (sz_buffer_size);
//    std::string net_agent = "RookID/1.0";
//
//    auto tmp_block = std::make_unique<unsigned char[]> (MAX_BLOCK_SIZE);
//
//    void_smart_ptr handle_session {InternetOpenA (net_agent.data (), INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0), ::InternetCloseHandle};
//
//    if (handle_session == nullptr)
//    {
//        errno = __LINE__;
//        return -1;
//    }
//
//    void_smart_ptr handle_url {::InternetOpenUrlA (handle_session.get(), str_url.data(), nullptr, 0, INTERNET_FLAG_DONT_CACHE, 0), ::InternetCloseHandle};
//
//    if (handle_url == nullptr)
//    {
//        errno = __LINE__;
//        return -1;
//    }
//
//    bool query_success = ::HttpQueryInfoA(handle_url.get (), HTTP_QUERY_CONTENT_LENGTH, sz_buffer_size, &buffer_length, nullptr);
//
//    if (!query_success)
//    {
//        errno = __LINE__;
//        return -1;
//    }
//
//    try
//    {
//        ul_total_byte = boost::lexical_cast<decltype (ul_total_byte)> (sz_buffer_size);
//    }
//    catch (...)
//    {
//        errno = __LINE__;
//        return -1;
//    }
//
//    auto iter = res.find ("init");
//    if (iter != end (res) and iter->second != nullptr)
//    {
//        iter->second (ul_total_byte);
//    }
//
//    fseek (fp.get (), 0, SEEK_SET);
//
//    if (fp == nullptr)
//    {
//        errno = __LINE__;
//        return -1;
//    }
//
//    long unsigned int read_len;
//    ul_current_byte = 0;
//
//    do
//    {
//        ::InternetReadFile (handle_url.get (), tmp_block.get (), MAX_BLOCK_SIZE, &read_len);
//
//        ul_current_byte += read_len;
//        if (read_len > 0)
//        {
//            ::fwrite (tmp_block.get (), sizeof (unsigned char), read_len, fp.get());
//        }
//
//        auto iter = res.find ("update");
//        if (iter != end (res) and iter->second != nullptr)
//        {
//            iter->second (ul_current_byte);
//        }
//
//    }while (read_len > 0);
//
//    if (ul_current_byte != ul_total_byte)
//    {
//        errno = __LINE__;
//        return -1;
//    }
//
//    return 0;
//}

std::string http_get (std::string str_host, std::string str_path, const std::map<std::string, std::string>& params, unsigned short port)
{
    int server_socket;

    std::string str_get = std::string ("GET ") + str_path + "?";
    for (auto iter = params.begin(); iter != params.end(); ++iter)
    {
        str_get += (iter->first + "=" + iter->second);

        auto tmp = iter;
        tmp++;
        if (tmp != params.end())
        {
            str_get += "&";
        }
    }

    boost::format send_header_format (
    "%1% HTTP/1.1\r\n"
    "Host: %2%\r\n"
    "\r\n");
    send_header_format % str_get % str_host;

    if ((server_socket = tcp_open (str_host.c_str(), port)) == -1)
    {
        return std::string("");
    }

    if (tcp_write(server_socket, send_header_format.str().c_str(), send_header_format.str().length()) != (int)send_header_format.str().length())
    {
        ::closesocket (server_socket);
        return std::string ("");
    }

    auto ret =  http_response(server_socket);

    ::closesocket (server_socket);

    return ret;
}



std::string http_response (int server_socket)
{
    unsigned long body_len;
    std::string str_http_body;
    auto up_rcv_header = std::make_unique<char[]> (8192 + 1);


    int32_t http_header_len = read_http_header (server_socket, up_rcv_header.get(), 8192);

    if (http_header_len == -1)
    {
        return "";
    }
    up_rcv_header[http_header_len] = '\0';

    do
    {
        std::regex expression("\r\nContent-Length:[[:space:]]+([[:digit:]]+)");
        std::smatch hit;
        std::string str_header = up_rcv_header.get();
        if (std::regex_search (str_header, hit, expression))
        {
            try
            {
                body_len = boost::lexical_cast<unsigned long> (hit[1].str());
            }
            catch (...)
            {
                errno = EIO;
                return "";
            }
        }
        else
        {
            break;
        }

        str_http_body.resize (body_len);

        unsigned long body_rcv = tcp_read_timeout(server_socket, const_cast<char*>(str_http_body.data()), body_len, DEFAULT_TCP_TIMEOUT);

        if (body_rcv != body_len)
        {
            errno = EIO;
            return std::string ("");
        }

        return str_http_body;
    }
    while (0);

    std::regex expression("\r\nTransfer-Encoding:[[:space:]]+chunked");
    std::smatch hit;
    std::string str_header = up_rcv_header.get();
    if (!std::regex_search (str_header, hit, expression))
    {
        errno = EIO;
        return std::string ("");
    }

    str_http_body = read_chunked (server_socket);

    return str_http_body;
}



#define MAX_CHUNK_LEN (1024*1024)
#define MAX_TOTAL_LEN (1024*1024*10)

std::string read_chunked (int server_socket)
{
    int read_len;

    std::unique_ptr<char[]> up_chunk_len = std::make_unique<char[]> (128 + 1);
    int n_chunk_len;

    std::string str_chunk_content;
    std::string str_output;

    std::list<std::string> list_chunk;
    int n_total_len = 0;

    while (1)
    {
        read_len = read_until(server_socket, up_chunk_len.get(), 128, "\r\n");

        if (read_len == -1)
        {
            return "";
        }

        up_chunk_len[read_len - 2] = '\0';

        sscanf(up_chunk_len.get(), "%x", &n_chunk_len);

        if (n_chunk_len > MAX_CHUNK_LEN)
        {
            return "";
        }
        else if (n_chunk_len == 0)
        {
            break;
        }


        str_chunk_content.resize (n_chunk_len + 2);

        read_len = tcp_read_timeout(server_socket, const_cast<char*>(str_chunk_content.data()), n_chunk_len + 2, DEFAULT_TCP_TIMEOUT);

        if (read_len != n_chunk_len + 2)
        {
            return "";
        }

        str_chunk_content.resize (n_chunk_len);

        list_chunk.emplace_back (move (str_chunk_content));

        if ((n_total_len += n_chunk_len) >  MAX_TOTAL_LEN)
        {
            return "";
        }
    }

    str_output.resize (n_total_len);
    str_output = "";

    for (auto it : list_chunk)
    {
        str_output += it;
    }

    return str_output;
}

int recv_peek (int socket_, void* buf, size_t len)
{
    int recv_len = 0;

    while ((recv_len = recv (socket_, (char*)buf, len, MSG_PEEK)) == -1 and errno == EINTR);

    return recv_len;
}



int32_t read_http_header (int socket_, void* buf, int max_len)
{
    int32_t readval = 0, offset = 0; char* tmp_ptr = (char*)buf;

    const char* header_bondary = "\r\n\r\n";


    while (offset < max_len)
    {

        /*----------------------------------------peek buffer----------------------------------------------*/
        readval = recv_peek (socket_, tmp_ptr, max_len - offset);

        if (readval <= 0)
        {
            return readval;
        }
        /*----------------------------------------------END------------------------------------------------*/


        /*------------------------------------------examine buffer-----------------------------------------*/
        for (int32_t i=0; i<= readval - 4; i++)
        {
            if (memcmp (tmp_ptr + i, header_bondary, 4) == 0)
            {
                readval = tcp_read_timeout (socket_, tmp_ptr, i + 4, DEFAULT_TCP_TIMEOUT);

                return readval == i + 4 ? readval + offset : -1;
            }
        }
        /*----------------------------------------------END------------------------------------------------*/


        /*--------------------------------end flag not found, updating offset------------------------------*/
        if (readval + offset >= max_len)
        {
            errno = EIO;
            break;
        }

        offset += readval;

        if ((readval = tcp_read_timeout (socket_, tmp_ptr, offset, DEFAULT_TCP_TIMEOUT)) != offset)
        {
            return -1;
        }

        tmp_ptr += offset;
        /*----------------------------------------------END------------------------------------------------*/

    }

    return -1;
}




int read_until (int socket_, void* buf, int max_len, const char* until)
{
    int32_t readval = 0, offset = 0; char* tmp_ptr = (char*)buf;

    int until_len = strlen (until);


    while (offset < max_len)
    {

        /*----------------------------------------peek buffer----------------------------------------------*/
        readval = recv_peek (socket_, (char*)buf + offset, max_len - offset);

        if (readval <= 0)
        {
            return readval;
        }
        /*----------------------------------------------END------------------------------------------------*/


        /*------------------------------------------examine buffer-----------------------------------------*/
        for (int32_t i=0; i<= readval - until_len; i++)
        {
            if (memcmp (tmp_ptr + i, until, until_len) == 0)
            {
                readval = tcp_read_timeout (socket_, (char*)buf + offset, i + until_len, DEFAULT_TCP_TIMEOUT);

                return readval == i + until_len ? readval + offset : -1;
            }
        }
        /*----------------------------------------------END------------------------------------------------*/


        /*--------------------------------end flag not found, updating offset------------------------------*/
        if (readval + offset >= max_len)
        {
            errno = EIO;
            break;
        }


        if (tcp_read_timeout (socket_, (char*)buf + offset, readval, DEFAULT_TCP_TIMEOUT) != readval)
        {
            return -1;
        }
        offset += readval;
        /*----------------------------------------------END------------------------------------------------*/

    }

    return -1;
}

int tcp_open (const char *host, unsigned short port)
{
    if (host == nullptr or port == 0)
    {
        errno = EINVAL;
        return -1;
    }

    WSADATA wsadata;
    if(WSAStartup(MAKEWORD(1,1),&wsadata)==SOCKET_ERROR)
    {
        return -1;
    }


    int server_fd;
    struct sockaddr_in server_addr;


    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr (host);
    server_addr.sin_port = htons (port);


    if ((server_fd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
        return -1;
    }

    if (connect (server_fd, (struct sockaddr*)&server_addr, sizeof server_addr) != 0)
    {
        return -1;
    }

    return server_fd;
}

int tcp_write (int fd, const void* buf, int len)
{
    if (fd < 0 || buf == nullptr || len <= 0)
    {
        errno = EINVAL;
        return -1;
    }
    int32_t offset, w_len;

    offset = w_len = 0;
    while (len > offset)
    {
        if ((w_len = send (fd, (char*)buf + offset, len, 0)) <= 0)
        {
            if (w_len == -1 and errno != EINTR) break;
            continue;
        }

        offset += w_len;
    }
    return offset;
}

int tcp_read_timeout (int fd, void* buf, int len, int second)
{
    /*--------------------------------------variable checking------------------------------------------*/
    if (second < 0 or len <= 0 || buf == nullptr)
    {
        errno = EINVAL;
        return -1;
    }
    /*----------------------------------------------END------------------------------------------------*/


    /*-------------------------------------------init values-------------------------------------------*/
    int32_t offset, read_len, select_return;
    fd_set read_fdset;
    struct timeval timeout;

    offset = read_len = 0;

    /*----------------------------------------------END------------------------------------------------*/


    while (len > offset)
    {
        /*----------------------------------------timeout examine----------------------------------------*/
        if (second != 0)
        {
            FD_ZERO(&read_fdset);
            FD_SET(fd, &read_fdset);
            timeout.tv_sec = second; timeout.tv_usec = 0;

            do
            {
                select_return = select(fd + 1, &read_fdset, nullptr, nullptr, &timeout);
            } while (select_return == -1 and errno == EINTR);

            if (select_return == 0)
            {
                errno = ETIMEDOUT;
                break;
            }
        }
        /*----------------------------------------------END------------------------------------------------*/

        /*------------------------------------------reading data-----------------------------------------*/
        if ((read_len = recv(fd, (char*)buf + offset, len - offset, 0)) <= 0)
        {
            if (read_len == -1 and errno == EINTR) continue;
            break;
        }
        /*----------------------------------------------END------------------------------------------------*/

        offset += read_len;
    }

    return offset;
}


bool write_string (int sock, const std::string& str_write)
{
    uint32_t len;

    len = htonl (str_write.length ());

    if (tcp_write (sock, &len, sizeof len) != sizeof len)
    {
        errno = EIO;
        return false;
    }

    if (tcp_write (sock, str_write.data (), str_write.size ()) != static_cast<int>(str_write.size ()))
    {
        errno = EIO;
        return false;
    }

    return true;
}

int tcp_read (int sock, void* buffer, unsigned len)
{
    if (sock < 0 or buffer == nullptr)
    {
        errno = EINVAL;
        return -1;
    }

    unsigned offset = 0;
    int read_return;


    while (offset < len)
    {
        read_return = ::recv (sock, static_cast<char*>(buffer) + offset, len, 0);

        if (read_return + offset == len)
        {
            return read_return;
        }

        if (read_return == 0)
        {
            errno = EIO;
            return offset;
        }

        if (read_return < 0)
        {
            if (errno != EINTR) return offset;
            continue;
        }


        offset += read_return;
    }
    return -1;
}

std::string read_string (int sock, unsigned max_len)
{
    uint32_t len;

    if (tcp_read (sock, &len, sizeof len) != sizeof len)
    {
        return {};
    }

    if ((len = htonl (len)) > max_len)
    {
        errno = E2BIG;
        return {};
    }

    std::string ret;
    ret.resize (len);

    if (tcp_read (sock, const_cast<char*> (ret.data ()), len) != static_cast<int> (len))
    {
        return {};
    }

    return ret;

}

std::string http_post(std::string host, std::string path, std::string text, const map_ss& headers, unsigned short port)
{
    //变量声明
    int server_socket;

    //格式化post消息
    //------header----------------
    std::stringstream ss;
    ss << "POST " << path << " HTTP/1.1\r\n";
    ss << "Host: " << host << "\r\n";
    ss << "Content-Length:" << std::to_string (text.length ()) << "\r\n";

    for (auto& iter : headers)
    {
        ss << iter.first << ": " << iter.second << "\r\n";
    }
    ss << "\r\n";
    //------body--------------------
    ss << text;
    //------------------------------

    std::string write_buffer {ss.str ()};

    //建立连接
    if ((server_socket = tcp_open (host.c_str(), port)) == -1)
    {
        return "";
    }

    if (tcp_write (server_socket, write_buffer.data (), write_buffer.length ()) != (signed)write_buffer.length ())
    {
        ::closesocket (server_socket);
        return "";
    }

    //获取接受的数据并返回
    return http_response(server_socket);
}
