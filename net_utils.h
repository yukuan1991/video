#ifndef __NET_UTILS__
#define __NET_UTILS__
#include <windows.h>
#include <regex>
#include <boost/format.hpp>
#include <memory>
#include <stdio.h>
#include <QString>
#include <winsock.h>
#include <boost/lexical_cast.hpp>
#include <string>
#include <memory>
#include <functional>
#include <wininet.h>
#include <errno.h>
#include <string>
#include <map>
#include <stddef.h>
#include <ciso646>


constexpr unsigned int MAX_BLOCK_SIZE = 4096;
constexpr unsigned int MAX_LONGLONG_BUFFER = 32;
constexpr unsigned int DEFAULT_TCP_TIMEOUT = 15;

using progress_response = std::map<std::string, std::function<void (unsigned long long)>>;
using map_ss = std::map<std::string, std::string>;

//int http_download_with_progress (std::string url, std::string save_path, const progress_response& res = {});

std::string http_response (int server_socket);

std::string read_chunked (int server_socket);

int recv_peek (int socket_, void* buf, size_t len);

int32_t read_http_header (int socket_, void* buf, int max_len);

int tcp_open (const char *host, unsigned short port);

int tcp_write (int fd, const void* buf, int len);

bool write_string (int sock, const std::string& str_write);

int tcp_read (int sock, void* buffer, unsigned len);

std::string read_string (int sock, unsigned max_len);

int tcp_read_timeout (int fd, void* buf, int len, int second);

int read_until (int socket_, void* buf, int max_len, const char* until);

std::string http_get (std::string str_host, std::string str_path, const std::map<std::string, std::string>& params, unsigned short port = 80);
std::string http_post(std::string host, std::string path, std::string text, const map_ss& header = map_ss {}, unsigned short port = 80);




#endif //__NET_UTILS__
