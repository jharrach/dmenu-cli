#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

volatile sig_atomic_t signal_recv;

static void signal_handler(int sig) {
	signal_recv = sig;
}

void init_signal(void) {
	struct sigaction new_action;

	signal_recv = 0;

	new_action.sa_flags = 0;
	sigemptyset(&new_action.sa_mask);
	sigaddset(&new_action.sa_mask, SIGINT);
	sigaddset(&new_action.sa_mask, SIGQUIT);
	sigaddset(&new_action.sa_mask, SIGPIPE);

	new_action.sa_handler = signal_handler;
	sigaction(SIGINT, &new_action, NULL);
	sigaction(SIGQUIT, &new_action, NULL);
	sigaction(SIGPIPE, &new_action, NULL);
}

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

	for (;;) {
		bytes_read = read(0, read_buf, sizeof(read_buf));
		if (bytes_read == -1) {
			perror("smenu: read");
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
					perror("smenu: realloc");
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
					perror("smenu: pvec_push");
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
			perror("smenu: pvec_push");
			return -1;	
		}
		read_buf_leftover = NULL;
		leftover_size = 0;
	}
	return 0;
}

int init_terminal(struct termios *settings) {
	struct termios new_settings;
	int fd = open("/dev/tty", O_RDONLY);

	if (fd == -1) {
		perror("smenu: open");
		return -1;
	}
	if (tcgetattr(fd, settings) == -1) {
		perror("smenu: tcgetattr");
		return -1;
	}
	new_settings = *settings;
	new_settings.c_lflag &= ~(ICANON);
	if (tcsetattr(fd, TCSANOW, &new_settings) == -1) {
		perror("smenu: tcsetattr");
		return -1;
	}
	return fd;
}

void restore_terminal(struct termios const *settings, int fd) {
	tcsetattr(fd, TCSANOW, settings);
}

int show_menu(Pvec const *input, int fd) {
	ssize_t bytes_read;

	for (size_t i = 0; i < input->size; ++i) {
		printf("%s\n", (char const *)input->vec[i]);
	}
	for (char c; signal_recv == 0;) {
		bytes_read = read(fd, &c, 1);
		if (bytes_read == -1 && signal_recv == 0) {
			perror("smenu: read");
			return -1;
		}
		switch (c) {
			case '\x1b':
			case '\x4':
				printf("\n");
				return 0;
			default:
				printf("%d\n", c);
		}
	}
	return 0;
}

int main() {
	Pvec input = { 0 };
	struct termios settings;

	init_signal();
	if (read_input(&input) == -1) {
		return 1;
	}
	int fd = init_terminal(&settings);
	if (fd == -1) {
		return 1;
	}
	if (show_menu(&input, fd) == -1) {
		restore_terminal(&settings, fd);
		return 1;
	}
	restore_terminal(&settings, fd);
	return 0;
}
