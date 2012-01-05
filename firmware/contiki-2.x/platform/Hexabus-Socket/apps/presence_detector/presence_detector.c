/*
 * Copyright (c) 2011, Fraunhofer ESK
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
 *
 * Author: 	G�nter Hildebrandt <guenter.hildebrandt@esk.fraunhofer.de>
 *
 * @(#)$$
 */
#include "hexabus_config.h"
#include "value_broadcast.h"

#include "presence_detector.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/ctimer.h"
#include "contiki.h"
#include "dev/leds.h"

#include "eeprom_variables.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#if PRESENCE_DETECTOR_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static uint8_t presence = 0;
static struct ctimer pd_timeout; 


void presence_detected(void) {
#if PRESENCE_DETECTOR_SERVER
    presence = 1;
    ctimer_restart(&pd_timeout);
    broadcast_value(26);
#elif PRESENCE_DETECTOR_CLIENT
    presence = PRESENCE_DETECTOR_CLIENT_GROUP;
    broadcast_value(26);
#endif
}

void no_presence_detected(void) {
    presence = 0;
#if PRESENCE_DETECTOR_SERVER
    broadcast_value(26);
#endif
}

uint8_t is_presence(void) {
    return presence;
}

void presence_detector_init() {
    ctimer_set(&pd_timeout, CLOCK_SECOND*60*PRESENCE_DETECTOR_SERVER_TIMEOUT, no_presence_detected, NULL);
    ctimer_stop(&pd_timeout);
}

