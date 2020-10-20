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

unsigned short ipv4_checksum(unsigned short *buf, int bytes) {
    unsigned int sum = 0;
    while (bytes) {
        sum += *buf++;
        bytes -= 2;
    }
    sum = (sum & 0xffff) + (sum >> 16);
    sum = (sum & 0xffff) + (sum >> 16);
    return ~sum;
}

int init_raw_ip_socket(int domain) {
    // IPの生のデータを扱うsocket
    int sockfd;
    if ((sockfd = socket(domain, SOCK_RAW, IPPROTO_RAW)) < 0) {
        perror("socket()");
        return sockfd;
    }
    // IP_HDRINCL: IPヘッダも自分で制御する
    int on = 1;
    if (setsockopt(sockfd,
                   domain == AF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP,
                   domain == AF_INET6 ? IPV6_HDRINCL : IP_HDRINCL,
                   &on, sizeof(on)) < 0) {
        perror("setsockopt()");
    }
    return sockfd;
}

// 希望する仮想デバイス名を入れる
int tun_alloc(char *dev) {
    // IFF_TUN: TUNを使う
    // IFF_NO_PI: 付加情報をつけないで生のパケットを読み書きする
    struct ifreq ifr = {
        .ifr_flags = IFF_TUN | IFF_NO_PI,
    };
    int fd, err;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        printf("open()");
        return fd;
    }

    if (dev && *dev) {
        strncpy(ifr.ifr_ifrn.ifrn_name, dev, IFNAMSIZ);
    }

    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
        close(fd);
        perror("ioctl()");
        return fd;
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

void print_sdaddr6(struct ip6_hdr *hdr) {
    char buf[100];
    char *ptr = buf;
    inet_ntop(AF_INET6, &hdr->ip6_src, ptr, 40);
    while (*ptr) ptr++;
    sprintf(ptr, " -> ");
    while (*ptr) ptr++;
    inet_ntop(AF_INET6, &hdr->ip6_dst, ptr, 40);
    puts(buf);
}