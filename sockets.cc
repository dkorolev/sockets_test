#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <cstring>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

typedef int SOCKET;

const int FLAGS_port = 2015;
const int FLAGS_max_connections = 10;

const std::string FLAGS_host = "localhost";
const std::string FLAGS_serv = "2015";

int main() {
  const SOCKET server_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  std::cerr << "server_socket = " << server_socket << "\n";
  std::thread server_thread([](SOCKET server_socket) {
      int just_one = 1;
      if (::setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
#ifndef _WIN32 
          &just_one,
#else 
          reinterpret_cast<const char*>(&just_one),
#endif 
          sizeof(int))) {
      std::cerr << "::setsockopt(" << server_socket << "), errno = " << errno << "\n";
      exit(-1);
      }

      sockaddr_in addr_server;
      memset(&addr_server, 0, sizeof(addr_server));
      addr_server.sin_family = AF_INET;
      addr_server.sin_addr.s_addr = INADDR_ANY;
      addr_server.sin_port = htons(FLAGS_port);

      if (::bind(server_socket, (sockaddr*)&addr_server, sizeof(addr_server)) == -1) {
      std::cerr << "::bind(" << server_socket << "), errno = " << errno << "\n";
      exit(-1);
      }

      if (::listen(server_socket, FLAGS_max_connections)) {
        std::cerr << "::listen(" << server_socket << "), errno = " << errno << "\n";
        exit(-1);
      }

      sockaddr_in addr_client;
      memset(&addr_client, 0, sizeof(addr_client));
      auto addr_client_length = sizeof(sockaddr_in);
      const int accepted_socket = ::accept(server_socket,
          reinterpret_cast<struct sockaddr*>(&addr_client),
#ifndef _WIN32
          reinterpret_cast<socklen_t*>(&addr_client_length)
#else
          reinterpret_cast<int*>(&addr_client_length)
#endif
          );
      if (accepted_socket == -1) {
        std::cerr << "::accept(" << server_socket << "), errno = " << errno << "\n";
        exit(-1);
      }

      const char* msg = "This test has passed.";
      if (::send(accepted_socket, msg, strlen(msg) + 1, 0) < 0) {
        std::cerr << "::send(" << accepted_socket << "), errno = " << errno << "\n";
      }

      ::close(accepted_socket);
      ::close(server_socket);
  },
    server_socket);

  const SOCKET client_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  std::cerr << "client_socket = " << client_socket << "\n";

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  struct addrinfo* servinfo;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  const int retval = ::getaddrinfo(FLAGS_host.c_str(), FLAGS_serv.c_str(), &hints, &servinfo);
  if (retval || !servinfo) {
    std::cerr << "::getaddrinfo(), retval = " << retval << ", errno = " << errno << "\n";
    exit(-1);
  }
  struct sockaddr* p_addr = servinfo->ai_addr;
  // TODO(dkorolev): Use a random address, not the first one. Ref. iteration:
  // for (struct addrinfo* p = servinfo; p != NULL; p = p->ai_next) {
  //   p->ai_addr;
  // }
  const int retval2 = ::connect(client_socket, p_addr, sizeof(*p_addr));
  if (retval2) {
    std::cerr << "::connect(), retval = " << retval2 << ", errno = " << errno << "\n";
    exit(-1);
  }
  ::freeaddrinfo(servinfo);

  const size_t max_length_in_bytes = 100;
  uint8_t recv_buffer[max_length_in_bytes];
  uint8_t* raw_buffer = reinterpret_cast<uint8_t*>(recv_buffer);
  uint8_t* raw_ptr = raw_buffer;
  do {
#ifndef _WIN32
    const ssize_t retval = ::recv(client_socket, raw_ptr, max_length_in_bytes - (raw_ptr - raw_buffer), 0);
#else
    const int retval =
      ::recv(socket, reinterpret_cast<char*>(raw_ptr), max_length_in_bytes - (raw_ptr - raw_buffer),
          0);
#endif
    if (retval < 0) {
      if (errno == EAGAIN) {
        continue;
      } else {
        std::cerr << "::recv(), retval = " << retval << ", errno = " << errno << "\n";
        exit(-1);
      }
    } else {
      if (retval == 0) {
        break;
      } else {
        raw_ptr += retval;
      }
    }
  } while (true);

  ::close(client_socket);

  const std::string result(raw_buffer, raw_ptr);
  std::cerr << "Result: '" << result << "', " << result.length() << " bytes.\n";

  server_thread.join();
}
