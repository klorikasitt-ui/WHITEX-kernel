#!/bin/bash
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
NC='\033[0m'
KERNEL_BIN="kernelll"
LOG_FILE="kernel_debug.log"
LINKER_SCRIPT="linker.ld"
TIMEOUT_SECS=10
QEMU_CMD="qemu-system-i386"
CLANG_CMD="clang"
NASM_CMD="nasm"
LD_CMD="ld"
OBJDUMP_CMD="objdump"
READELF_CMD="readelf"
START_TIME=$(date +%s)
TOTAL_C_FILES=0
COMPILED_C_FILES=0
FAILED_C_FILES=0
BOOTLOADER_STATUS="UNKNOWN"
LINKER_STATUS="UNKNOWN"
QEMU_STATUS="UNKNOWN"
PANIC_DETECTED="FALSE"
OOM_DETECTED="FALSE"

print_header() {
    echo -e "${BLUE}${BOLD}======================================================================${NC}"
    echo -e "${CYAN}${BOLD}           WHITEX KERNEL ADVANCED BUILD SYSTEM           ${NC}"
    echo -e "${BLUE}${BOLD}======================================================================${NC}"
}

print_step() {
    echo -e "\n${MAGENTA}${BOLD}>>> $1${NC}"
}

print_success() {
    echo -e "${GREEN}[SUCCESS] $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}[WARNING] $1${NC}"
}

print_error() {
    echo -e "${RED}[ERROR] $1${NC}"
}

check_environment() {
    print_step "ENVIRONMENT INITIALIZATION & DEPENDENCY CHECK"
    local deps=("$CLANG_CMD" "$NASM_CMD" "$LD_CMD" "$QEMU_CMD" "$OBJDUMP_CMD" "$READELF_CMD")
    for tool in "${deps[@]}"; do
        if command -v "$tool" >/dev/null 2>&1; then
            print_success "Toolchain component found: $tool"
        else
            print_warning "Toolchain component missing: $tool. Execution will proceed but may fail."
        fi
    done
    if [ ! -f "$LINKER_SCRIPT" ]; then
        print_error "Linker script $LINKER_SCRIPT is missing."
    else
        print_success "Linker script $LINKER_SCRIPT located."
    fi
}

build_bootloader() {
    print_step "BOOTLOADER COMPILATION PHASE"
    if [ ! -f "boot.asm" ]; then
        if [ -f "../boot.asm" ]; then
            echo -e "${CYAN}Copying boot.asm from parent directory...${NC}"
            cp ../boot.asm .
        else
            print_error "boot.asm not found in current or parent directory."
            BOOTLOADER_STATUS="FAILED"
            return
        fi
    fi
    $NASM_CMD -f elf32 boot.asm -o boot.o
    if [ $? -eq 0 ]; then
        print_success "Bootloader compiled successfully to ELF32 format."
        BOOTLOADER_STATUS="SUCCESS"
    else
        print_error "Bootloader compilation failed."
        BOOTLOADER_STATUS="FAILED"
    fi
}

build_kernel() {
    print_step "KERNEL C SOURCES COMPILATION PHASE"
    local c_files=(*.c)
    if [ "${#c_files[@]}" -eq 0 ] || [ ! -e "${c_files[0]}" ]; then
        print_error "No C source files found in the current directory."
        return
    fi
    for file in "${c_files[@]}"; do
        TOTAL_C_FILES=$((TOTAL_C_FILES + 1))
        local obj_file="${file%.c}.o"
        local temp_log="conpile${file}.log"
        echo -e "${CYAN}Compiling module:${NC} $file -> $obj_file"
        $CLANG_CMD --target=i686-elf -m32 -ffreestanding -Wall  -c "$file" -o "$obj_file" > "$temp_log" 2>&1
        if [ $? -eq 0 ]; then
            print_success "Module $file compiled without fatal errors."
            COMPILED_C_FILES=$((COMPILED_C_FILES + 1))
        else
            print_error "Module $file failed to compile."
            cat "$temp_log"
            FAILED_C_FILES=$((FAILED_C_FILES + 1))
        fi
    done
    echo -e "${CYAN}Compilation Summary: $COMPILED_C_FILES/$TOTAL_C_FILES successful.${NC}"
}

link_kernel() {
    print_step "KERNEL LINKING PHASE"
    local obj_files=(*.o)
    if [ "${#obj_files[@]}" -eq 0 ] || [ ! -e "${obj_files[0]}" ]; then
        print_error "No object files available for linking."
        LINKER_STATUS="FAILED"
        return
    fi
    echo -e "${CYAN}Executing linker with target script: $LINKER_SCRIPT${NC}"
    $LD_CMD -m elf_i386 -T "$LINKER_SCRIPT" -o "$KERNEL_BIN" "${obj_files[@]}" > linker_output.log 2>&1
    if [ $? -eq 0 ] && [ -f "$KERNEL_BIN" ]; then
        print_success "Kernel successfully linked into binary: $KERNEL_BIN"
        LINKER_STATUS="SUCCESS"
        local ksize=$(stat -c%s "$KERNEL_BIN" 2>/dev/null || stat -f%z "$KERNEL_BIN" 2>/dev/null || echo "Unknown")
        echo -e "${CYAN}Generated kernel size: $ksize bytes${NC}"
    else
        print_error "Linker phase failed. Check linker_output.log for details."
        cat linker_output.log
        LINKER_STATUS="FAILED"
    fi
}

