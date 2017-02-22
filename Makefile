PROGRAM				= earphone_extender
EXTRA_COMPONENTS	= extras/rda5807m extras/i2c extras/httpd extras/mbedtls
PROGRAM_SRC_DIR		= . ./common ./main ./cmdsvr
EXTRA_CFLAGS		= -DLWIP_HTTPD_CGI=1 -DLWIP_HTTPD_SSI=1 -I./fsdata
#EXTRA_CFLAGS		+=-DLWIP_DEBUG=1 -DHTTPD_DEBUG=LWIP_DBG_ON

ESPPORT				= /dev/ttyUSB0
ESBAUD				= 115200

include ../esp-open-rtos/common.mk

html:
	@echo "Generating fsdata.."
	cd fsdata && ./makefsdata
