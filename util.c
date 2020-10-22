#include "util.h"

void printhex(char *src, size_t size) {
    static const unsigned int width = 0b1111;
    static const char *hex_figures = "0123456789ABCDEF";
    for (size_t i = 0; i < size; i++) {
        putc(hex_figures[(src[i] & 0xf0) >> 4], stdout);
        putc(hex_figures[src[i] & 0x0f], stdout);
        putc(' ', stdout);

        if (i && (i & width) == width)
            putc('\n', stdout);
    }
    putc('\n', stdout);
}

int init_raw_ipv4_socket() {
    // IPv4の生のデータを扱うsocket
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
        perror("socket()");
        exit(1);
    }
    // IP_HDRINCL: IPヘッダも自分で制御する
    int on = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
        perror("setsockopt()");
    }
    return sockfd;
}

// 希望する仮想デバイス名を入れる
int tun_alloc(char *dev) {
    struct ifreq ifr = {};
    int fd, err;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        printf("open()");
        return fd;
    }

    // IFF_TUN: TUNを使う
    // IFF_NO_PI: 付加情報をつけないで生のパケットを読み書きする
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    if (*dev) {
        strncpy(ifr.ifr_ifrn.ifrn_name, dev, IFNAMSIZ);
    }

    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
        close(fd);
        perror("ioctl()");
        return err;
    }
    strncpy(dev, ifr.ifr_ifrn.ifrn_name, IFNAMSIZ);
    return fd;
}

void print_sdaddr(struct iphdr *hdr) {
    printf("%u.%u.%u.%u -> %u.%u.%u.%u\n",
           (hdr->saddr & 0x000000ff),
           (hdr->saddr & 0x0000ff00) >> 8,
           (hdr->saddr & 0x00ff0000) >> 16,
           (hdr->saddr & 0xff000000) >> 24,
           (hdr->daddr & 0x000000ff),
           (hdr->daddr & 0x0000ff00) >> 8,
           (hdr->daddr & 0x00ff0000) >> 16,
           (hdr->daddr & 0xff000000) >> 24);
}