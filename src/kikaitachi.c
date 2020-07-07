#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
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

void kt_log_warn(char *format, ...) {
	va_list argptr;
	va_start(argptr, format);
	kt_log_entry('W', format, argptr);
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

// Network *********************************************************************

int kt_udp_bind(char *port) {
	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

int kt_msg_write(void **buf, int *buf_len, void *data, size_t len) {
	if (*buf_len < len) {
		return -1;
	}
	memcpy(*buf, data, len);
	*buf = (int8_t *)*buf + len;
	*buf_len = *buf_len - len;
	return 0;
}

int kt_msg_read(void **buf, int *buf_len, void *data, size_t len) {
	if (*buf_len < len) {
		return -1;
	}
	memcpy(data, *buf, len);
	*buf = (int8_t *)*buf + len;
	*buf_len = *buf_len - len;
	return 0;
}

int kt_msg_write_int(void **buf, int *buf_len, int value) {
	if (*buf_len < 1) {
		return -1;
	}
	int sign;
	if (value < 0) {
		sign = 1 << 6;
		value = -value;
	} else {
		sign = 0;
	}
	int8_t byte = ((value > 63 ? 1 : 0) << 7) | sign | (value & 63);
	memcpy(*buf, &byte, 1);
	value >>= 6;
	*buf = (int8_t *)*buf + 1;
	*buf_len = *buf_len - 1;
	while (value > 0) {
		if (*buf_len < 1) {
			return -1;
		}
		int8_t byte = ((value > 127 ? 1 : 0) << 7) | (value & 127);
		memcpy(*buf, &byte, 1);
		value >>= 7;
		*buf = (int8_t *)*buf + 1;
		*buf_len = *buf_len - 1;
	}
	return 0;
}

int kt_msg_read_int(void **buf, int *buf_len, int *value) {
	int negative;
	for (int i = 0; *buf_len > 0; ) {
		int byte = ((int8_t *)*buf)[0];
		if (i == 0) {
			*value = byte & 63;
			negative = (byte & (1 << 6)) > 0;
			i += 6;
		} else {
			*value = *value | ((byte & 127) << i);
			i += 7;
		}
		*buf = (int8_t *)*buf + 1;
		*buf_len = *buf_len - 1;
		if (byte >= 0) {
			if (negative) {
				*value = -*value;
			}
			return 0;
		}
	}
	return -1;
}

int kt_msg_write_float(void **buf, int *buf_len, float value) {
	return kt_msg_write(buf, buf_len, &value, 4);
}
int kt_msg_read_float(void **buf, int *buf_len, float *value) {
	return kt_msg_read(buf, buf_len, value, 4);
}

int kt_msg_write_double(void **buf, int *buf_len, double value) {
	return kt_msg_write(buf, buf_len, &value, 8);
}

int kt_msg_read_double(void **buf, int *buf_len, double *value) {
	return kt_msg_read(buf, buf_len, value, 8);
}

int kt_msg_write_telemetry(void **buf, int *buf_len, int id, enum KT_TELEMETRY_TYPE type, int value_len, void *value) {
	return 0;
}

// Util ************************************************************************

int kt_clamp_int(int value, int min, int max) {
	return value < min ? min : value > max ? max : value;
}

float kt_clamp_float(float value, float min, float max) {
	return value < min ? min : value > max ? max : value;
}

double kt_clamp_double(double value, double min, double max) {
	return value < min ? min : value > max ? max : value;
}

// Map *************************************************************************

static float map_origin_x, map_origin_y, map_width, map_height, map_min_size;
static map_node *map = NULL;

void kt_map_free(map_node **node) {
	if (*node != NULL) {
		kt_map_free(&((*node)->children[0]));
		kt_map_free(&((*node)->children[1]));
		free(*node);
		*node = NULL;
	}
}

static map_node *new_map_node(enum MAP_NODE_TYPE type, float likelihood) {
	map_node *node = malloc(sizeof(map_node));
	node->type = type;
	node->likelihood = likelihood;
	node->children[0] = NULL;
	node->children[1] = NULL;
	return node;
}

void kt_map_init(float origin_x, float origin_y, float width, float height, float min_size) {
	map_origin_x = origin_x;
	map_origin_y = origin_y;
	map_width = width;
	map_height = height;
	map_min_size = min_size;
	kt_map_free(&map);
	map = new_map_node(MAP_NODE_UNKNOW, 0);
}

static int rect_contains_circle(float rect_x, float rect_y, float rect_width, float rect_height, float circle_x, float circle_y, float circle_radius) {
	return rect_x <= circle_x - circle_radius &&
		rect_y <= circle_y - circle_radius &&
		rect_x + rect_width >= circle_x + circle_radius &&
		rect_y + rect_width >= circle_y + circle_radius;
}

static float squared_dist(float x1, float y1, float x2, float y2) {
	float dist_x = x1 - x2;
	float dist_y = y1 - y2;
	return dist_x * dist_x + dist_y * dist_y;
}

static int circle_contains_rect(float circle_x, float circle_y, float circle_radius, float rect_x, float rect_y, float rect_width, float rect_height) {
	float radius_squared = circle_radius * circle_radius;
	return squared_dist(circle_x, circle_y, rect_x, rect_y) <= radius_squared &&
		squared_dist(circle_x, circle_y, rect_x + rect_width, rect_y) <= radius_squared &&
		squared_dist(circle_x, circle_y, rect_x, rect_y + rect_height) <= radius_squared &&
		squared_dist(circle_x, circle_y, rect_x + rect_width, rect_y + rect_height) <= radius_squared;
}

static int circle_intersects_rect(float circle_x, float circle_y, float circle_radius, float rect_x, float rect_y, float rect_width, float rect_height) {
	// Find the closest point to the circle within the rectangle
	float closestX = kt_clamp_float(circle_x, rect_x, rect_x + rect_width);
	float closestY = kt_clamp_float(circle_y, rect_y, rect_y + rect_height);

	// Calculate the distance between the circle's center and this closest point
	float distanceX = circle_x - closestX;
	float distanceY = circle_y - closestY;

	// If the distance is less than the circle's radius, an intersection occurs
	float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
	return distanceSquared < circle_radius * circle_radius;
}

/*static int circle_intersects_rect(float circle_x, float circle_y, float circle_radius, float rect_x, float rect_y, float rect_width, float rect_height) {
	float radius_squared = circle_radius * circle_radius;
	return squared_dist(circle_x, circle_y, rect_x, rect_x) <= radius_squared ||
		squared_dist(circle_x, circle_y, rect_x + rect_width, rect_x) <= radius_squared ||
		squared_dist(circle_x, circle_y, rect_x, rect_x + rect_height) <= radius_squared ||
		squared_dist(circle_x, circle_y, rect_x + rect_width, rect_x + rect_height) <= radius_squared;
}*/

static void update_map_node(map_node *node, enum MAP_NODE_TYPE type, float likelihood) {
	if (likelihood >= map->likelihood) {
		node->type = type;
		node->likelihood = likelihood;
		// Unsplit node if split
		kt_map_free(&node->children[0]);
		kt_map_free(&node->children[1]);
	}
}

static void add_circle(
		map_node *node,
		float rect_x, float rect_y, float rect_width, float rect_height,
		float circle_x, float circle_y, float circle_radius, enum MAP_NODE_TYPE type, float likelihood) {
	if (!circle_intersects_rect(circle_x, circle_y, circle_radius, rect_x, rect_y, rect_width, rect_height)) {
		return;
	}
	if (circle_contains_rect(circle_x, circle_y, circle_radius, rect_x, rect_y, rect_width, rect_height)) {
		update_map_node(node, type, likelihood);
	} else {
		// If still not too small to split
		if (rect_width > map_min_size || rect_height > map_min_size) {
			if (node->children[0] == NULL) {
				// Split
				node->children[0] = new_map_node(node->type, node->likelihood);
				node->children[1] = new_map_node(node->type, node->likelihood);
			}
			if (rect_width > rect_height) {
				add_circle(node->children[0], rect_x, rect_y, rect_width / 2, rect_height,
					circle_x, circle_y, circle_radius, type, likelihood);
				add_circle(node->children[1], rect_x + rect_width / 2, rect_y, rect_width / 2, rect_height,
					circle_x, circle_y, circle_radius, type, likelihood);
			} else {
				add_circle(node->children[0], rect_x, rect_y, rect_width, rect_height / 2,
					circle_x, circle_y, circle_radius, type, likelihood);
				add_circle(node->children[1], rect_x, rect_y + rect_height / 2, rect_width, rect_height / 2,
					circle_x, circle_y, circle_radius, type, likelihood);
			}
		} else {
			// Since not whole node is covered by circle reduce likelyhood by 5%
			update_map_node(node, type, likelihood - 0.05 * likelihood);
		}
	}
}

void kt_map_add_circle(float center_x, float center_y, float radius, enum MAP_NODE_TYPE type, float likelihood) {
	if (!rect_contains_circle(map_origin_x, map_origin_y, map_width, map_height, center_x, center_y, radius)) {
		// TODO: extend map
	}
	add_circle(
		map,
		map_origin_x, map_origin_y, map_width, map_height,
		center_x, center_y, radius, type, likelihood);
}

void map_traverse_node(
		map_node *node, float x, float y, float width, float height,
		void (*on_node)(float x, float y, float width, float height, enum MAP_NODE_TYPE type, float likelihood, void *data), void *data) {
	if (node != NULL) {
		if (node->children[0] == NULL) {
			on_node(x, y, width, height, node->type, node->likelihood, data);
		} else {
			if (width > height) {
				map_traverse_node(node->children[0], x, y, width / 2, height, on_node, data);
				map_traverse_node(node->children[1], x + width / 2, y, width / 2, height, on_node, data);
			} else {
				map_traverse_node(node->children[0], x, y, width, height / 2, on_node, data);
				map_traverse_node(node->children[1], x, y + height / 2, width, height / 2, on_node, data);
			}
		}
	}
}

void kt_map_traverse(void (*on_node)(float x, float y, float width, float height, enum MAP_NODE_TYPE type, float likelihood, void *data), void *data) {
	map_traverse_node(map, map_origin_x, map_origin_y, map_width, map_height, on_node, data);
}

// Ring buffer *****************************************************************

kt_double_ring_buffer *kt_double_ring_buffer_create(int size) {
	kt_double_ring_buffer *ring_buffer = malloc(sizeof(kt_double_ring_buffer));
	ring_buffer->values = malloc(size * sizeof(double));
	for (int i = 0; i < size; i++) {
		ring_buffer->values[i] = 0;
	}
	ring_buffer->size = size;
	ring_buffer->index = 0;
	ring_buffer->sum = 0;
	return ring_buffer;
}

void kt_double_ring_buffer_add(kt_double_ring_buffer *ring_buffer, double value) {
	ring_buffer->index++;
	if (ring_buffer->index == ring_buffer->size) {
		ring_buffer->index = 0;
	}
	ring_buffer->sum = ring_buffer->sum - ring_buffer->values[ring_buffer->index] + value;
	ring_buffer->values[ring_buffer->index] = value;
}

double kt_double_ring_buffer_last(kt_double_ring_buffer *ring_buffer) {
	return ring_buffer->values[ring_buffer->index];
}

double kt_double_ring_buffer_sum(kt_double_ring_buffer *ring_buffer) {
	return ring_buffer->sum;
}

void kt_double_ring_buffer_free(kt_double_ring_buffer *ring_buffer) {
	free(ring_buffer->values);
	free(ring_buffer);
}

// Camera **********************************************************************

#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct buffer {
  void *start;
  size_t length;
};

int xioctl(int fh, int request, void *arg) {
  int r;
  do {
    r = v4l2_ioctl(fh, request, arg);
  } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));
  return r;
}

