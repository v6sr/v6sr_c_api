/* Common logic for the SRH ping utilities
 * Author: Dave Sizer
 *
 * Copyright 2017
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <arpa/inet.h>

#include "ping_common.h"
#include "sr_api.h"


int parse_connection_options(int argc, 
        char **argv,
        inet6_addr **addr,
        uint16_t *port) {

    char *addr_str, *port_str;
    int c;
    int flagc = 0;
    

    while ( (c = getopt( argc, argv, "a:p:" )) != -1 ) {
        switch( c ) {
        case 'a':
            addr_str = optarg;
            flagc++;
            break;
        case 'p':
            port_str = optarg;
            flagc++;
            break;

        case '?':
            if (optopt == 'a' || optopt == 'p')
              fprintf (stderr, "Option -%c requires an argument.\n", optopt);

            return 1;
      default:
        abort ();

        }
    }

    if ( argc <= 1 || flagc < 2 ) {
        fprintf( stderr, "Both the address and port options must be provided\n" );
        return 1;
    }

    *port = strtol( port_str, NULL, 10 );
    if (errno != 0 ) {
        fprintf( stderr, "Error parsing port `%s`.\n", port_str );
        return errno;
    }


    return str_to_inet6( addr_str, addr );
}

int str_to_inet6( const char *str, inet6_addr **addr ) {
    
    // Assuming these are 16 bytes (128bits), can't find this struct documented
    // anywhere right now
    *addr = (inet6_addr*)malloc( 16 );
    int res = inet_pton( AF_INET6, str, *addr );
    if ( res != 1 ) {
        fprintf( stderr, "Error parsing IPv6 address `%s`.\n", str );
        return 1;
    }

    return 0;
}


void *build_srh_from_file(FILE *file) {
    int str_count = 0;
    str_ll *strs = NULL;
    while ( 1 ) {
        char *str = getline_safe( file, 256 );
        if ( NULL == str )
            break;
        if ( strlen(str) <= 1 )
            continue;

        // Strip the newline
        str[strlen(str)-1] = '\0';

        // Ignore lines starting with # for comment support
        if ( str[0] == '#' )
            continue;

        strs = str_ll_append( strs, str );
        str_count++;
    }

    printf("Done reading file.\n");

    // Allocate a new SR header
    socklen_t hdr_size = inet6_rth_space_n( IPV6_RTHDR_TYPE_4, str_count );
    void *hdr = malloc( hdr_size );

    // Initialize the header
    inet6_rth_init_n( hdr, hdr_size, IPV6_RTHDR_TYPE_4, str_count );

    struct ip6_rthdr4 *rthdr = (struct ip6_rthdr4*)hdr;

    str_ll *this_node = strs;
    int node_idx = 0;
    while ( NULL != this_node ) {

        inet6_addr *addr = NULL;
        if ( str_to_inet6( this_node->data, &addr )) {
            fprintf( stderr, "Failed to parse address: %s\n", this_node->data );
            return NULL;
        }

        rthdr->ip6r4_addr[node_idx++] = *(struct in6_addr*)addr;

        this_node = this_node->next;
    }

    rthdr->ip6r4_segleft = str_count - 1;
    rthdr->ip6r4_lastentry = str_count - 1;

    return hdr;
}


void *build_test_srh() {
    int total_segs = 1;
    struct ip6_rthdr4 *srh = (struct ip6_rthdr4*)malloc(sizeof(struct ip6_rthdr4) + 16 * total_segs);
    memset(srh, 0, sizeof(struct ip6_rthdr4) + 16 * total_segs);

    srh->ip6r4_type = IPV6_RTHDR_TYPE_4;

    srh->nexthdr = IPPROTO_TCP;
    srh->ip6r4_segleft = total_segs - 1;
    srh->ip6r4_lastentry = total_segs - 1;
    srh->ip6r4_len = ((8 + 16 * total_segs) >> 3) - 1;
    // NOTE not setting any flags


    /*
    inet_pton(AF_INET6, "2001:db8:0:2::1", &srh->ip6r4_addr[0]);
    inet_pton(AF_INET6, "2001:db8:0:5::1", &srh->ip6r4_addr[1]);
    inet_pton(AF_INET6, "2001:db8:0:7::1", &srh->ip6r4_addr[2]);
    inet_pton(AF_INET6, "2001:db8:0:8::1", &srh->ip6r4_addr[3]);
    */

    //inet_pton(AF_INET6, "2001:db8:0:1::1", &srh->ip6r4_addr[0]);
    inet_pton(AF_INET6, "2001:db8:0:1::2", &srh->ip6r4_addr[0]);
    //inet_pton(AF_INET6, "2001:db8:0:1::1", &srh->ip6r4_addr[0]);
    //inet_pton(AF_INET6, "2001:db8:0:1::1", &srh->ip6r4_addr[0]);



    return (void*)srh;
}

void hex_print(const uint8_t *buf, uint16_t len) {
    uint16_t off = 0;

    while ( off < len ) {
        printf( "%#x ", buf[off] );
        if ( (off + 1) % 4 == 0 )
            printf( "\n" );

        off++;
    }
    printf("\n");
}


void print_segment_addresses( void *srh ) {

    int num_segments = inet6_rth_segments_n( srh );

    for (int i = 0; i < num_segments; i++) {
        char addr_str[INET6_ADDRSTRLEN];
        printf("%s\n", inet_ntop(AF_INET6, 
                    inet6_rth_getaddr_n(srh, i), 
                    addr_str, INET6_ADDRSTRLEN));

    }

}

char *getline_safe( FILE *file, uint16_t size ) {
    char *str = (char*)malloc(size);
    memset( str, '\0', size );
    fgets(str, size, file);
    uint16_t line_length = 0;
    int valid_input = 0;
    while ( !valid_input && line_length < size) {
        if ( str[line_length] == '\n' )
            valid_input = 1;
        line_length++;
    }

    if ( valid_input ) {
        return str;
    } else {
        return NULL;
    }

}

str_ll *str_ll_append( str_ll *ll, char *str ) {
    str_ll *new = malloc(sizeof *new);

    new->data = str;
    new->next = NULL;

    if ( NULL == ll )
        return new;

    // Walk to the end
    str_ll *ptr = ll;
    while ( NULL != ptr->next )
        ptr = ptr->next;

    ptr->next = new;
    return ll;
}
