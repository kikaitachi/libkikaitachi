#ifndef _KIKAITACHI_
#define _KIKAITACHI_

#define KT_MESSAGE_TELEMETRY_ITEM 0

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

#endif