struct v4l2_buffer buf;
unsigned int n_buffers;
struct buffer *buffers;

int kt_camera_open(char* device, int width, int height, int format, int numerator, int denominator, int buf_count) {
  int fd = v4l2_open(device, O_RDWR | O_NONBLOCK);
  if (fd < 0) {
    kt_log_last("Can't open video device %s", device);
    return fd;
  }

  struct v4l2_format fmt;
  CLEAR(fmt);
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = width;
  fmt.fmt.pix.height      = height;
  fmt.fmt.pix.pixelformat = format;
  fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
  if (xioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
    kt_log_last("Can't set camera format");
    return -1;
  }
  if (fmt.fmt.pix.pixelformat != format) {
    kt_log_error("Given format is not supported by camera");
    return -1;
  }

  struct v4l2_streamparm frameint;
  CLEAR(frameint);
  frameint.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  frameint.parm.capture.timeperframe.numerator = numerator;
  frameint.parm.capture.timeperframe.denominator = denominator;
  if (xioctl(fd, VIDIOC_S_PARM, &frameint) == -1) {
    kt_log_last("Can't set camera frame rate");
    return -1;
  }

  struct v4l2_requestbuffers req;
  CLEAR(req);
  req.count = buf_count;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
    kt_log_last("Can't allocate buffers for camera");
    return -1;
  }

  buffers = calloc(req.count, sizeof(*buffers));
  for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
    CLEAR(buf);
    buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory      = V4L2_MEMORY_MMAP;
    buf.index       = n_buffers;

    xioctl(fd, VIDIOC_QUERYBUF, &buf);

    buffers[n_buffers].length = buf.length;
    buffers[n_buffers].start = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

    if (MAP_FAILED == buffers[n_buffers].start) {
      kt_log_last("Failed to map memory for camera");
      return -1;
    }
  }

  for (int i = 0; i < n_buffers; ++i) {
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    xioctl(fd, VIDIOC_QBUF, &buf);
  }

  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (xioctl(fd, VIDIOC_STREAMON, &type) == -1) {
    kt_log_last("Failed to start capturing from camera");
    return -1;
  }

  return fd;
}

