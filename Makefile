CONFIG_TARGET_DIR := i386/whitex
CONFIG_TARGET_ARCH := i386
CONFIG_COMPRESS_ALGO := bz2
CONFIG_KERNEL_BIN_NAME := kernelll
CONFIG_BUILD_VERBOSITY := 0

ROOT_WORKSPACE := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
OUTPUT_IMAGES_DIR := $(ROOT_WORKSPACE)/Images
STAGING_DIR := $(ROOT_WORKSPACE)/.staging_build

CC := clang
CXX := clang++
AS := nasm
LD := ld.lld
OBJCOPY := llvm-objcopy
OBJDUMP := llvm-objdump
READELF := llvm-readelf
STRIP := llvm-strip
AR := llvm-ar
NM := llvm-nm
ASFLAGS := -f elf32
COMPRESS_BIN_gzip := gzip
COMPRESS_ARGS_gzip := -9 -c
COMPRESS_EXT_gzip := .gz

COMPRESS_BIN_bz2 := bzip2
COMPRESS_ARGS_bz2 := -9 -c
COMPRESS_EXT_bz2 := .bz2

COMPRESS_BIN_lzma := lzma
COMPRESS_ARGS_lzma := -9 -c
COMPRESS_EXT_lzma := .lzma

ARCH_TRIPLE_i386 := i686-pc-none-elf
ARCH_TRIPLE_x86_64 := x86_64-pc-none-elf
ARCH_TRIPLE_arm := armv7a-none-eabi
ARCH_TRIPLE_aarch64 := aarch64-none-elf
ARCH_TRIPLE_xtensa := xtensa-esp32-elf
ARCH_TRIPLE_mips := mips-none-elf
ARCH_TRIPLE_mips64 := mips64-none-elf
ARCH_TRIPLE_riscv32 := riscv32-none-elf
ARCH_TRIPLE_riscv64 := riscv64-none-elf
ARCH_TRIPLE_powerpc := powerpc-none-elf
ARCH_TRIPLE_sparc := sparc-none-elf

ARCH_CFLAGS_i386 := -m32 -march=i686
ARCH_CFLAGS_x86_64 := -m64 -march=x86-64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2
ARCH_CFLAGS_arm := -target $(ARCH_TRIPLE_arm) -march=armv7-a -mfloat-abi=soft
ARCH_CFLAGS_aarch64 := -target $(ARCH_TRIPLE_aarch64) -march=armv8-a -mgeneral-regs-only
ARCH_CFLAGS_xtensa := -target $(ARCH_TRIPLE_xtensa) -mlongcalls
ARCH_CFLAGS_mips := -target $(ARCH_TRIPLE_mips) -mabi=32
ARCH_CFLAGS_mips64 := -target $(ARCH_TRIPLE_mips64) -mabi=64
ARCH_CFLAGS_riscv32 := -target $(ARCH_TRIPLE_riscv32) -march=rv32ima -mabi=ilp32
ARCH_CFLAGS_riscv64 := -target $(ARCH_TRIPLE_riscv64) -march=rv64imafdc -mabi=lp64
ARCH_CFLAGS_powerpc := -target $(ARCH_TRIPLE_powerpc)
ARCH_CFLAGS_sparc := -target $(ARCH_TRIPLE_sparc)

BASE_CFLAGS := -ffreestanding -w -Wall
BASE_LDFLAGS := -nostdlib

SELECTED_TRIPLE := $(ARCH_TRIPLE_$(CONFIG_TARGET_ARCH))
SELECTED_ARCH_CFLAGS := $(ARCH_CFLAGS_$(CONFIG_TARGET_ARCH))

ifeq ($(SELECTED_TRIPLE),)
$(error Target architecture $(CONFIG_TARGET_ARCH) is not supported in the matrix)
endif

FINAL_CFLAGS := -target $(SELECTED_TRIPLE) $(SELECTED_ARCH_CFLAGS) $(BASE_CFLAGS)
FINAL_LDFLAGS := $(BASE_LDFLAGS)

