#include <glib.h>
#include "kikaitachi.h"

static void test_kt_msg_write_int_buf_len_is_0() {
	int buf_len = 0;
	g_assert_cmpint(-1, ==, kt_msg_write_int(NULL, &buf_len, 123));
}

static void test_kt_msg_write_int_positive_less_than_7_bits() {
	char buf[1];
	void *buf_ptr = buf;
	int buf_len = sizeof(buf);
	g_assert_cmpint(0, ==, kt_msg_write_int(&buf_ptr, &buf_len, 8));
	g_assert_cmpint(8, ==, buf[0]);
}

static void test_kt_msg_write_int_negative_less_than_7_bits() {
	char buf[1];
	void *buf_ptr = buf;
	int buf_len = sizeof(buf);
	g_assert_cmpint(0, ==, kt_msg_write_int(&buf_ptr, &buf_len, -8));
	g_assert_cmpint(72, ==, buf[0]);
}

static void test_kt_msg_write_int_positive_more_than_7_bits() {
	char buf[2];
	void *buf_ptr = buf;
	int buf_len = sizeof(buf);
	g_assert_cmpint(0, ==, kt_msg_write_int(&buf_ptr, &buf_len, 64));
	g_assert_cmpint(-128, ==, buf[0]);
	g_assert_cmpint(1, ==, buf[1]);
}

static void test_kt_msg_read_int_positive_more_than_7_bits() {
	int value;
	char buf[2] = { -128, 1 };
	void *buf_ptr = buf;
	int buf_len = sizeof(buf);
	g_assert_cmpint(0, ==, kt_msg_read_int(&buf_ptr, &buf_len, &value));
	g_assert_cmpint(64, ==, value);
}

static void test_kt_msg_write_and_read_int_negative_more_than_7_bits() {
	int value;
	char buf[5];
	void *buf_ptr = buf;
	int buf_len = sizeof(buf);
	g_assert_cmpint(0, ==, kt_msg_write_int(&buf_ptr, &buf_len, -123456789));
	buf_ptr = buf;
	buf_len = sizeof(buf);
	g_assert_cmpint(0, ==, kt_msg_read_int(&buf_ptr, &buf_len, &value));
	g_assert_cmpint(-123456789, ==, value);
}

int main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/serialization/kt_msg_write_int/buf_len_is_0", test_kt_msg_write_int_buf_len_is_0);
	g_test_add_func("/serialization/kt_msg_write_int/positive_less_than_7_bits", test_kt_msg_write_int_positive_less_than_7_bits);
	g_test_add_func("/serialization/kt_msg_write_int/negative_less_than_7_bits", test_kt_msg_write_int_negative_less_than_7_bits);
	g_test_add_func("/serialization/kt_msg_write_int/positive_more_than_7_bits", test_kt_msg_write_int_positive_more_than_7_bits);
	g_test_add_func("/serialization/kt_msg_read_int/positive_more_than_7_bits", test_kt_msg_read_int_positive_more_than_7_bits);
	g_test_add_func("/serialization/kt_msg_write_and_read_int/negative_more_than_7_bits", test_kt_msg_write_and_read_int_negative_more_than_7_bits);

	return g_test_run();
}