analyze_binary() {
    print_step "POST-BUILD BINARY ANALYSIS"
    if [ "$LINKER_STATUS" = "SUCCESS" ] && [ -f "$KERNEL_BIN" ]; then
        if command -v "$READELF_CMD" >/dev/null 2>&1; then
            echo -e "${CYAN}Extracting ELF headers...${NC}"
            $READELF_CMD -h "$KERNEL_BIN" | grep -E "Class|Machine|Entry"
        fi
        if command -v "$OBJDUMP_CMD" >/dev/null 2>&1; then
            echo -e "${CYAN}Extracting section structure...${NC}"
            $OBJDUMP_CMD -h "$KERNEL_BIN" | awk 'NR>5 {print $1, $2, $3, $4}' | head -n 10
        fi
    else
        print_warning "Skipping binary analysis due to linker failure."
    fi
}

run_simulation() {
    print_step "HARDWARE SIMULATION (QEMU) PHASE"
    if [ ! -f "$KERNEL_BIN" ]; then
        print_error "Kernel binary missing. Attempting to run simulation with empty state..."
    fi
    > "$LOG_FILE"
    echo -e "${CYAN}Initializing QEMU i386 emulation engine...${NC}"
    echo -e "${CYAN}Allocating 64MB RAM, attaching debug console to I/O port 0xe9...${NC}"
    $QEMU_CMD -kernel "$KERNEL_BIN" -display none -vga std -m 64M -debugcon file:"$LOG_FILE" -global isa-debugcon.iobase=0xe9 -serial stdio > /dev/null 2>&1 &
    local sim_pid=$!
    echo -e "${CYAN}Simulation PID: $sim_pid. Initiating telemetry monitor for $TIMEOUT_SECS seconds...${NC}"
    local frames=("-" "\\" "|" "/")
    local f_idx=0
    for (( i=1; i<=TIMEOUT_SECS; i++ )); do
        f_idx=$(( (f_idx+1) % 4 ))
        printf "\r${YELLOW}[${frames[$f_idx]}] System running... Elapsed: %d/%d sec${NC}" "$i" "$TIMEOUT_SECS"
        sleep 1
    done
    printf "\n"
    if kill -0 $sim_pid 2>/dev/null; then
        echo -e "${CYAN}Simulation timeframe elapsed. Terminating QEMU process...${NC}"
        kill -9 $sim_pid 2>/dev/null
        QEMU_STATUS="COMPLETED"
    else
        print_warning "QEMU process terminated prematurely. Checking for fatal aborts."
        QEMU_STATUS="ABORTED_PREMATURELY"
    fi
}

analyze_telemetry() {
    print_step "TELEMETRY & LOG ANALYSIS PHASE"
    if [ ! -s "$LOG_FILE" ]; then
        print_warning "Debug console output is empty. Kernel may not have booted or I/O port 0xe9 is uninitialized."
        return
    fi
    echo -e "${CYAN}Scanning log signatures for kernel panics and fault states...${NC}"
    if grep -qE "KERNEL PANIC|CRITICAL FAULT|SYSTEM HALTED|Division by zero" "$LOG_FILE"; then
        PANIC_DETECTED="TRUE"
        print_error "KERNEL PANIC SIGNATURE DETECTED!"
        grep -E --color=always "KERNEL PANIC|CRITICAL FAULT|SYSTEM HALTED|Division by zero" "$LOG_FILE"
    fi
    if grep -qE "OOM_ALLOC_FAILED_COMPLETELY|OOM Kills|Not available RAM" "$LOG_FILE"; then
        OOM_DETECTED="TRUE"
        print_error "OUT OF MEMORY (OOM) SIGNATURE DETECTED!"
        grep -E --color=always "OOM_ALLOC_FAILED_COMPLETELY|OOM Kills|Not available RAM" "$LOG_FILE"
    fi
    echo -e "${CYAN}Scanning log signatures for successful initialization events...${NC}"
    if grep -qE "FS Ready|ATA Ready|Welcome to WhiteX" "$LOG_FILE"; then
        print_success "Valid initialization signatures found:"
        grep -E --color=always "FS Ready|ATA Ready|Welcome to WhiteX" "$LOG_FILE" | head -n 5
    fi
}

generate_report() {
    print_step "AUTOMATED DEPLOYMENT REPORT"
    local END_TIME=$(date +%s)
    local DURATION=$((END_TIME - START_TIME))
    echo -e "${BOLD}Execution Duration:${NC} $DURATION seconds"
    echo -e "${BOLD}Bootloader Assembly:${NC} $BOOTLOADER_STATUS"
    echo -e "${BOLD}C Modules Compiled:${NC} $COMPILED_C_FILES / $TOTAL_C_FILES"
    echo -e "${BOLD}Linker State:${NC} $LINKER_STATUS"
    echo -e "${BOLD}Simulation Engine:${NC} $QEMU_STATUS"
    echo -e "${BOLD}System Stability (Panic):${NC} $PANIC_DETECTED"
    echo -e "${BOLD}Memory Stability (OOM):${NC} $OOM_DETECTED"
    echo -e "${BLUE}${BOLD}======================================================================${NC}"
    if [ "$PANIC_DETECTED" = "TRUE" ] || [ "$OOM_DETECTED" = "TRUE" ] || [ "$LINKER_STATUS" = "FAILED" ]; then
        echo -e "${RED}${BOLD}STATUS: VALIDATION FAILED. SYSTEM CONTAINS CRITICAL DEFECTS.${NC}"
    else
        echo -e "${GREEN}${BOLD}STATUS: VALIDATION PASSED. SYSTEM APPEARS STABLE UNDER TEST CONDITIONS.${NC}"
    fi
    echo -e "${BLUE}${BOLD}======================================================================${NC}"
}

print_header
check_environment
build_bootloader
build_kernel
link_kernel
analyze_binary
run_simulation
analyze_telemetry
generate_report
