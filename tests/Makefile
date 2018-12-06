CFLAGS += -g -I../src/ -I.

all_tests : test_list test_eval test_calc_state

%.o : ../src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

list.o : ../src/list.h
test_ut.o : test_ut.h
eval.o : ../src/list.h ../src/ieee_fp.h ../src/eval.h
calc_state.o : ../src/calc_state.h ../src/eval.h \
    ../src/list.h ../src/ftoa.h
ftoa.o : ../src/ieee_fp.h ../src/ftoa.h
ieee_fp.o : ../src/ieee_fp.h
compat_eval.o : ../src/list.h ../src/eval.h

test_eval test_calc_state : LDLIBS += -lm
test_list : list.o test_ut.o
test_eval : eval.o test_ut.o list.o ieee_fp.o compat_eval.o
test_calc_state : eval.o list.o test_ut.o calc_state.o ftoa.o ieee_fp.o compat_eval.o

clean :
	rm -f *.o test_list test_eval test_calc_state
