PROGRAM					= earphone_extender
EXTRA_COMPONENTS		= extras/i2c extras/httpd extras/mbedtls extras/dhcpserver extras/spiffs
PROGRAM_SRC_DIR			= . ./common ./main ./cmdsvr
PROGRAM_EXTRA_SRC_FILES	=./discovery/discovery.c
EXTRA_CFLAGS			= -DLWIP_HTTPD_CGI=1 -DLWIP_HTTPD_SSI=1 -DLWIP_HTTPD_SSI_INCLUDE_TAG=0 -I./fsdata
EXTRA_CFLAGS			+=-DLWIP_IGMP -DLWIP_RAND=rand
#EXTRA_CFLAGS			+=-DLWIP_DEBUG=1 -DHTTPD_DEBUG=1

FLASH_SIZE				= 32
SPIFFS_BASE_ADDR		= 0x200000
SPIFFS_SIZE				= 0x010000
SPIFFS_SINGLETON		= 1
	
ifdef EARPHONE_END	
EXTRA_COMPONENTS		+=extras/rda5807m
EXTRA_CFLAGS			+=-DEARPHONE_END
else	
EXTRA_COMPONENTS		+=extras/kt0803l
EXTRA_CFLAGS			+=-DSTATION_END
endif	
	
ESPPORT					?= /dev/ttyUSB0
ESBAUD					?= 115200

include ../esp-open-rtos/common.mk

$(eval $(call make_spiffs_image,files))

html:
	@echo "Generating fsdata.."
	cd fsdata && ./makefsdata