export CC
export CXX
export AS
export LD
export OBJCOPY
export OBJDUMP
export FINAL_CFLAGS
export FINAL_LDFLAGS
export CONFIG_TARGET_ARCH

TARGET_SUBMAKE := $(ROOT_WORKSPACE)/$(CONFIG_TARGET_DIR)/Makefile
SOURCE_BINARY := $(ROOT_WORKSPACE)/$(CONFIG_TARGET_DIR)/$(CONFIG_KERNEL_BIN_NAME)
STAGED_BINARY := $(STAGING_DIR)/$(CONFIG_KERNEL_BIN_NAME)
FINAL_COMPRESSED_FILE := $(OUTPUT_IMAGES_DIR)/$(CONFIG_KERNEL_BIN_NAME)-$(CONFIG_TARGET_ARCH)$(COMPRESS_EXT_$(CONFIG_COMPRESS_ALGO))

ifeq ($(CONFIG_BUILD_VERBOSITY),0)
Q := @
MAKE_SILENT := -s
else
Q :=
MAKE_SILENT :=
endif

.PHONY: all pre_flight execute_submake stage_artifacts compress_artifacts cleanup_staging clean deep_clean verify_config build_matrix

all: verify_config pre_flight execute_submake stage_artifacts compress_artifacts cleanup_staging

verify_config:
	$(Q)if [ ! -d "$(ROOT_WORKSPACE)/$(CONFIG_TARGET_DIR)" ]; then \
		echo "FATAL: Configuration directory $(CONFIG_TARGET_DIR) does not exist."; \
		exit 1; \
	fi
	$(Q)if [ ! -f "$(TARGET_SUBMAKE)" ]; then \
		echo "FATAL: Makefile not found in $(CONFIG_TARGET_DIR)."; \
		exit 1; \
	fi
	$(Q)if [ -z "$(COMPRESS_BIN_$(CONFIG_COMPRESS_ALGO))" ]; then \
		echo "FATAL: Compression algorithm $(CONFIG_COMPRESS_ALGO) is invalid."; \
		exit 1; \
	fi

pre_flight:
	$(Q)mkdir -p $(OUTPUT_IMAGES_DIR)
	$(Q)mkdir -p $(STAGING_DIR)

execute_submake:
	$(Q)$(MAKE) $(MAKE_SILENT) -C $(ROOT_WORKSPACE)/$(CONFIG_TARGET_DIR) all

stage_artifacts: execute_submake
	$(Q)if [ ! -f "$(SOURCE_BINARY)" ]; then \
		echo "FATAL: Kernel binary $(CONFIG_KERNEL_BIN_NAME) was not generated in $(CONFIG_TARGET_DIR)."; \
		exit 1; \
	fi
	$(Q)cp $(SOURCE_BINARY) $(STAGED_BINARY)
	$(Q)$(STRIP) $(STAGED_BINARY) || true

compress_artifacts: stage_artifacts
	$(Q)echo "Compressing artifact using $(CONFIG_COMPRESS_ALGO)..."
	$(Q)$(COMPRESS_BIN_$(CONFIG_COMPRESS_ALGO)) $(COMPRESS_ARGS_$(CONFIG_COMPRESS_ALGO)) $(STAGED_BINARY) > $(FINAL_COMPRESSED_FILE)
	$(Q)echo "Build successful. Final image located at: $(FINAL_COMPRESSED_FILE)"

cleanup_staging: compress_artifacts
	$(Q)rm -rf $(STAGING_DIR)

clean:
	$(Q)rm -rf $(OUTPUT_IMAGES_DIR)
	$(Q)rm -rf $(STAGING_DIR)
	$(Q)if [ -f "$(TARGET_SUBMAKE)" ]; then \
		$(MAKE) $(MAKE_SILENT) -C $(ROOT_WORKSPACE)/$(CONFIG_TARGET_DIR) clean; \
	fi

