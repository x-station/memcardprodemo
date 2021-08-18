TARGET = memcardprodemo

CXXFLAGS = -std=c++17 -fno-exceptions -fno-rtti

SRCS = \
nugget/crt0/crt0.s \
nugget/crt0/mainstub.c \
\
littlelibc.cc \
memcardpro.cc \
memcardprodemo.cc \
spi.cc \
utility.cc \

include ./nugget/common.mk

CPPFLAGS += -Werror

LDFLAGS += -Xlinker --defsym=TLOAD_ADDR=0x80010000
LDFLAGS += -Xlinker --defsym=ROM_MODE=0x00
