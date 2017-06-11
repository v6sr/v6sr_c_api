/**
 * Modified from glibc.
 *
 * Author: Dave Sizer
 */
#ifndef __SR_API_H__
#define __SR_API_H__
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define IPV6_RTHDR_TYPE_4 4

struct ip6_rthdr4 {
    uint8_t nexthdr;
    uint8_t ip6r4_len;
    uint8_t ip6r4_type;
    uint8_t ip6r4_segleft;
    uint8_t ip6r4_lastentry;
    uint8_t ip6r4_flags;
    uint16_t ip6r4_tag;

    struct in6_addr ip6r4_addr[0];

} __attribute__((packed)) ;

/* These declarations are taken directly from glibc */
extern socklen_t inet6_rth_space_n (int __type, int __segments);
extern void *inet6_rth_init_n (void *__bp, socklen_t __bp_len, int __type,int __segments);
extern int inet6_rth_add_n (void *__bp, const struct in6_addr *__addr);
extern int inet6_rth_reverse_n (const void *__in, void *__out);
extern int inet6_rth_segments_n (const void *__bp);
extern struct in6_addr *inet6_rth_getaddr_n (const void *__bp, int __index);
#endif
