#include <gmock/gmock.h>

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <time.h>
//#include <event2/event-config.h>

//#ifndef WIN32
//#   include <sys/queue.h>
//#   include <unistd.h>
//#   include <sys/queue.h>
//#endif

//#ifdef _EVENT_HAVE_SYS_TIME_H
//#   include <sys/time.h>
//#endif

//#include <signal.h>
//#include <fcntl.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>
//#include <errno.h>

#include <event2/event.h>
//#include <event2/event_struct.h>
//#include <event2/util.h>

#ifdef WIN32
#   include <winsock2.h>
#   include <windows.h>
#endif

static void
timeout_cb(evutil_socket_t /*fd*/, short event, void *arg)
{
}

TEST(test_libevent, timeout)
{
#ifdef WIN32
    WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(2, 2);

    (void)WSAStartup(wVersionRequested, &wsaData);
#endif

    /* Initalize the event library */
    event_base *base = event_base_new();
    ASSERT_TRUE(base);

    event *timeout = event_new(base, -1, EV_TIMEOUT, &timeout_cb, 0);
    ASSERT_TRUE(timeout);

    timeval tv { 2, 0 };
    EXPECT_FALSE(event_pending(timeout, EV_TIMEOUT, &tv));
    EXPECT_EQ(0, event_add(timeout, &tv));
    EXPECT_TRUE(event_pending(timeout, EV_TIMEOUT, &tv));
    EXPECT_EQ(0, event_base_loop(base, EVLOOP_NONBLOCK | EVLOOP_ONCE));
    event_free(timeout);
    return;
}

/*#include <event.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>

// Read/write buffer max length
static const size_t MAX_BUF = 512;

typedef struct {
    struct event ev;
    char         buf[MAX_BUF];
    size_t       offset;
    size_t       size;
} connection_data;

void on_connect(int fd, short event, void *arg);
void client_read(int fd, short event, void *arg);
void client_write(int fd, short event, void *arg);

TEST(test_libevent, echo_server)
{
    // Create server socket
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    sockaddr_in sa;
    int         on      = 1;
    char      * ip_addr = "127.0.0.1";
    short       port    = 4563;

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = inet_addr(ip_addr);

    // Set option SO_REUSEADDR to reuse same host:port in a short time
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
        std::cerr << "Failed to set option SO_REUSEADDR" << std::endl;
        return;
    }

    // Bind server socket to ip:port
    if (bind(server_sock, reinterpret_cast<const sockaddr*>(&sa), sizeof(sa)) == -1) {
        std::cerr << "Failed to bind server socket" << std::endl;
        return;
    }

    // Make server to listen
    if (listen(server_sock, 10) == -1) {
        std::cerr << "Failed to make server listen" << std::endl;
        return;
    }

    // Init events
    struct event evserver_sock;
    // Initialize
    event_init();
    // Set connection callback (on_connect()) to read event on server socket
    event_set(&evserver_sock, server_sock, EV_READ, on_connect, &evserver_sock);
    // Add server event without timeout
    event_add(&evserver_sock, NULL);

    // Dispatch events
    event_dispatch();

    return;
}

// Handle new connection {{{
void on_connect(int fd, short event, void *arg)
{
    sockaddr_in client_addr;
    socklen_t   len = 0;

    // Accept incoming connection
    int sock = accept(fd, reinterpret_cast<sockaddr*>(&client_addr), &len);
    if (sock < 1) {
        return;
    }

    // Set read callback to client socket
    connection_data * data = new connection_data;
    event_set(&data->ev, sock, EV_READ, client_read, data);
    // Reschedule server event
    //event_add(reinterpret_cast<struct event*>(arg), NULL);
    // Schedule client event
    event_add(&data->ev, NULL);
}
//}}}

// Handle client request {{{
void client_read(int fd, short event, void *arg)
{
    connection_data * data = reinterpret_cast<connection_data*>(arg);
    if (!data) {
        close(fd);
        return;
    }
    int len = read(fd, data->buf, MAX_BUF - 1);
    if (len < 1) {
        close(fd);
        delete data;
        return;
    }
    data->buf[len] = 0;
    data->size     = len;
    data->offset   = 0;
    // Set write callback to client socket
    event_set(&data->ev, fd, EV_WRITE, client_write, data);
    // Schedule client event
    event_add(&data->ev, NULL);
}
//}}}

// Handle client responce {{{
void client_write(int fd, short event, void *arg)
{
    connection_data * data = reinterpret_cast<connection_data*>(arg);
    if (!data) {
        close(fd);
        return;
    }
    // Send data to client
    int len = write(fd, data->buf + data->offset, data->size - data->offset);
    if (len < data->size - data->offset) {
        // Failed to send rest data, need to reschedule
        data->offset += len;
        event_set(&data->ev, fd, EV_WRITE, client_write, data);
        // Schedule client event
        event_add(&data->ev, NULL);
    }
    close(fd);
    delete data;
}
//}}}
*/
