#ifndef ENDPOINT_REGISTRY_H_
#define ENDPOINT_REGISTRY_H_

#include <stdint.h>
#include <avr/pgmspace.h>
#include "../../../../../../shared/hexabus_types.h"

typedef enum hxb_error_code (*endpoint_read_fn)(uint32_t eid, struct hxb_value* value);
typedef enum hxb_error_code (*endpoint_write_fn)(uint32_t eid, const struct hxb_value* value);

struct endpoint_descriptor {
	uint8_t datatype;
	uint32_t eid;

	PGM_P name;

	// if set to 0, the endpoint will not be readable
	endpoint_read_fn read;
	// if set to 0, the endpoint will not be writable
	endpoint_write_fn write;
};

#define ENDPOINT_DESCRIPTOR const struct endpoint_descriptor PROGMEM

struct endpoint_registry_entry {
	const struct endpoint_descriptor* descriptor;
	struct endpoint_registry_entry* next;
};

extern struct endpoint_registry_entry* _endpoint_chain;

#define ENDPOINT_REGISTER(DESC) \
	do { \
		static struct endpoint_registry_entry _endpoint_entry = { 0 }; \
		_endpoint_entry.descriptor = &DESC; \
		_endpoint_entry.next = _endpoint_chain; \
		_endpoint_chain = &_endpoint_entry; \
	} while (0)

enum hxb_datatype endpoint_get_datatype(uint32_t eid);
enum hxb_error_code endpoint_write(uint32_t eid, const struct hxb_value* value);
enum hxb_error_code endpoint_read(uint32_t eid, struct hxb_value* value);
enum hxb_error_code endpoint_get_name(uint32_t eid, char* buffer, size_t len);

#endif
