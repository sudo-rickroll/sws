CFLAGS = -g -Wall -Werror -Wextra -Wformat=2 -Wjump-misses-init -Wlogical-op -Wpedantic -Wshadow -std=c11 -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -D_NETBSD_SOURCE -D__EXTENSIONS__
IFLAGS = `uname -s | grep -q SunOS && echo -I/opt/magic/include -I/opt/magic/lib -L/opt/magic/lib -Wl,-rpath,/opt/magic/lib -lsocket`
CFLAGS += ${IFLAGS}
LD_FLAGS = -lmagic
TARGET_NAME = sws
OBJECT_FILES = sws.o logger.o connections.o handlers.o cgi.o util.o

all: ${TARGET_NAME}

depend:
	mkdep -- ${CFLAGS} *.c

${TARGET_NAME}: $(OBJECT_FILES)
	$(CC) $(CFLAGS) ${LD_FLAGS} ${OBJECT_FILES} -o $(TARGET_NAME)

clean:
	rm -- $(TARGET_NAME) ${OBJECT_FILES}

format:
	clang-format -i -- *.c *.h

format-readme:
	fmt README.md > README.fmt
	mv README.fmt README.md
