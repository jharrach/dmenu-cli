#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

#include "ansi.h"

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
		tmp = realloc(dst->vec, dst->capacity * sizeof(*(dst->vec)));
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
	new_settings.c_lflag &= ~(ECHO | ICANON);
	if (tcsetattr(fd, TCSANOW, &new_settings) == -1) {
		perror("smenu: tcsetattr");
		return -1;
	}
	fprintf(stderr, ANSI_CUHIDE);
	return fd;
}

void restore_terminal(struct termios const *settings, int fd) {
	tcsetattr(fd, TCSANOW, settings);
	fprintf(stderr, ANSI_CUSHOW);
}

int show_menu(Pvec const *input, int fd) {
	ssize_t bytes_read;
	int ctrl_seq = 0;
	size_t selected_entry = 0;

	for (char c; signal_recv == 0;) {
		fprintf(stderr, ANSI_EL(2) ANSI_CHA(0) );
		for (size_t i = 0;; ++i) {
			if (i == selected_entry) {
				fprintf(stderr, ANSI_BG_BLUE "%s" ANSI_BG_DEFAULT, (char const *)input->vec[i]);
			} else {
				fprintf(stderr, "%s", (char const *)input->vec[i]);
			}
			if (i + 1 == input->size) {
				break;
			}
			fprintf(stderr, " ");
		}
		bytes_read = read(fd, &c, 1);
		if (bytes_read == -1 && signal_recv == 0) {
			perror("smenu: read");
			return -1;
		}
		if (ctrl_seq) {
			if (ctrl_seq == 1 && c == '[') {
				ctrl_seq += 2;
				continue;
			}
			if (ctrl_seq == 3) {
				switch (c) {
					case 'B':
					case 'C':
						if (selected_entry + 1 != input->size) {
							selected_entry += 1;
						}
						break;
					case 'A':
					case 'D':
						if (selected_entry != 0) {
							selected_entry -= 1;
						}
						break;
					default:
						ctrl_seq = 0;
				}
			}
			if (ctrl_seq) {
				ctrl_seq = 0;
				continue;
			}
			ctrl_seq = 0;
		}
		switch (c) {
			case '\x4':
				fprintf(stderr, "\n");
				return 0;
			case '\x1b':
				ctrl_seq = 1;
				break;
			case '\xe':
				if (selected_entry + 1 != input->size) {
					selected_entry += 1;
				}
				break;
			case '\x10':
				if (selected_entry != 0) {
					selected_entry -= 1;
				}
				break;
			case '\xa':
				fprintf(stderr, "\n");
				printf("%s\n", (char const *)input->vec[selected_entry]);
				return 0;
		}
	}
	fprintf(stderr, "\n");
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
