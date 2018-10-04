CFLAGS += -g -I../src/ -I.

all_tests : test_array test_eval test_calc_state

%.o : ../src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

test_eval test_calc_state : LDLIBS += -lm

test_array : array.o test_ut.o

test_eval : eval.o test_ut.o array.o

test_calc_state : eval.o array.o test_ut.o calc_state.o ftoa.o

clean :
	rm -f *.o test_array test_eval test_calc_state
