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
	KT_TELEMETRY_TYPE_INT,
	KT_TELEMETRY_TYPE_FLOAT
};

// Network *********************************************************************

#define KT_MAX_MSG_SIZE 65507

int kt_udp_bind(char *port);

int kt_udp_connect(char *address, char *port);

int kt_udp_send(int fd, const void *buf, size_t len);

// Messages ********************************************************************

enum KT_MESSAGE {
	KT_MSG_DISCOVER,
	KT_MSG_TELEMETRY,
	KT_MSG_TELEMETRY_DEFINITION,
	KT_MSG_MAP_RESET,
	KT_MSG_MAP_ADD,
	KT_MSG_MAP_SPLIT,
	KT_MSG_MAP_JOIN,
};

int kt_msg_write(void **buf, int *buf_len, void *data, size_t len);
int kt_msg_read(void **buf, int *buf_len, void *data, size_t len);

int kt_msg_write_int(void **buf, int *buf_len, int value);
int kt_msg_read_int(void **buf, int *buf_len, int *value);

int kt_msg_write_float(void **buf, int *buf_len, float value);
int kt_msg_read_float(void **buf, int *buf_len, float *value);

int kt_msg_write_double(void **buf, int *buf_len, double value);
int kt_msg_read_double(void **buf, int *buf_len, double *value);

int kt_msg_write_telemetry(void **buf, int *buf_len, int id, enum KT_TELEMETRY_TYPE type, int value_len, void *value);

#endif

// Map *************************************************************************

enum MAP_NODE_TYPE {
	MAP_NODE_UNKNOW,
	MAP_NODE_FLAT,
	MAP_NODE_SUNKEN,
	MAP_NODE_RISEN,
};

typedef struct map_node {
	enum MAP_NODE_TYPE type;
	float likelihood;
	struct map_node *children[2];
} map_node;

void kt_map_init(float origin_x, float origin_y, float width, float height, float min_size);

void kt_map_free(map_node **node);

void kt_map_add_circle(float center_x, float center_y, float radius, enum MAP_NODE_TYPE type, float likelihood);

void kt_map_traverse(void (*on_node)(float x, float y, float width, float height, enum MAP_NODE_TYPE type, float likelihood, void *data), void *data);

// Ring buffer *****************************************************************

typedef struct {
	double *values;
	int size;
	int index;
	double sum;
} kt_double_ring_buffer;

kt_double_ring_buffer *kt_double_ring_buffer_create(int size);

void kt_double_ring_buffer_add(kt_double_ring_buffer *ring_buffer, double value);

double kt_double_ring_buffer_last(kt_double_ring_buffer *ring_buffer);

double kt_double_ring_buffer_sum(kt_double_ring_buffer *ring_buffer);

void kt_double_ring_buffer_free(kt_double_ring_buffer *ring_buffer);

