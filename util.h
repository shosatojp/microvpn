#pragma once
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

void printhex(char *src, size_t size);
unsigned short ipv4_checksum(unsigned short *buf, int bytes);
int init_raw_ipv4_socket();
int tun_alloc(char *dev);
void print_sdaddr(struct iphdr *hdr);