deep_clean: clean
	$(Q)find $(ROOT_WORKSPACE) -name "*.o" -type f -delete
	$(Q)find $(ROOT_WORKSPACE) -name "*.log" -type f -delete
	$(Q)find $(ROOT_WORKSPACE) -name "*.tmp" -type f -delete

build_matrix: pre_flight
	$(Q)$(MAKE) all CONFIG_TARGET_ARCH=i386 CONFIG_COMPRESS_ALGO=gzip
	$(Q)$(MAKE) all CONFIG_TARGET_ARCH=x86_64 CONFIG_COMPRESS_ALGO=lzma
	$(Q)$(MAKE) all CONFIG_TARGET_ARCH=arm CONFIG_COMPRESS_ALGO=bz2
	$(Q)$(MAKE) all CONFIG_TARGET_ARCH=aarch64 CONFIG_COMPRESS_ALGO=lzma
	$(Q)$(MAKE) all CONFIG_TARGET_ARCH=xtensa CONFIG_COMPRESS_ALGO=gzip

define GENERATE_ARCH_TARGET
build_$(1):
	$$(Q)$$(MAKE) all CONFIG_TARGET_ARCH=$(1)
endef

$(eval $(call GENERATE_ARCH_TARGET,i386))
$(eval $(call GENERATE_ARCH_TARGET,x86_64))
$(eval $(call GENERATE_ARCH_TARGET,arm))
$(eval $(call GENERATE_ARCH_TARGET,aarch64))
$(eval $(call GENERATE_ARCH_TARGET,xtensa))
$(eval $(call GENERATE_ARCH_TARGET,mips))
$(eval $(call GENERATE_ARCH_TARGET,mips64))
$(eval $(call GENERATE_ARCH_TARGET,riscv32))
$(eval $(call GENERATE_ARCH_TARGET,riscv64))
$(eval $(call GENERATE_ARCH_TARGET,powerpc))
$(eval $(call GENERATE_ARCH_TARGET,sparc))

define GENERATE_COMPRESS_TARGET
pack_$(1):
	$$(Q)$$(MAKE) all CONFIG_COMPRESS_ALGO=$(1)
endef

$(eval $(call GENERATE_COMPRESS_TARGET,gzip))
$(eval $(call GENERATE_COMPRESS_TARGET,lzma))
$(eval $(call GENERATE_COMPRESS_TARGET,bz2))

dump_config:
	$(Q)echo "================ CONFIGURATION DUMP ================"
	$(Q)echo "CONFIG_TARGET_DIR    : $(CONFIG_TARGET_DIR)"
	$(Q)echo "CONFIG_TARGET_ARCH   : $(CONFIG_TARGET_ARCH)"
	$(Q)echo "CONFIG_COMPRESS_ALGO : $(CONFIG_COMPRESS_ALGO)"
	$(Q)echo "CONFIG_KERNEL_BIN    : $(CONFIG_KERNEL_BIN_NAME)"
	$(Q)echo "COMPILER             : $(CC)"
	$(Q)echo "ARCH_TRIPLE          : $(SELECTED_TRIPLE)"
	$(Q)echo "FINAL_CFLAGS         : $(FINAL_CFLAGS)"
	$(Q)echo "===================================================="

test_environment:
	$(Q)$(CC) --version > /dev/null 2>&1 || (echo "Clang not found" && exit 1)
	$(Q)$(OBJCOPY) --version > /dev/null 2>&1 || (echo "llvm-objcopy not found" && exit 1)
	$(Q)gzip -V > /dev/null 2>&1 || (echo "gzip not found" && exit 1)
	$(Q)lzma -V > /dev/null 2>&1 || (echo "lzma not found" && exit 1)
	$(Q)bzip2 -V > /dev/null 2>&1 || (echo "bzip2 not found" && exit 1)
	$(Q)echo "Environment verification passed."
