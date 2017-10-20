ROOT = /home/jay/work/31452/rootfs
CFLAGS=-march=armv7-a -mfloat-abi=hard -mfpu=neon -mtune=cortex-a9 --sysroot=${ROOT}

TARGET = camera

$(TARGET): %:%.c
	${CROSS_COMPILE}gcc -o $(TARGET) *.c ${CFLAGS}
clean:
	rm -f $(TARGET)

