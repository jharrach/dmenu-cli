#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct {
	size_t capacity;
	size_t size;
	void **vec;
} Pvec;

size_t pvec_push(Pvec *dst, void *p) {
	void *tmp;

	if (dst->size == dst->capacity) {
		dst->capacity += 1024;
		tmp = realloc(dst->vec, dst->capacity * sizeof(void *));
		if (tmp == NULL) {
			return SIZE_MAX;
		}
		dst->vec = tmp;
	}
	dst->vec[dst->size] = p;
	++dst->size;
	return dst->size - 1;
}

ssize_t read_input(Pvec *dst) {
	char read_buf[1024];
	char *read_buf_leftover = NULL;
	ssize_t leftover_size = 0;
	ssize_t last_newline;
	ssize_t bytes_read;
	ssize_t additional_size;
	int fd = open("/dev/stdin", O_RDONLY);

	if (fd == -1) {
		return -1;
	}

	for (;;) {
		bytes_read = read(fd, read_buf, sizeof(read_buf));
		if (bytes_read == -1) {
			return -1;
		}
		if (bytes_read == 0) {
			break;
		}
		last_newline = 0;
		for (ssize_t i = 0;; ++i) {
			if (i != bytes_read && read_buf[i] != '\n') {
				continue;
			}
			additional_size = i - last_newline;
			if (additional_size != 0) {
				read_buf_leftover = realloc(read_buf_leftover, leftover_size + additional_size + 1);
				if (read_buf_leftover == NULL) {
					return -1;
				}
				for (ssize_t j = 0; j < additional_size; ++j) {
					read_buf_leftover[leftover_size + j] = read_buf[last_newline + j];
				}
				leftover_size += additional_size;
				read_buf_leftover[leftover_size] = 0;
			}
			if (i == bytes_read) {
				break;
			}
			if (read_buf_leftover != NULL) {
				if (pvec_push(dst, read_buf_leftover) == SIZE_MAX) {
					return -1;	
				}
				read_buf_leftover = NULL;
				leftover_size = 0;
			}
			last_newline = i + 1;
		}
	}
	if (read_buf_leftover != NULL) {
		if (pvec_push(dst, read_buf_leftover) == SIZE_MAX) {
			return -1;	
		}
		read_buf_leftover = NULL;
		leftover_size = 0;
	}
	return 0;
}

int main() {
	Pvec input = { 0 };

	if (read_input(&input) == -1) {
		perror("smenu");
		return 1;
	}
	for (size_t i = 0; i < input.size; ++i) {
		printf("%s\n", (char const *)input.vec[i]);
	}
}
