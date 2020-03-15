#include <stdlib.h>
#include <string.h>
#include "kikaitachi.h"

kt_telemetry_item *kt_telemetry_create_item(int id, int name_len, char* name, enum KT_TELEMETRY_TYPE type, int value_len, char* value) {
	kt_telemetry_item *item = malloc(sizeof(kt_telemetry_item));
	item->id = id;
	item->name_len = name_len;
	item->name = memcpy(malloc(name_len), name, name_len);
	item->type = type;
	item->value_len = value_len;
	item->value = memcpy(malloc(value_len), value, value_len);
	item->child = NULL;
	item->next = NULL;
	return item;
}

void kt_telemetry_free_item(kt_telemetry_item *item) {
	if (item != NULL) {
		kt_telemetry_free_item(item->child);
		kt_telemetry_free_item(item->next);
		free(item->name);
		free(item->value);
		free(item);
	}
}

