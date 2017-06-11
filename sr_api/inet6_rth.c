/*
 * This file has been modified from glibc to enable Segment Routing Header
 * version 4, as defined in Section 3 of the following document:
 *
 * https://www.ietf.org/id/draft-ietf-6man-segment-routing-header-06.txt
 *
 * The original license is below.
 *
 * Author: Dave Sizer
 *
 */

/* Copyright (C) 2006-2017 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2006.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <string.h>
#include <netinet/in.h>
#include <netinet/ip6.h>

#include "sr_api.h"


/* RFC 3542, 7.1

   This function returns the number of bytes required to hold a
   Routing header of the specified type containing the specified
   number of segments (addresses).  For an IPv6 Type 0 Routing header,
   the number of segments must be between 0 and 127, inclusive.  */
socklen_t
inet6_rth_space_n (int type, int segments)
{
  switch (type)
    {
    case IPV6_RTHDR_TYPE_4:
      if (segments < 0 || segments > 255 )
          return 0;

      return sizeof (struct ip6_rthdr4) + segments * sizeof (struct in6_addr);
    }

  return 0;
}


/* RFC 3542, 7.2

   This function initializes the buffer pointed to by BP to contain a
   Routing header of the specified type and sets ip6r_len based on the
   segments parameter.  */
void *
inet6_rth_init_n (void *bp, socklen_t bp_len, int type, int segments)
{
  struct ip6_rthdr *rthdr = (struct ip6_rthdr *) bp;

  switch (type)
    {
    case IPV6_RTHDR_TYPE_4:
      /* Make sure the parameters are valid and the buffer is large enough.  */
      if (segments < 0 || segments > 255)
	break;

      socklen_t len = (sizeof (struct ip6_rthdr4)
		       + segments * sizeof (struct in6_addr));
      if (len > bp_len)
	break;

      /* Some implementations seem to initialize the whole memory area.  */
      memset (bp, '\0', len);

      /* Length in units of 8 octets.  */
      rthdr->ip6r_len = segments * sizeof (struct in6_addr) / 8;
      rthdr->ip6r_type = IPV6_RTHDR_TYPE_4;
      return bp;
    }

  return NULL;
}


/* RFC 3542, 7.3

    This function adds the IPv6 address pointed to by addr to the end of
    the Routing header being constructed.  
   
    NOTE: 
    It should not be used during initial construction of the RH, it should
    only be called on a RH with one or more segments
   
*/
int
inet6_rth_add_n (void *bp, const struct in6_addr *addr)
{
  struct ip6_rthdr *rthdr = (struct ip6_rthdr *) bp;

  switch (rthdr->ip6r_type)
    {
      struct ip6_rthdr4 *rthdr4;

    case IPV6_RTHDR_TYPE_4:
      rthdr4 = (struct ip6_rthdr4 *) rthdr;
      uint8_t segments = rthdr4->ip6r4_len * 8 / sizeof (struct in6_addr);

      /* Check if segleft is valid */
      if (segments - rthdr4->ip6r4_segleft < 1)
        return -1;

      /* Check if firstseg is valid */
      if (rthdr4->ip6r4_lastentry >= segments)
          return -1;

      memcpy (&rthdr4->ip6r4_addr[rthdr4->ip6r4_segleft++],
	      addr, sizeof (struct in6_addr));

      rthdr4->ip6r4_lastentry++;

      return 0;
    }

  return -1;
}


/* RFC 3542, 7.4

   This function takes a Routing header extension header (pointed to by
   the first argument) and writes a new Routing header that sends
   datagrams along the reverse of that route.  The function reverses the
   order of the addresses and sets the segleft member in the new Routing
   header to the number of segments.  */
int
inet6_rth_reverse_n (const void *in, void *out)
{
  struct ip6_rthdr *in_rthdr = (struct ip6_rthdr *) in;

  switch (in_rthdr->ip6r_type)
    {
      struct ip6_rthdr4 *in_rthdr4;
      struct ip6_rthdr4 *out_rthdr4;

    case IPV6_RTHDR_TYPE_4:
      in_rthdr4 = (struct ip6_rthdr4 *) in;
      out_rthdr4 = (struct ip6_rthdr4 *) out;

      /* Copy header, not the addresses.  The memory regions can overlap.  */
      memmove (out_rthdr4, in_rthdr4, sizeof (struct ip6_rthdr4));

      int total = in_rthdr4->ip6r4_len * 8 / sizeof (struct in6_addr);
      for (int i = 0; i < total / 2; ++i)
	{
	  /* Remember, IN_RTHDR0 and OUT_RTHDR0 might overlap.  */
	  struct in6_addr temp = in_rthdr4->ip6r4_addr[i];
	  out_rthdr4->ip6r4_addr[i] = in_rthdr4->ip6r4_addr[total - 1 - i];
	  out_rthdr4->ip6r4_addr[total - 1 - i] = temp;
	}
      if (total % 2 != 0 && in != out)
	out_rthdr4->ip6r4_addr[total / 2] = in_rthdr4->ip6r4_addr[total / 2];

      out_rthdr4->ip6r4_segleft = total;

      return 0;

    }

  return -1;
}


/* RFC 3542, 7.5

   This function returns the number of segments (addresses) contained in
   the Routing header described by BP.  */
int
inet6_rth_segments_n (const void *bp)
{
  struct ip6_rthdr *rthdr = (struct ip6_rthdr *) bp;

  switch (rthdr->ip6r_type)
    {
    case IPV6_RTHDR_TYPE_4:

      return rthdr->ip6r_len * 8 / sizeof (struct in6_addr);
    }

  return -1;
}


/* RFC 3542, 7.6

   This function returns a pointer to the IPv6 address specified by
   index (which must have a value between 0 and one less than the
   value returned by 'inet6_rth_segments') in the Routing header
   described by BP.  */
struct in6_addr *
inet6_rth_getaddr_n (const void *bp, int index)
{
  struct ip6_rthdr *rthdr = (struct ip6_rthdr *) bp;

  switch (rthdr->ip6r_type)
    {
       struct ip6_rthdr4 *rthdr4;
    case IPV6_RTHDR_TYPE_4:
      rthdr4 = (struct ip6_rthdr4 *) rthdr;

      if (index >= rthdr4->ip6r4_len * 8 / sizeof (struct in6_addr))
	break;

      return &rthdr4->ip6r4_addr[index];
    }

  return NULL;
}
