CC      = gcc
STD     = -std=c11 -Wall -Wextra -Wpedantic -Werror
SAN     ?= 0

ifeq ($(SAN),1)
SAN_FLAGS = -g -fsanitize=address,undefined -fno-omit-frame-pointer
else
SAN_FLAGS =
endif

CFLAGS  = $(STD) $(SAN_FLAGS)

TEST_BINS  = t_stack_array t_stack_list t_queue_circ t_queue_list
TEST_BINS += t_expr_array t_expr_list
TEST_BINS += t_deque_circ t_deque_dll
TEST_BINS += t_sliding_circ t_sliding_dll
BENCH_BINS = bench_array bench_list

.PHONY: all test test-array test-list test-expr test-deque test-sliding-max bench clean sanitizer-check

all: sanitizer-check $(TEST_BINS) $(BENCH_BINS)

ifeq ($(SAN),1)
sanitizer-check:
	@echo "Verification de la disponibilite de ASan/UBSan..."
	@printf '%s\n' 'int main(void) { return 0; }' > .san_probe.c
	@if $(CC) $(CFLAGS) .san_probe.c -o .san_probe; then \
		rm -f .san_probe.c .san_probe .san_probe.exe; \
	else \
		rm -f .san_probe.c .san_probe .san_probe.exe; \
		echo "ERREUR : ASan/UBSan indisponibles dans cet environnement."; \
		echo "Relancez sans SAN=1, par exemple :"; \
		echo "  make -f Makefile.tp5 test"; \
		exit 1; \
	fi
else
sanitizer-check:
	@:
endif

t_stack_array: test_stack.c stack_array.c stack.h ds_status.h
	$(CC) $(CFLAGS) test_stack.c stack_array.c -o $@

t_stack_list: test_stack.c stack_list.c stack.h ds_status.h
	$(CC) $(CFLAGS) test_stack.c stack_list.c -o $@

t_queue_circ: test_queue.c queue_circ.c queue.h ds_status.h
	$(CC) $(CFLAGS) test_queue.c queue_circ.c -o $@

t_queue_list: test_queue.c queue_list.c queue.h ds_status.h
	$(CC) $(CFLAGS) test_queue.c queue_list.c -o $@

t_expr_array: test_expr.c expr.c stack_array.c queue_circ.c expr.h stack.h queue.h ds_status.h
	$(CC) $(CFLAGS) test_expr.c expr.c stack_array.c queue_circ.c -o $@

t_expr_list: test_expr.c expr.c stack_list.c queue_list.c expr.h stack.h queue.h ds_status.h
	$(CC) $(CFLAGS) test_expr.c expr.c stack_list.c queue_list.c -o $@

t_deque_circ: test_deque.c deque_circ.c deque.h ds_status.h
	$(CC) $(CFLAGS) test_deque.c deque_circ.c -o $@

t_deque_dll: test_deque.c deque_dll.c deque.h ds_status.h
	$(CC) $(CFLAGS) test_deque.c deque_dll.c -o $@

t_sliding_circ: test_sliding_max.c sliding_max.c deque_circ.c sliding_max.h deque.h ds_status.h
	$(CC) $(CFLAGS) test_sliding_max.c sliding_max.c deque_circ.c -o $@

t_sliding_dll: test_sliding_max.c sliding_max.c deque_dll.c sliding_max.h deque.h ds_status.h
	$(CC) $(CFLAGS) test_sliding_max.c sliding_max.c deque_dll.c -o $@

bench_array: bench.c stack_array.c queue_circ.c stack.h queue.h ds_status.h
	$(CC) $(CFLAGS) -O2 -DIMPL_NAME='"array"' bench.c stack_array.c queue_circ.c -o $@

bench_list: bench.c stack_list.c queue_list.c stack.h queue.h ds_status.h
	$(CC) $(CFLAGS) -O2 -DIMPL_NAME='"list"' bench.c stack_list.c queue_list.c -o $@

test-array: sanitizer-check t_stack_array t_queue_circ
	./t_stack_array
	./t_queue_circ

test-list: sanitizer-check t_stack_list t_queue_list
	./t_stack_list
	./t_queue_list

test-expr: sanitizer-check t_expr_array t_expr_list
	./t_expr_array
	./t_expr_list

test-deque: sanitizer-check t_deque_circ t_deque_dll
	./t_deque_circ
	./t_deque_dll

test-sliding-max: sanitizer-check t_sliding_circ t_sliding_dll
	./t_sliding_circ
	./t_sliding_dll

test: test-array test-list test-expr test-deque test-sliding-max

bench: sanitizer-check bench_array bench_list
	./bench_array
	./bench_list

clean:
	rm -f $(TEST_BINS) $(BENCH_BINS) .san_probe*
