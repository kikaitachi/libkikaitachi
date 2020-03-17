#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "kikaitachi.h"

// Logging *********************************************************************

void kt_log_entry(char level, char *format, va_list argptr) {
	struct timespec now_timespec;
	clock_gettime(CLOCK_REALTIME, &now_timespec);
	time_t now_t = now_timespec.tv_sec;
	struct tm *now_tm = localtime(&now_t);
	printf("%02d-%02d-%02d %02d:%02d:%02d.%06ld \033[1m%c \033[0m",
	now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday,
	now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec,
	now_timespec.tv_nsec / 1000, level);
	if (level == 'E') {
		printf("\033[31m");
	}
	vprintf(format, argptr);
	printf("\033[0m \n");
}

void kt_log_debug(char *format, ...) {
	va_list argptr;
	va_start(argptr, format);
	kt_log_entry('D', format, argptr);
	va_end(argptr);
}

void kt_log_error(char *format, ...) {
	va_list argptr;
	va_start(argptr, format);
	kt_log_entry('E', format, argptr);
	va_end(argptr);
}

void kt_log_info(char *format, ...) {
	va_list argptr;
	va_start(argptr, format);
	kt_log_entry('I', format, argptr);
	va_end(argptr);
}

void kt_log_last(char *format, ...) {
	char buffer[1024 * 4];
	snprintf(buffer, sizeof(buffer), "%s: %s", format, strerror(errno));
	va_list argptr;
	va_start(argptr, format);
	kt_log_entry('L', buffer, argptr);
	va_end(argptr);
}

// Telemetry *******************************************************************

kt_telemetry_item *kt_telemetry_create_item(int id, int name_len, char* name, enum KT_TELEMETRY_TYPE type, int value_len, char* value) {
	kt_telemetry_item *item = malloc(sizeof(kt_telemetry_item));
	item->id = id;
	item->name_len = name_len;
	item->name = memcpy(malloc(name_len), name, name_len);
	item->type = type;
	item->value_len = value_len;
	item->value = value == NULL ? NULL : memcpy(malloc(value_len), value, value_len);
	item->child = NULL;
	item->next = NULL;
	return item;
}

void kt_telemetry_free_item(kt_telemetry_item *item) {
	if (item != NULL) {
		kt_telemetry_free_item(item->child);
		kt_telemetry_free_item(item->next);
		free(item->name);
		if (item->value != NULL) {
			free(item->value);
		}
		free(item);
	}
}

int kt_telemetry_send_item(int fd, int parent_id, kt_telemetry_item *item) {
	if (item == NULL) {
		return 0;
	}
	char buffer[KT_MAX_MSG_SIZE];
	buffer[0] = KT_MSG_TELEMETRY_DEFINITION;
	memcpy(buffer + 1, &item->name_len, 4); // Use NETWORK BYTE ORDER
	memcpy(buffer + 5, item->name, item->name_len);
	size_t count = 5 + item->name_len;
	memcpy(buffer + count, &item->value_len, 4); // Use NETWORK BYTE ORDER
	count += 4;
	memcpy(buffer + count, item->value, item->value_len);
	count += item->value_len;
	// Add type, id, parent_id
	ssize_t result = write(fd, buffer, count);
	if (result == -1) {
		return -1;
	}
	if (item->child != NULL) {
		if (kt_telemetry_send_item(fd, item->id, item->child) == -1) {
			return -1;
		}
	}
	if (item->next != NULL) {
		if (kt_telemetry_send_item(fd, parent_id, item->next) == -1) {
			return -1;
		}
	}
	return 0;
}


int kt_telemetry_send(int fd, kt_telemetry_item *item) {
	return kt_telemetry_send_item(fd, 0, item);
}

// Network *********************************************************************

int kt_udp_bind(char *port) {
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		return -1;
	}
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(port));
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
		return -1;
	}
	return fd;
}

int kt_udp_connect(char *address, char *port) {
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		return -1;
	}
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(port));
	inet_pton(AF_INET, address, &addr.sin_addr);
	if (connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
		return -1;
	}
	return fd;
}

int kt_udp_send(int fd, const void *buf, size_t len) {
	return write(fd, buf, len);
}

// Messages ********************************************************************

int kt_msg_write_int(void **buf, int *buf_len, int value) {
	do {
		if (*buf_len < 1) {
			return -1;
		}
		int8_t byte = ((value > 127 ? 1 : 0) << 7) | (value & 127);
		memcpy(*buf, &byte, 1);
		value >>= 7;
		*buf = (int8_t *)*buf + 1;
		*buf_len = *buf_len - 1;
	} while (value > 0);
	return 0;
}

int kt_msg_read_int(void **buf, int *buf_len, int *value) {
	*value = 0;
	for (int i = 0; *buf_len > 0; i += 7) {
		int byte = ((int8_t *)*buf)[0];
		*value = *value | ((byte & 127) << i);
		*buf = (int8_t *)*buf + 1;
		*buf_len = *buf_len - 1;
		if (byte >= 0) {
			return 0;
		}
	}
	return -1;
}

int kt_msg_write_float(void **buf, int *buf_len, float value) {
	if (*buf_len < 4) {
		return -1;
	}
	memcpy(*buf, &value, 4);
	*buf = (int8_t *)*buf + 4;
	*buf_len = *buf_len - 4;
	return 0;
}
int kt_msg_read_float(void **buf, int *buf_len, float *value) {
	if (*buf_len < 4) {
		return -1;
	}
	memcpy(&value, *buf, 4);
	*buf = (int8_t *)*buf + 4;
	*buf_len = *buf_len - 4;
	return 0;
}

int kt_msg_write_telemetry(void **buf, int *buf_len, int id, enum KT_TELEMETRY_TYPE type, int value_len, void *value) {
	return 0;
}

