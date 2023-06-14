build: libscheduler.so

libscheduler.so: so_scheduler.o
	gcc -Wall -Wextra -shared so_scheduler.o -o libscheduler.so

libscheduler.o: so_scheduler.c util/so_scheduler.h
	gcc -Wall -Wextra -c so_scheduler.c util/so_scheduler.h -o so_scheduler.o

clean:
	rm so_scheduler.o libscheduler.so