#-----------------------------------------------------------------------------------------------------------------------
# Environment variables
#-----------------------------------------------------------------------------------------------------------------------
#NDKPATH=/mnt/hd3/build/nexus5/source/prebuilts/ndk/9/platforms/android-9/
#NDKPATH=${HOME}/android-8/
LIBDIR=$(NDKPATH)arch-arm/usr/lib
INCDIR=$(NDKPATH)arch-arm/usr/include

AS = $(CROSS_COMPILE)as
CC = $(CROSS_COMPILE)gcc 
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)ld
NM = $(CROSS_COMPILE)nm
RANLIB = $(CROSS_COMPILE)ran
STRIP = $(CROSS_COMPILE)strip

INCLUDE = -I. -I../include/ -I$(INCDIR)

CFLAGS = -Wall -fno-short-enums -g
LDFLAGS = -nostdlib -Wl,--dynamic-linker,"/system/bin/linker" -L$(LIBDIR) -lc -lm -lstdc++ -ldl -lgcc -fpie -fpic -fPIE -pie 

CRTBEGIN = $(LIBDIR)/crtbegin_dynamic.o
CRTEND = $(LIBDIR)/crtend_android.o

TARGET = sprofiler
SRCS = sprofiler.c spr_parse.c server/server.c
SRCS_HOST = host.c spr_parse.c
OBJS = $(SRCS:.c=.o)


#-----------------------------------------------------------------------------------------------------------------------
# Define the rule
#-----------------------------------------------------------------------------------------------------------------------
.SUFFIXES:.c .o 

.c.o:
	@echo Compiling: $< 
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<

all: app

app:
	@echo Build sprofiler
	$(CC) -o $(TARGET) $(SRCS) $(CFLAGS) $(INCLUDE) $(LDFLAGS) $(CRTBEGIN) $(CRTEND)

install :
	cp sprofiler tmpp
	adb push tmpp /sdcard/
	adb shell 'su -c mv /sdcard/tmpp /data/local'
	adb shell 'su -c mv /data/local/tmpp /data/local/sprofiler'
	adb shell 'su -c chmod 777 /data/local/sprofiler'
	adb push dump.sh /sdcard/
	adb shell 'su -c mv /sdcard/dump.sh /data/local'
	adb shell 'su -c chmod 777 /data/local/dump.sh'

#	adb shell 'su -c /data/local/sprofiler'
install2 :
	adb push dump.sh /sdcard/
	adb shell 'su -c mv /sdcard/dump.sh /data/local'
	adb shell 'su -c chmod 777 /data/local/dump.sh'

run : $(install)
	adb shell "/data/local/sprofiler"

clean:
	rm -f *~
	rm -f *.o
	rm -f $(TARGET)

host:
	gcc -C $(SRCS_HOST) -o host.out -Wall -ggdb -DHOST -g -pthread -lm
