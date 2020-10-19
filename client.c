#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/if_tun.h>
#include <net/ethernet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

int main() {
    // TUN仮想デバイスディスクリプタ(tunclientを希望)
    char tunname[IFNAMSIZ] = "tunclient";
    int tunfd = tun_alloc(tunname);
    printf("tun = '%s (%d)'\n", tunname, tunfd);

    // 外部コマンドで仮想デバイスにIPアドレスを設定＆リンクアップ
    char command[256];
    sprintf(command, "ip addr add 11.8.0.5 dev %s", tunname);
    system(command);
    sprintf(command, "ip link set %s up", tunname);
    system(command);

    // サーバのIPアドレスを環境変数から取得
    char *_server_ipaddr;
    if (!(_server_ipaddr = getenv("SERVER_IPADDR"))) {
        puts("SERVER_IPADDR not found");
        exit(1);
    }
    char server_ipaddr[256];
    strncpy(server_ipaddr, _server_ipaddr, sizeof(server_ipaddr));
    const int port = 1195;

    // サーバとのUDPsocket
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

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr(server_ipaddr),
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
    {  // アプリケーションからの受信イベントを登録
        struct epoll_event ev = {.events = EPOLLIN, .data.fd = tunfd};
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, tunfd, &ev) < 0) perror("epoll_ctl()");
    }
    {  // サーバからの受信イベントを登録
        struct epoll_event ev = {.events = EPOLLIN, .data.fd = sockfd};
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) perror("epoll_ctl()");
    }

    const int bufsize = UINT16_MAX;
    char data[bufsize];
    while (1) {
        int events_size;
        // イベント発生待ち（ブロック）
        if ((events_size = epoll_wait(epollfd, events_list, max_events, -1)) < 0) {
            perror("epoll_wait()");
        }

        for (size_t i = 0; i < events_size; i++) {
            struct epoll_event *e = &events_list[i];

            if (e->data.fd == sockfd && e->events & EPOLLIN) {
                // サーバからデータ受信
                int datalen;
                if ((datalen = read(sockfd, data, sizeof(data))) < 0) {
                    perror("read sockfd");
                } else {
                    // アプリケーションに送信
                    if (write(tunfd, data, datalen) < 0)
                        perror("write tunfd");
                }
            } else if (e->data.fd == tunfd && e->events & EPOLLIN) {
                // アプリケーションから受信
                int datalen;
                if ((datalen = read(tunfd, data, sizeof(data))) < 0) {
                    perror("read tunfd");
                } else {
                    // サーバに送信
                    if (sendto(sockfd, data, datalen, 0,
                               (struct sockaddr *)&server_addr,
                               sizeof(server_addr)) < 0)
                        perror("write sockfd");
                }
            }
        }
    }
}