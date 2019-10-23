#
# XbeeSetup
#

# Needs to modify the XBeeIF.cc.
XBEE_S2B_FILE=82002356_D.zip
XBEE_S2B_COORDINATOR_AT=XB24-ZB_20A7.ebl
XBEE_S2B_COORDINATOR_API=XB24-ZB_21A7.ebl
XBEE_S2B_ROUTER_AT=XB24-ZB_22A7.ebl
XBEE_S2B_ROUTER_API=XB24-ZB_23A7.ebl
XBEE_S2B_ENDPOINT_AT=XB24-ZB_28A7.ebl
XBEE_S2B_ENDPOINT_API=XB24-ZB_29A7.ebl
XBEE_S2C_FILE=XB24-s2c_4060_82004138.zip
XBEE_S2C=xb24c-s2c_4060.ebl
XBEE_S3_FILE=XB3-24Z_1008.zip
XBEE_S3=XB3-24Z_1008.gbl
XBEE_S3_BOOT_FILE=xb3-boot-rf_1.8.1.zip
XBEE_S3_BOOT=xb3-boot-rf_1.8.1.gbl
XBEE_EBL_FILES= \
  ebl_files/${XBEE_S2B_COORDINATOR_AT} \
  ebl_files/${XBEE_S2B_COORDINATOR_API} \
  ebl_files/${XBEE_S2B_ROUTER_AT} \
  ebl_files/${XBEE_S2B_ROUTER_API} \
  ebl_files/${XBEE_S2B_ENDPOINT_AT} \
  ebl_files/${XBEE_S2B_ENDPOINT_API} \
  ebl_files/${XBEE_S2C} \
  ebl_files/${XBEE_S3} \
  ebl_files/${XBEE_S3_BOOT}

target_program = $(shell pwd | sed 's!.*/!!')
cc_srcs = $(sort $(shell ls *.cc 2> /dev/null))
cc_header = $(sort $(shell ls *.h 2> /dev/null))
target_os = $(shell uname)
LIBS =
CC_FLAGS = -I.

ifeq (${target_os},Linux)
  TAR = /usr/bin/bsdtar
endif
ifeq (${target_os},Darwin)
  TAR = /usr/bin/tar
endif
LD_FLAGS = ${LIBS}

all: ${ebl_files} $(target_program)

$(target_program) : $(cc_srcs) $(cc_header)
	g++ $(CC_FLAGS) $(LIBS) -Wno-deprecated-declarations -o $@ $(cc_srcs)

ebl_files: ${XBEE_EBL_FILES}

ebl_files/${XBEE_S2B_FILE}:
	mkdir -p ebl_files
	curl -o $@ http://ftp1.digi.com/support/firmware/${XBEE_S2B_FILE}

ebl_files/${XBEE_S2C_FILE}:
	mkdir -p ebl_files
	curl -o $@ http://ftp1.digi.com/support/firmware/${XBEE_S2C_FILE}

ebl_files/${XBEE_S3_FILE}:
	mkdir -p ebl_files
	curl -o $@ http://ftp1.digi.com/support/firmware/${XBEE_S3_FILE}

ebl_files/${XBEE_S3_BOOT_FILE}:
	mkdir -p ebl_files
	curl -o $@ http://ftp1.digi.com/support/firmware/bootloaders/${XBEE_S3_BOOT_FILE}

ebl_files/${XBEE_S2B_COORDINATOR_AT}: ebl_files/${XBEE_S2B_FILE}
	mkdir -p ebl_files
	${TAR} xvOf $< \*${XBEE_S2B_COORDINATOR_AT} > $@

ebl_files/${XBEE_S2B_COORDINATOR_API}: ebl_files/${XBEE_S2B_FILE}
	mkdir -p ebl_files
	${TAR} xvOf $< \*${XBEE_S2B_COORDINATOR_API} > $@

ebl_files/${XBEE_S2B_ROUTER_AT}: ebl_files/${XBEE_S2B_FILE}
	mkdir -p ebl_files
	${TAR} xvOf $< \*${XBEE_S2B_ROUTER_AT} > $@

ebl_files/${XBEE_S2B_ROUTER_API}: ebl_files/${XBEE_S2B_FILE}
	mkdir -p ebl_files
	${TAR} xvOf $< \*${XBEE_S2B_ROUTER_API} > $@

ebl_files/${XBEE_S2B_ENDPOINT_AT}: ebl_files/${XBEE_S2B_FILE}
	mkdir -p ebl_files
	${TAR} xvOf $< \*${XBEE_S2B_ENDPOINT_AT} > $@

ebl_files/${XBEE_S2B_ENDPOINT_API}: ebl_files/${XBEE_S2B_FILE}
	mkdir -p ebl_files
	${TAR} xvOf $< \*${XBEE_S2B_ENDPOINT_API} > $@

ebl_files/${XBEE_S2C}: ebl_files/${XBEE_S2C_FILE}
	mkdir -p ebl_files
	${TAR} xvOf $< \*${XBEE_S2C} > $@

ebl_files/${XBEE_S3}: ebl_files/${XBEE_S3_FILE}
	mkdir -p ebl_files
	${TAR} xvOf $< \*${XBEE_S3} > $@

ebl_files/${XBEE_S3_BOOT}: ebl_files/${XBEE_S3_BOOT_FILE}
	mkdir -p ebl_files
	${TAR} xvOf $< \*${XBEE_S3_BOOT} > $@

clean:
	rm $(target_program)

install: all
	cp XBeeSetup /usr/local/bin

