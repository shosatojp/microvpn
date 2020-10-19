#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

int main() {
    // TUN仮想デバイスディスクリプタ(tunserverを希望)
    char tunname[IFNAMSIZ] = "tunserver";
    int tunfd = tun_alloc(tunname);
    printf("tun = '%s (%d)'\n", tunname, tunfd);

    // 外部コマンドで仮想デバイスにIPアドレスを設定&リンクアップ&ルーティング
    char command[256];
    sprintf(command, "ip addr add 11.8.0.1 dev %s", tunname);
    system(command);
    sprintf(command, "ip link set %s up", tunname);
    system(command);
    sprintf(command, "ip route add 11.8.0.0/24 dev %s", tunname);
    system(command);

    // インターネットへの送信用RAW IP socket
    int eth = init_raw_ipv4_socket();

    // クライアントのIPアドレスを環境変数から取得
    char *_client_ipaddr;
    if (!(_client_ipaddr = getenv("CLIENT_IPADDR"))) {
        puts("CLIENT_IPADDR not found");
        exit(1);
    }
    char client_ipaddr[256];
    strncpy(client_ipaddr, _client_ipaddr, sizeof(client_ipaddr));
    const int port = 1195;

    // クライアントとのUDPsocket
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket()");
        exit(1);
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY,
    };

    struct sockaddr_in client_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr(client_ipaddr),
    };

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind()");
        exit(1);
    }

    int epollfd;
    const int max_events = 1000;
    if ((epollfd = epoll_create(max_events)) < 0) {
        perror("epoll()");
        exit(1);
    }

    struct epoll_event events_list[max_events];
    {  // クライアントからの受信イベントを登録
        struct epoll_event e = {.events = EPOLLIN, .data.fd = sockfd};
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &e) < 0) perror("epoll_ctl()");
    }
    {  // インターネットからの受信用
        struct epoll_event e = {.events = EPOLLIN, .data.fd = tunfd};
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, tunfd, &e) < 0) perror("epoll_ctl()");
    }

    const int bufsize = UINT16_MAX;
    char data[bufsize];
    while (1) {
        int events_size;
        if ((events_size = epoll_wait(epollfd, events_list, max_events, -1)) < 0) {
            perror("epoll_wait()");
        }

        for (size_t i = 0; i < events_size; i++) {
            struct epoll_event *e = &events_list[i];

            if (e->data.fd == sockfd && e->events & EPOLLIN) {
                // クライアントから受信
                int datalen;
                if ((datalen = read(sockfd, data, sizeof(data))) < 0) {
                    perror("recv from client");
                } else {
                    // インターネットに送信
                    struct sockaddr_in to = {
                        .sin_family = AF_INET,
                        .sin_addr.s_addr = ((struct iphdr *)data)->daddr,
                    };
                    if (sendto(eth, data, datalen, 0,
                               (struct sockaddr *)&to, sizeof(to)) < 0) {
                        perror("sendto internet");
                    }
                }
            } else if (e->data.fd == tunfd && e->events & EPOLLIN) {
                // インターネットからパケット受信
                int datalen;
                if ((datalen = read(tunfd, data, sizeof(data))) < 0) {
                    perror("read tunfd");
                } else {
                    // クライアントに送信
                    if (sendto(sockfd, data, datalen, 0,
                               (struct sockaddr *)&client_addr,
                               sizeof(client_addr)) < 0) {
                        perror("sendto client");
                    }
                }
            }
        }
    }
}