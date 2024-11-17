#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <sys/ioctl.h>

#include "ansi.h"

static char const *selected_entry_bg_color = ANSI_BG_BLUE;

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
	sigaddset(&new_action.sa_mask, SIGWINCH);

	new_action.sa_handler = signal_handler;
	sigaction(SIGINT, &new_action, NULL);
	sigaction(SIGQUIT, &new_action, NULL);
	sigaction(SIGPIPE, &new_action, NULL);
	sigaction(SIGWINCH, &new_action, NULL);
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

typedef struct {
	size_t capacity;
	size_t size;
	char *str;
} String;

size_t strlen(char const *s) {
	size_t len = 0;
	for (; s[len]; ++len) ;
	return len;
}

int string_init(String *s) {
	s->capacity = 3;
	s->str = malloc(s->capacity * sizeof(*(s->str)));
	if (s->str == NULL) {
		return -1;
	}
	s->size = 0;
	s->str[s->size] = 0;
	return 0;
}

int string_push_char_array(String *dst, char const *src) {
	size_t src_size = strlen(src);
	char *tmp;

	size_t new_size_with_terminator = dst->size + src_size + 1; // TODO overflow
	if (new_size_with_terminator > dst->capacity) {
		dst->capacity += 3;
		if (new_size_with_terminator > dst->capacity) {
			dst->capacity = new_size_with_terminator;
		}
		tmp = realloc(dst->str, dst->capacity * sizeof(*(dst->str)));
		if (tmp == NULL) {
			return -1;
		}
		dst->str = tmp;
	}
	for (size_t i = 0; i < src_size; ++i) {
		dst->str[i + dst->size] = src[i];
	}
	dst->size = new_size_with_terminator - 1;
	dst->str[dst->size] = 0;
	return 0;
}

int string_push_n_char_array(String *dst, char const *src, size_t n) {
	size_t src_size = strlen(src);
	char *tmp;

	if (src_size > n) {
		src_size = n;
	}
	size_t new_size_with_terminator = dst->size + src_size + 1; // TODO overflow
	if (new_size_with_terminator > dst->capacity) {
		dst->capacity += 3;
		if (new_size_with_terminator > dst->capacity) {
			dst->capacity = new_size_with_terminator;
		}
		tmp = realloc(dst->str, dst->capacity * sizeof(*(dst->str)));
		if (tmp == NULL) {
			return -1;
		}
		dst->str = tmp;
	}
	for (size_t i = 0; i < src_size; ++i) {
		dst->str[i + dst->size] = src[i];
	}
	dst->size = new_size_with_terminator - 1;
	dst->str[dst->size] = 0;
	return 0;
}

void string_empty(String *s) {
	s->size = 0;
	s->str[s->size] = 0;
}

void string_debug(String const *s) {
	printf("{\n\tstr: \"%s\",\n\tsize: %lu,\n\tcapacity: %lu\n}\n", s->str, s->size, s->capacity);
}

void string_print(String const *s, int fd) {
	write(fd, s->str, s->size);
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

void print_menu(String *output_buf, Pvec const *input, size_t selected_entry, struct winsize const *w) {
	unsigned int available_size;

	if (w->ws_col < 10 || input->size == 0) {
		return;
	}
	available_size = w->ws_col - 6;
	string_empty(output_buf);
	string_push_char_array(output_buf, ANSI_EL(2) ANSI_CHA(0));
	if (strlen(input->vec[selected_entry]) + 2 > available_size) {
		string_push_char_array(output_buf, selected_entry != 0 ? " < " : "   ");
		string_push_char_array(output_buf, selected_entry_bg_color);
		string_push_char_array(output_buf, " ");
		string_push_n_char_array(output_buf, (char const *)input->vec[selected_entry], available_size - 4);
		string_push_char_array(output_buf, "...");
		string_push_char_array(output_buf, ANSI_BG_DEFAULT);
		string_push_char_array(output_buf, selected_entry + 1 != input->size ? " > " : "   ");
		string_print(output_buf, 2);
		return;
	}
	size_t page_size = 0;
	size_t page_start = 0;
	// rules out any string larger than available size from page_start to selelcted_entry
	for (size_t i = 0;;) {
		page_size += strlen((char const *)input->vec[i]) + 2;
		if (page_size > available_size && page_start != i) {
			page_start = i;
			page_size = 0;
			continue;
		}
		if (i == selected_entry) {
			break;
		}
		++i;
	}
	string_push_char_array(output_buf, page_start != 0 ? " < " : "   ");
	size_t i = page_start;
	for (;i < input->size; ++i) {
		size_t i_size = strlen((char const *)input->vec[i]) + 2;
		if (available_size < i_size) {
			break;
		}
		available_size -= i_size;
		if (i == selected_entry) {
			string_push_char_array(output_buf, ANSI_BG_BLUE);
			string_push_char_array(output_buf, " ");
			string_push_char_array(output_buf, (char const *)input->vec[i]);
			string_push_char_array(output_buf, " ");
			string_push_char_array(output_buf, ANSI_BG_DEFAULT);
		} else {
			string_push_char_array(output_buf, " ");
			string_push_char_array(output_buf, (char const *)input->vec[i]);
			string_push_char_array(output_buf, " ");
		}
	}
	string_push_char_array(output_buf, i != input->size ? " > " : "   ");
	string_print(output_buf, 2);
}

int show_menu(Pvec const *input, int fd) {
	ssize_t bytes_read;
	int ctrl_seq = 0;
	size_t selected_entry = 0;
	struct winsize w;
	String output_buf;

	string_init(&output_buf);
	ioctl(1, TIOCGWINSZ, &w);
	for (char c; signal_recv == 0 || signal_recv == SIGWINCH;) {
		if (signal_recv == SIGWINCH) {
			ioctl(1, TIOCGWINSZ, &w);
		}
		print_menu(&output_buf, input, selected_entry, &w);
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
				if (selected_entry + 1 < input->size) {
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
