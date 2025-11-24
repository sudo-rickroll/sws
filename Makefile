CFLAGS = -ansi -g -Wall -Werror -Wextra -Wformat=2 -Wjump-misses-init -Wlogical-op -Wpedantic -Wshadow
TARGET_NAME = sws
OBJECT_FILES = sws.o logger.o connections.o

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
