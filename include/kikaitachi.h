#ifndef _KIKAITACHI_
#define _KIKAITACHI_

#include <netinet/in.h>

// Logging *********************************************************************

/**
 * Log debug message.
 */
void kt_log_debug(char *format, ...);

/**
 * Log warning message.
 */
void kt_log_warn(char *format, ...);

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

// Telemetry *******************************************************************

enum KT_TELEMETRY_TYPE {
	KT_TELEMETRY_TYPE_GROUP,
	KT_TELEMETRY_TYPE_ACTION,
	KT_TELEMETRY_TYPE_STRING,
	KT_TELEMETRY_TYPE_FLOAT
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

int kt_udp_send(int fd, const void *buf, size_t len);

//int kt_udp_send(int fd, const void *buf, size_t len);

/*void kt_(int port,
	void(*on_connected)(int fd),
	void(*on_disconnected)());

void kt_connect(char *address, int port,
	void(*on_connected)(int fd),
	void(*on_disconnected)(),
	void(*on_telemetry_item_added)(),
	void(*on_telemetry_value_updated)(int id, int len, void *value));*/

#endif

// Messages ********************************************************************

enum KT_MESSAGE {
	KT_MSG_DISCOVER = 0,
	KT_MSG_TELEMETRY = 1,
	KT_MSG_TELEMETRY_DEFINITION = 2,
	KT_MSG_SHUTDOWN = 3,
	KT_MSG_UPDATE = 4,
	KT_MSG_REBOOT = 5
};

int kt_msg_write(void **buf, int *buf_len, void *data, size_t len);
int kt_msg_read(void **buf, int *buf_len, void *data, size_t len);

int kt_msg_write_int(void **buf, int *buf_len, int value);
int kt_msg_read_int(void **buf, int *buf_len, int *value);

int kt_msg_write_float(void **buf, int *buf_len, float value);
int kt_msg_read_float(void **buf, int *buf_len, float *value);

int kt_msg_write_telemetry(void **buf, int *buf_len, int id, enum KT_TELEMETRY_TYPE type, int value_len, void *value);

