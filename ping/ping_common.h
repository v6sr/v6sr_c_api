/* Common logic and tools for the SRH ping utilities
 * Author: Dave Sizer
 *
 * Copyright 2017
 */
#ifndef __SRH_PING_COMMON_H__
#define __SRH_PING_COMMON_H__
#include <stdint.h>
#include <stdio.h>
#include <netinet/in.h>

typedef struct inet6_addr inet6_addr;

/* A very basic linked list of strings */
struct str_ll;
struct str_ll {
   char *data;
   struct str_ll *next;
};
typedef struct str_ll str_ll;

/* Append a string to a linked list (it can be null)
 * Args:
 * ll  - The linked list to append to, or a null pointer
 * str - The string to append
 *
 * Return: The modified linked list with the new string
 */
str_ll *str_ll_append( str_ll *ll, char *str );

/* Helper to parse an IPv6 address and port number from the command line
 * Args:
 * argc, argv - Raw command line inputs
 * addr       - Pointer to the address struct pointer the parsed address will be
 *              stored in. This function takes care of allocating the underlying
 *              pointer.
 * port       - Pointer to the parsed port number to be populated by the
 *              function
 *
 * Return: Zero on success, nonzero on failure
 */
int parse_connection_options(int argc, 
        char **argv,
        inet6_addr **addr,
        uint16_t *port);

/* Construct a segment routing header using the segment addresses in the
 * specified file (newline delimited)
 * Args:
 * file - File pointer for the input file
 *
 * Return: A void pointer to the header buffer on success, or NULL on failure
 */
void *build_srh_from_file(FILE *file);

/* Manually construct a segment routing header for testing purposes.
 * 
 * Return: A void pointer to the header buffer
 */
void *build_test_srh();

/* Get a line of text from the specified file, with a maximum size (including
 * the newline character)
 * Args:
 * file - The file pointer to the file to be read from
 * size - The maximum number of bytes to read (result will be truncated if a
 *        newline is not reached by this number of bytes)
 *
 * Return: The read string on success, or NULL on failure or if EOF is reached
 */
char *getline_safe( FILE *file, uint16_t size );

/* Print a hex dump of the specified buffer, 4 bytes per line
 * Args:
 * buf  - A pointer to the buffer to print
 * size - The number of bytes to print
 */
void hex_print(const uint8_t *buf, uint16_t len);

/* Print all of the segment addresses in a segment routing header
 * Args:
 * srh - The header to print from
 */
void print_segment_addresses( void *srh );

/* Parse an IPv6 address string into an IPv6 address structure 
 * Args:
 * str  - The string to parse
 * addr - A pointer to the IPv6 struct pointer to be generated.  It will be
 * allocated by this function.
 *
 * Return: Zero on success, nonzero on failure.
 */
int str_to_inet6( const char *str, inet6_addr **addr );
#endif
