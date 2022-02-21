#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
static int stream_socketpair(struct addrinfo* addr_info, SOCKET sock[2]) {
    SOCKET listener, client, server;
    int opt = 1;
    struct sockaddr_in* addr;

    listener = server = client = INVALID_SOCKET;
    listener = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol); //����������socket�����а󶨼�����
    if (INVALID_SOCKET == listener)
        goto fail;

    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    addr = (struct sockaddr_in*)addr_info->ai_addr;
    if (SOCKET_ERROR == bind(listener, addr_info->ai_addr, addr_info->ai_addrlen))
        goto fail;

    if (SOCKET_ERROR == getsockname(listener, addr_info->ai_addr, (int*)&addr_info->ai_addrlen))
        goto fail;

    if (SOCKET_ERROR == listen(listener, 5))
        goto fail;

    client = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol); 

    if (INVALID_SOCKET == client)
        goto fail;

    if (SOCKET_ERROR == connect(client, addr_info->ai_addr, addr_info->ai_addrlen))
        goto fail;

    server = accept(listener, 0, 0);

    if (INVALID_SOCKET == server)
        goto fail;

    closesocket(listener);

    sock[0] = client;
    sock[1] = server;

    return 0;
fail:
    if (INVALID_SOCKET != listener)
        closesocket(listener);
    if (INVALID_SOCKET != client)
        closesocket(client);
    return -1;
}

static int socketpair(int family, int type, int protocol, SOCKET recv[2]) {
    const char* address;
    struct addrinfo addr_info, * p_addrinfo;
    int result = -1;

    memset(&addr_info, 0, sizeof(addr_info));
    addr_info.ai_family = family;
    addr_info.ai_socktype = type;
    addr_info.ai_protocol = protocol;
    if (AF_INET6 == family)
        address = "0:0:0:0:0:0:0:1";
    else
        address = "127.0.0.1";

    if (0 == getaddrinfo(address, "0", &addr_info, &p_addrinfo)) {
        if (SOCK_STREAM == type)
            result = stream_socketpair(p_addrinfo, recv);   //use for tcp
        freeaddrinfo(p_addrinfo);
    }
    return result;
}
DWORD WINAPI send_data(LPVOID param)
{
    SOCKET send_fd = *(SOCKET*)param;
    char buf[10];
    while (1) {
        printf("input send data:\n");
        scanf_s("%s", buf);
        int ret = send(send_fd, buf, 3, 0);
        if (ret <= 0) {
            printf("failed send data:\n");
        }
    }
    return 0;
}

int main(void* arg, void* argv[])
{
    WSADATA wsaData;
    WORD wsaVersion = MAKEWORD(2, 2);
    int ret = WSAStartup(wsaVersion, &wsaData);
    if (ret != 0) {
        printf("wsa init failed!\n");
        return -1;
    }
    SOCKET channels[2];
    if (socketpair(AF_INET, SOCK_STREAM, 0, (SOCKET*)channels) == -1) {
        printf("create socketpair failed\n");
        return -1;
    }
    DWORD thread_id;
    HANDLE thread_handle = CreateThread(NULL, 0, send_data, &channels[0], 0, &thread_id);
    char buf[10];
    int len = 10;
    while (1) {
        memset(buf, 0, sizeof(buf));
        ret = recv(channels[1], buf, len - 1 , 0);
        printf("recv data: %s\n", buf);
    }
    closesocket(channels[0]);
    closesocket(channels[1]);
    WSACleanup();
    CloseHandle(thread_handle);
    return 0;
}