void kt_camera_list_formats(int camera_fd,
    void (*process_format)(int pixelformat, int width, int height, int numerator, int denominator, char *description)) {
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  struct v4l2_fmtdesc fmt;
  struct v4l2_frmsizeenum frmsize;
  struct v4l2_frmivalenum frmival;
  fmt.index = 0;
  fmt.type = type;
  while (ioctl(camera_fd, VIDIOC_ENUM_FMT, &fmt) >= 0) {
    frmsize.pixel_format = fmt.pixelformat;
    frmsize.index = 0;
    while (ioctl(camera_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0) {
      if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
        frmival.pixel_format = fmt.pixelformat;
        frmival.index = 0;
        frmival.width = frmsize.discrete.width;
        frmival.height = frmsize.discrete.height;
        while (ioctl(camera_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) >= 0) {
          process_format(fmt.pixelformat, frmival.width, frmival.height, frmival.discrete.numerator, frmival.discrete.denominator, (char *)fmt.description);
          frmival.index++;
        }
      }
      frmsize.index++;
    }
    fmt.index++;
  }
}

void kt_camera_capture_image(int camera_fd, void (*process_image)(unsigned char *frame, int length, struct timeval timestamp)) {
  CLEAR(buf);
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  xioctl(camera_fd, VIDIOC_DQBUF, &buf);
  process_image(buffers[buf.index].start, buf.bytesused, buf.timestamp);
  xioctl(camera_fd, VIDIOC_QBUF, &buf);
}

void kt_camera_close(int camera_fd) {
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  xioctl(camera_fd, VIDIOC_STREAMOFF, &type);
  for (int i = 0; i < n_buffers; ++i) {
    v4l2_munmap(buffers[i].start, buffers[i].length);
  }
  v4l2_close(camera_fd);
}

