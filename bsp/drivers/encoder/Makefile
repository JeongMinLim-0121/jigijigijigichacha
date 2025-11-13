# ===== Driver-local Makefile (per module) =====
# 위치: bsp/drivers/encoder/Makefile

# 모듈 이름(= 소스 encoder_drv.c -> encoder_drv.ko)
MOD := encoder_drv
obj-m := $(MOD).o

# 커널 소스(헤더) 경로
# - 네이티브 빌드: /lib/modules/$(uname -r)/build
# - 크로스 빌드: 라즈베리/임베디드 커널 빌드 디렉토리로 교체
KDIR ?= /lib/modules/$(shell uname -r)/build

# 현재 디렉토리
PWD  := $(shell pwd)

# 크로스 컴파일러 (네이티브면 비워두거나 주석 처리해도 OK)
# 예) ARMhf: arm-linux-gnueabihf- / AArch64: aarch64-linux-gnu-
ARCH ?= $(shell uname -m | sed 's/x86_64/x86/;s/aarch64/arm64/')
CROSS_COMPILE ?=

# 선택: 디버그 매크로
EXTRA_CFLAGS += -DDEBUG

.PHONY: all clean load unload reload install help

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# 편의 타겟들 (타겟 머신에서 실행 가정)
load:
	sudo insmod $(MOD).ko || sudo modprobe $(MOD) || true
	dmesg | tail -n 20

unload:
	- sudo rmmod $(MOD) || true
	dmesg | tail -n 20

reload: unload all load

# 결과 ko를 NFS 등 공유 폴더에 복사하고 싶을 때
install:
	@mkdir -p /srv/nfs || true
	sudo cp -v $(MOD).ko /srv/nfs/

help:
	@echo "== $(MOD) =="
	@echo "make                # build"
	@echo "make clean          # clean"
	@echo "make load           # insmod & dmesg tail"
	@echo "make unload         # rmmod & dmesg tail"
	@echo "make reload         # rebuild & reload"
	@echo "make install        # copy ko to /srv/nfs"
	@echo ""
	@echo "Cross build example:"
	@echo "  make KDIR=/path/to/target/kernel/build ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-"
