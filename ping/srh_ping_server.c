/* Simple server that listens on a segment routing enabled TCP socket using a
 * given IPv6 address and port.
 *
 * Args: -i : The IPv6 address of the interface to listen on -p : The port to
 * listen on
 *
 * Author: Dave Sizer
 *
 * Copyright 2017
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "ping_common.h"
#include "sr_api.h"


void start_server(inet6_addr* listen_addr, uint16_t port, void *srh, socklen_t srh_size);


int main(int argc, char **argv) {
    // Path to the text file that contains segments, one per line, with the last
    // segment first.  Note that the segment list MUST include the destination
    // address
    const char *SEGMENT_PATH = "segments.txt";


    uint16_t port;
    inet6_addr *addr = NULL;
    void *header_1 = NULL, *header_2 = NULL;
    FILE *segment_file = NULL;

    // Parse the bind address and port from the command line
    int parse_res = parse_connection_options(argc, argv, &addr, &port); 
    if ( parse_res != 0 )
        return parse_res;

    // Debug print the command line options
    char addr_str[INET6_ADDRSTRLEN];
    inet_ntop( AF_INET6, addr, addr_str, INET6_ADDRSTRLEN );

    printf( "Got address %s\n", addr_str );
    printf( "Got port %d\n", port );


    // Open the input file
    if( (segment_file = fopen( SEGMENT_PATH, "r" )) == NULL ) {
        fprintf( stderr, "Error opening segment file `%s`\n", SEGMENT_PATH );
        return 1;
    }

    // Construct the segment routing header based on the contents of the file
    header_2 = build_srh_from_file( segment_file );


    // This is here for debugging purposes, header_1 is built "by hand", instead
    // of dynamically.  Dynamic building has been confirmed to work, but might
    // be useful to keep this plumbing around
    //header_1 = build_test_srh();

    if ( NULL == header_2 ) {
        fprintf( stderr, "Failed to build SRH.\n" );
        return 1;
    }

    socklen_t hdr_len_2 = inet6_rth_space_n( IPV6_RTHDR_TYPE_4, inet6_rth_segments_n( header_2 ));

    printf( "Hex dump of routing header:\n" );
    hex_print( header_2, hdr_len_2 );
    start_server( addr, port , header_2, hdr_len_2);
    
    return 0;

}

void start_server(inet6_addr* listen_addr, uint16_t port, void *srh, socklen_t srh_size) {
    int listen_sock = 0, conn_sock = 0;

    const char *send_data = "PING.\n";

    // Setting up the socket address struct
    struct sockaddr_in6 sock_addr;
    sock_addr.sin6_family = AF_INET6;
    sock_addr.sin6_port = htons(port);
    sock_addr.sin6_flowinfo = 0;

    // Assuming the inet6_addr and in6_addr structs are compatible... why are
    // there two?
    sock_addr.sin6_addr = *((struct in6_addr*)listen_addr);

    sock_addr.sin6_scope_id = 0;


    // Create an IPv6 TCP listen socket
    if (( listen_sock = socket( AF_INET6, SOCK_STREAM, 0 ) ) < 0 ) {
        fprintf( stderr, "Error creating listen socket.\n" );
        fprintf( stderr, "%s\n", strerror(errno) );
        exit( EXIT_FAILURE );
    }
    printf( "Socket fd created...\n" );

    // Tell the kernel that it can recycle the socket structure.  This allows
    // this application to be killed and restarted rapidly without the delay of
    // waiting for the kernel to release the socket
    int yes = '1';
    if ( setsockopt( listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        fprintf( stderr, "Error setting reuseaddr on listen socket.\n" );
        fprintf( stderr, "%s\n", strerror(errno) );
        exit( EXIT_FAILURE );
    } 

    // Bind the listen socket
    if (bind( listen_sock, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in6) ) < 0) {
        fprintf( stderr, "Error binding listen socket.\n" );
        fprintf( stderr, "%s\n", strerror(errno) );
        exit( EXIT_FAILURE );

    }
    printf( "Listen socket bound successfully...\n" );

    // Start listening on the socket
    if (listen( listen_sock, 10) < 0) {
        fprintf( stderr, "Error binding listen socket.\n" );
        fprintf( stderr, "%s\n", strerror(errno) );
        exit( EXIT_FAILURE );
    }

    printf( "Now listening...\n" );

    // We will store the socket address of the client so we can get some
    // information about it
    struct sockaddr_in6 client_socket;
    socklen_t client_socket_len = 0;

    // Accept a client connection. Note I am assuming that we are connecting to
    // another IPv6 socket.  I would need to test what happens if an IPv4
    // connection is attempted here
    if ((conn_sock = accept(listen_sock, (struct sockaddr*)&client_socket, &client_socket_len)) < 0) {
        fprintf( stderr, "Error accepting client connection.\n" );
        fprintf( stderr, "%s\n", strerror(errno) );
        exit( EXIT_FAILURE );
        
    }

    // Set the routing header on the socket
    if ( setsockopt(conn_sock, IPPROTO_IPV6, IPV6_RTHDR, srh, srh_size) < 0 ) {
        fprintf( stderr, "SRH setsockopt failed.  Are you running kernel 4.10 or newer?\n" );
        fprintf( stderr, "%d: %s\n", errno, strerror(errno) );
        exit( EXIT_FAILURE );
    }

    // Get and print the address of the connected client
    char client_addr_str[INET6_ADDRSTRLEN];
    getpeername( conn_sock, (struct sockaddr*)&client_socket, &client_socket_len );
    if ( inet_ntop(AF_INET6, &client_socket.sin6_addr, client_addr_str, INET6_ADDRSTRLEN) )
        printf( "%s connected.\n", client_addr_str );


    // This could be substituted with any TCP application logic that you want to
    // utilize segment routing with.  For now, we just send the ping message on
    // a one second delay.
    while ( 1 ) {
        printf( "Sending ping...\n" );
        send( conn_sock, (void*)send_data, strlen(send_data)+1, 0 );
        sleep( 1 );

    }



}
