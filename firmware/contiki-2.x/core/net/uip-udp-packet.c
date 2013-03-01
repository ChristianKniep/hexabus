/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * $Id: uip-udp-packet.c,v 1.7 2009/10/18 22:02:01 adamdunkels Exp $
 */

/**
 * \file
 *         Module for sending UDP packets through uIP.
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki-conf.h"

extern uint16_t uip_slen;

#include "net/uip-udp-packet.h"

#include "sequence_number.h"

#include <string.h>

#define UIP_IP_BUF                          ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_DESTO_BUF                        ((struct uip_hbho_hdr *)&uip_buf[UIP_LLH_LEN+UIP_IPH_LEN])
#define UIP_SEQNUM_BUF                        ((struct uip_ext_hdr_opt_exp_hxb_seqnum *)&uip_buf[UIP_LLH_LEN+UIP_IPH_LEN+UIP_HBHO_LEN])

/*---------------------------------------------------------------------------*/
void
uip_udp_packet_send(struct uip_udp_conn *c, const void *data, int len)
{
#if UIP_UDP
  if(data != NULL) {
    uip_udp_conn = c;
    uip_slen = len;
    memcpy(&uip_buf[UIP_LLH_LEN + UIP_IPUDPH_LEN], data,
           len > UIP_BUFSIZE? UIP_BUFSIZE: len);
    uip_process(UIP_UDP_SEND_CONN);
	
    /* Insert Hexabus sequence number */
    memmove(&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN + UIP_HBHO_LEN + UIP_EXT_HDR_OPT_EXP_HXB_SEQNUM_LEN],&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN], (len+UIP_UDPH_LEN) > UIP_BUFSIZE? UIP_BUFSIZE: (len+UIP_UDPH_LEN)); // Make space for the DESTO header

    uip_len += UIP_HBHO_LEN + UIP_EXT_HDR_OPT_EXP_HXB_SEQNUM_LEN;
    UIP_IP_BUF->proto = UIP_PROTO_DESTO;
    UIP_IP_BUF->len[0] = ((uip_len - UIP_IPH_LEN) >> 8);
    UIP_IP_BUF->len[1] = ((uip_len - UIP_IPH_LEN) & 0xff);
    UIP_DESTO_BUF->next = UIP_PROTO_UDP;
    UIP_DESTO_BUF->len  = (UIP_HBHO_LEN + UIP_EXT_HDR_OPT_EXP_HXB_SEQNUM_LEN) / 8  - 1;
    UIP_SEQNUM_BUF->tag = UIP_EXT_HDR_OPT_EXP_HXB_SEQNUM;
    UIP_SEQNUM_BUF->len = UIP_EXT_HDR_OPT_EXP_HXB_SEQNUM_LEN - 2;
    UIP_SEQNUM_BUF->seqnum = increase_seqnum();
    UIP_SEQNUM_BUF->flags = 0;

#if UIP_CONF_IPV6
    tcpip_ipv6_output();
#else
    if(uip_len > 0) {
      tcpip_output();
    }
#endif
  }
  uip_slen = 0;
#endif /* UIP_UDP */
}
/*---------------------------------------------------------------------------*/
void
uip_udp_packet_sendto(struct uip_udp_conn *c, const void *data, int len,
		      const uip_ipaddr_t *toaddr, uint16_t toport)
{
  uip_ipaddr_t curaddr;
  uint16_t curport;

  if(toaddr != NULL) {
    /* Save current IP addr/port. */
    uip_ipaddr_copy(&curaddr, &c->ripaddr);
    curport = c->rport;

    /* Load new IP addr/port */
    uip_ipaddr_copy(&c->ripaddr, toaddr);
    c->rport = toport;

    uip_udp_packet_send(c, data, len);

    /* Restore old IP addr/port */
    uip_ipaddr_copy(&c->ripaddr, &curaddr);
    c->rport = curport;
  }
}
/*---------------------------------------------------------------------------*/
