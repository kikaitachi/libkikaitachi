#ifndef _KIKAITACHI_
#define _KIKAITACHI_

#include <netinet/in.h>

// Logging *********************************************************************

/**
 * Log debug message.
 */
void kt_log_debug(char *format, ...);

/**
 * Log error message.
 */
void kt_log_error(char *format, ...);

/**
 * Log information message.
 */
void kt_log_info(char *format, ...);

/**
 * Log error provided by errno from last system call.
 */
void kt_log_last(char *format, ...);

// Messages ********************************************************************

#define KT_MESSAGE_TELEMETRY_ITEM 0

void kt_msg_set_uint32(void *buffer, uint32_t host_int);
uint32_t kt_msg_get_uint32(void *buffer);

// Telemetry *******************************************************************

enum KT_TELEMETRY_TYPE {
	KT_TELEMETRY_TYPE_GROUP,
	KT_TELEMETRY_TYPE_ACTION,
	KT_TELEMETRY_TYPE_STRING,
	KT_TELEMETRY_TYPE_DOUBLE
};

typedef struct kt_telemetry_item {
	int id;
	int name_len;
	char *name;
	enum KT_TELEMETRY_TYPE type;
	int value_len;
	void *value;
	struct kt_telemetry_item *child;
	struct kt_telemetry_item *next;
} kt_telemetry_item;

kt_telemetry_item *kt_telemetry_create_item(int id, int name_len, char* name, enum KT_TELEMETRY_TYPE type, int value_len, char* value);

void kt_telemetry_free_item(kt_telemetry_item *item);

int kt_telemetry_send(int fd, kt_telemetry_item *item);

// Network *********************************************************************

#define KT_MAX_MSG_SIZE 65507

int kt_udp_bind(char *port);

int kt_udp_connect(char *address, char *port);

//int kt_udp_send(int fd, const void *buf, size_t len);

/*void kt_(int port,
	void(*on_connected)(int fd),
	void(*on_disconnected)());

void kt_connect(char *address, int port,
	void(*on_connected)(int fd),
	void(*on_disconnected)(),
	void(*on_telemetry_item_added)(),
	void(*on_telemetry_value_updated)(int id, int len, void *value));

void kt_send(int fd, ssize_t len, void *buffer);*/

#endif

