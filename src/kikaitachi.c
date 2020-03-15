#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "kikaitachi.h"

#define MAX_MSG_SIZE 1500

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
	char buffer[MAX_MSG_SIZE];
	buffer[0] = KT_MESSAGE_TELEMETRY_ITEM;
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

