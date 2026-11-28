ifdef CROSS_COMPILE

$(info ************  CROSS COMPILING ************)
CC=$(CROSS_COMPILE)gcc

endif

all:
	$(CC)  -Wall -Wextra -Werror -o aesdsocket aesdsocket.c

clean:
	rm -f aesdsocket
