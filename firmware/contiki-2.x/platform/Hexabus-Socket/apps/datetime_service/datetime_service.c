#include "datetime_service.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"

#include "eeprom_variables.h"
#include <avr/eeprom.h>

#include <stdio.h>

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static process_event_t dt_update_event;
static struct datetime current_dt;
static bool time_valid;

void updateDatetime(struct datetime* dt) {
    process_post(&datetime_service_process, dt_update_event, dt);
}

int getDatetime(struct datetime *dt) {

    if(time_valid) {
        dt->hour = current_dt.hour;
        dt->minute = current_dt.minute;
        dt->second = current_dt.second;
        dt->day = current_dt.day;
        dt->month = current_dt.month;
        dt->year = current_dt.year;
        dt->weekday = current_dt.weekday;
        return 0;
    } else {
        return -1;
    }
}

PROCESS(datetime_service_process, "Keeps the Date and Time up-to-date\n");

AUTOSTART_PROCESSES(&datetime_service_process);

PROCESS_THREAD(datetime_service_process, ev, data) {

    static struct etimer update_timer;
    static struct etimer valid_timer;

    PROCESS_BEGIN();

    time_valid = false;
    etimer_set(&update_timer, CLOCK_SECOND * UPDATE_INTERVALL);
    etimer_set(&valid_timer, CLOCK_SECOND * 10);

    while(1) {
        PROCESS_WAIT_EVENT();

        PRINTF("Time: %d:%d:%d\t%d.%d.%d Day: %d Valid: %d\n", current_dt.hour, current_dt.minute, current_dt.second, current_dt.day, current_dt.month, current_dt.year, current_dt.weekday, time_valid);
        if(ev == PROCESS_EVENT_TIMER) {

            etimer_reset(&update_timer);
            if(!etimer_expired(&valid_timer)&& time_valid) {
                current_dt.second+=1;
                if(current_dt.second > 59) {
                    current_dt.second = 0;
                    current_dt.minute+=1;
                    if(current_dt.minute > 59) {
                        current_dt.minute = 0;
                        current_dt.hour+=1;
                        if(current_dt.hour > 23) {
                            current_dt.hour = 0;
                            current_dt.day+=1;
                            current_dt.weekday+=1;
                            if(current_dt.weekday>6) {
                                current_dt.weekday=0;
                            }
                            if( ((current_dt.month==1||current_dt.month==3||current_dt.month==5||current_dt.month==7||current_dt.month==8||current_dt.month==10||current_dt.month==12)&&current_dt.day>31) || ((current_dt.month==4||current_dt.month==6||current_dt.month==9||current_dt.month==11)&&current_dt.day>30) || (current_dt.month==2 && current_dt.day>28)) { //TODO Schaltjahre >.<
                                current_dt.day=0;
                                current_dt.month+=1;
                                if(current_dt.month>12) {
                                    current_dt.month=0;
                                    current_dt.year+=1;
                                }
                            }
                        }
                    }
                }
            } else {
                time_valid = false;
            }
        } else if(ev == dt_update_event) {
            
            current_dt.second = ((struct hxb_data*)data)->value.datetime.second;
            current_dt.minute = ((struct hxb_data*)data)->value.datetime.minute;
            current_dt.hour = ((struct hxb_data*)data)->value.datetime.hour;
            current_dt.day = ((struct hxb_data*)data)->value.datetime.day;
            current_dt.month = ((struct hxb_data*)data)->value.datetime.month;
            current_dt.year = ((struct hxb_data*)data)->value.datetime.year;
            current_dt.weekday = ((struct hxb_data*)data)->value.datetime.weekday;
            
            free(data);

            time_valid = true;

            etimer_restart(&valid_timer);
            etimer_reset(&update_timer);
        }
    }
    PROCESS_END();
}



    

