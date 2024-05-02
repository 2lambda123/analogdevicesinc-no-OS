
ifeq (y,$(strip $(BASIC_EXAMPLE)))
CFLAGS += -DBASIC_EXAMPLE=1
SRCS += $(PROJECT)/src/examples/basic_example/basic_example.c
INCS += $(PROJECT)/src/examples/basic_example/basic_example.h
endif

ifeq (xilinx,$(PLATFORM))
CFLAGS += -DSPI_ENGINE_OFFLOAD_EXAMPLE
endif
ifeq (stm32,$(PLATFORM))
CFLAGS += -DUSE_STANDARD_SPI
endif

ifeq (y,$(strip $(IIO_EXAMPLE)))
CFLAGS += -DIIO_EXAMPLE=1 -DIIO_SUPPORT
LIBRARIES += iio

SRCS += $(PROJECT)/src/examples/iio_example/iio_example.c
INCS += $(PROJECT)/src/examples/iio_example/iio_example.h

INCS += $(DRIVERS)/adc/ad400x/iio_ad400x.h
SRCS += $(DRIVERS)/adc/ad400x/iio_ad400x.c

INCS +=	$(NO-OS)/iio/iio.h \
	$(NO-OS)/iio/iiod.h \
	$(NO-OS)/iio/iiod_private.h \
	$(NO-OS)/iio/iio_types.h \
	$(NO-OS)/iio/iio_app/iio_app.h

SRCS +=  $(NO-OS)/iio/iio.c \
	$(NO-OS)/iio/iiod.c \
	$(NO-OS)/iio/iio_app/iio_app.c
endif

INCS += $(DRIVERS)/adc/ad400x/ad400x.h
SRCS += $(DRIVERS)/adc/ad400x/ad400x.c

INCS += $(INCLUDE)/no_os_list.h \
        $(PLATFORM_DRIVERS)/$(PLATFORM)_uart.h
