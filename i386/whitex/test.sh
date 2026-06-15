#!/bin/bash
set -euo pipefail

COLOR_RESET='\033[0m'
COLOR_RED='\033[0;31m'
COLOR_GREEN='\033[0;32m'
COLOR_YELLOW='\033[0;33m'
COLOR_BLUE='\033[0;34m'
COLOR_MAGENTA='\033[0;35m'
COLOR_CYAN='\033[0;36m'
COLOR_BOLD='\033[1m'
COLOR_DIM='\033[2m'

PROJECT_IDENTIFIER="WhiteX_Kernel"
PIPELINE_REVISION="3.0.1_Auto"
WORKSPACE_ROOT=$(pwd)
OUT_DIR="${WORKSPACE_ROOT}/out"
ARTIFACT_DIR="${WORKSPACE_ROOT}/artifacts"
LOG_DIR="${WORKSPACE_ROOT}/logs"
MAIN_LOG="${LOG_DIR}/master_execution.log"
TELEMETRY_LOG="${LOG_DIR}/hypervisor_telemetry.log"
METRICS_FILE="${ARTIFACT_DIR}/deployment_metrics.json"
LINKER_SCRIPT="${WORKSPACE_ROOT}/linker.ld"
KERNEL_BIN="${WORKSPACE_ROOT}/kernelll"

COMPILER_CMD="clang"
ASSEMBLER_CMD="nasm"
LINKER_CMD="ld"
EMULATOR_CMD="qemu-system-i386"
BINARY_DUMP_CMD="objdump"
ELF_READER_CMD="readelf"
CHECKSUM_CMD="sha256sum"

COMPILER_FLAGS=(--target=i686-elf -m32 -ffreestanding -w -Wall)
ASSEMBLER_FLAGS=(-f elf32)
LINKER_FLAGS=(-m elf_i386 -T "${LINKER_SCRIPT}")
EMULATOR_FLAGS=(-display none -vga std -m 256M -serial stdio -global isa-debugcon.iobase=0xe9 -no-reboot -no-shutdown -d int,cpu_reset,guest_errors)

EMULATION_TIMEOUT_SECONDS=25
START_TIMESTAMP=$(date +%s)
END_TIMESTAMP=0

TOTAL_MODULES=0
COMPILED_MODULES=0
FAILED_MODULES=0
ASSEMBLY_STATUS="PENDING"
COMPILATION_STATUS="PENDING"
LINKAGE_STATUS="PENDING"
EMULATION_STATUS="PENDING"
PANIC_FLAG="FALSE"
OOM_FLAG="FALSE"
EMULATOR_PID=""
TOTAL_LINE=$(wc -l *.c *.h *.asm *.ld | tail -n 1 | awk '{print $1}')



terminate_pipeline() {
    exit_code=$1
    if [ -n "${EMULATOR_PID}" ]; then
        kill -9 "${EMULATOR_PID}" 2>/dev/null || true
    fi
    if [ "${exit_code}" -ne 0 ]; then
        echo -e "${COLOR_RED}${COLOR_BOLD}[CRITICAL FAULT] PIPELINE TERMINATION SIGNAL RECEIVED.${COLOR_RESET}"
        exit 1
    else
        echo -e "${COLOR_GREEN}${COLOR_BOLD}[SUCCESS] PIPELINE EXECUTION CONCLUDED.${COLOR_RESET}"
        exit 0
    fi
}

handle_exception() {
    local line_num=$1
    local cmd=$2
    echo -e "${COLOR_RED}${COLOR_BOLD}[EXCEPTION] Instruction failed at line ${line_num}${COLOR_RESET}"
    echo -e "${COLOR_RED}Faulting sequence: ${cmd}${COLOR_RESET}"
    emit_log "FATAL" "Instruction fault at line ${line_num}: ${cmd}"
    terminate_pipeline 1
}

trap 'handle_exception $LINENO "$BASH_COMMAND"' ERR
trap 'terminate_pipeline 1' SIGINT SIGTERM

emit_log() {
    local severity=$1
    local message=$2
    local timestamp
    timestamp=$(date +"%Y-%m-%dT%H:%M:%S%z")
    mkdir -p "${LOG_DIR}"
    echo "[${timestamp}] [${severity}] ${message}" >> "${MAIN_LOG}" || true
}

display_info() {
    echo -e "${COLOR_CYAN}[INFO] $1${COLOR_RESET}"
    emit_log "INFO" "$1"
}

display_success() {
    echo -e "${COLOR_GREEN}${COLOR_BOLD}[SUCCESS] $1${COLOR_RESET}"
    emit_log "SUCCESS" "$1"
}

display_warning() {
    echo -e "${COLOR_YELLOW}${COLOR_BOLD}[WARNING] $1${COLOR_RESET}"
    emit_log "WARN" "$1"
}

display_error() {
    echo -e "${COLOR_RED}${COLOR_BOLD}[ERROR] $1${COLOR_RESET}"
    emit_log "ERROR" "$1"
}

initialize_header() {
    clear
    echo -e "${COLOR_BLUE}${COLOR_BOLD}========================================================================================${COLOR_RESET}"
    echo -e "${COLOR_CYAN}${COLOR_BOLD}                        WHITEX KERNEL TEST                ${COLOR_RESET}"
    echo -e "${COLOR_BLUE}${COLOR_BOLD}========================================================================================${COLOR_RESET}"
    echo -e "${COLOR_DIM}Architecture: i386 | Execution Mode: Unattended Strict | Isolation: Active${COLOR_RESET}\n"
    emit_log "AUDIT" "Pipeline boot sequence initiated."
}

trigger_phase() {
    echo -e "\n${COLOR_MAGENTA}${COLOR_BOLD}>>> [EXECUTION PHASE] $1 <<<${COLOR_RESET}"
    emit_log "PHASE" "$1"
}

provision_environment() {
    trigger_phase "ENVIRONMENT PROVISIONING"
    mkdir -p "${OUT_DIR}" "${LOG_DIR}" "${ARTIFACT_DIR}"
    touch "${MAIN_LOG}" "${TELEMETRY_LOG}"
    if [ -d "${OUT_DIR}" ]; then
        display_info "Deallocating previous output matrix: ${OUT_DIR}"
        rm -rf "${OUT_DIR}"/* || true
    fi
    display_success "Environment isolation matrix successfully established."
}

verify_dependencies() {
    trigger_phase "TOOLCHAIN DEPENDENCY VERIFICATION"
    local required_binaries=("${COMPILER_CMD}" "${ASSEMBLER_CMD}" "${LINKER_CMD}" "${EMULATOR_CMD}" "${BINARY_DUMP_CMD}" "${ELF_READER_CMD}" "${CHECKSUM_CMD}")
    local missing_count=0
    for binary in "${required_binaries[@]}"; do
     if command -v "${binary}" >/dev/null 2>&1; then
            local bin_path
            bin_path=$(command -v "${binary}")
            display_info "Dependency verified: ${binary} -> ${bin_path}"
        else
            display_error "Critical dependency missing: ${binary}"
            missing_count=$((missing_count + 1))
        fi
    done
    if [ ${missing_count} -gt 0 ]; then
        display_error "Dependency verification failed. Missing constraints: ${missing_count}"
        terminate_pipeline 1
    fi
    if [ ! -f "${LINKER_SCRIPT}" ]; then
        display_error "Linker topology script missing: ${LINKER_SCRIPT}"
        terminate_pipeline 1
    fi
    local compiler_version
    compiler_version=$(${COMPILER_CMD} --version | head -n 1)
    emit_log "METRIC" "Compiler Layer: ${compiler_version}"
    display_success "Toolchain dependency layer secured."
}
translate_assembly_layer() {
    trigger_phase "ASSEMBLY LAYER TRANSLATION"
    
    
    local asm_sources=("$WORKSPACE_ROOT"/*.asm)
    
   
    if [ ! -e "${asm_sources[0]}" ]; then
        display_error "No assembly source files detected in the workspace root."
        ASSEMBLY_STATUS="FAILED_MISSING_SOURCE"
        terminate_pipeline 1
    fi
    
    for asm_source in "${asm_sources[@]}"; do
        local base_name
        base_name=$(basename "$asm_source")
        local object_file="${OUT_DIR}/${base_name%.asm}.o"
        
        display_info "Invoking assembler layer on ${base_name}"
        
       
        if ${ASSEMBLER_CMD} "${ASSEMBLER_FLAGS[@]}" "$asm_source" -o "$object_file" >> "${MAIN_LOG}" 2>&1; then
            display_success "Assembly layer translated. Artifact registered: ${object_file}"
        else
            display_error "Assembler layer syntax or translation exception in ${base_name}."
            ASSEMBLY_STATUS="FAILED"
            terminate_pipeline 1
        fi
    done
    
    ASSEMBLY_STATUS="SUCCESS"
}



translate_c_layer() {
    trigger_phase "C-MODULE SUBSYSTEM TRANSLATION"
    local c_sources=("${WORKSPACE_ROOT}"/*.c)
    if [ "${#c_sources[@]}" -eq 0 ] || [ ! -e "${c_sources[0]}" ]; then
        display_warning "Zero C source entities detected in the matrix."
        COMPILATION_STATUS="SKIPPED"
        return
    fi
    TOTAL_MODULES=${#c_sources[@]}
    display_info "Subsystem discovery complete. Registered entities: ${TOTAL_MODULES}"
    for source_file in "${c_sources[@]}"; do
        local base_name
        base_name=$(basename "${source_file}")
        local object_file="${OUT_DIR}/${base_name%.c}.o"
        local translation_log="${LOG_DIR}/compile_${base_name%.c}.log"
        printf "${COLOR_CYAN}[TRANSLATING]${COLOR_RESET} %-25s -> %s\n" "${base_name}" "${object_file}"
        if ${COMPILER_CMD} "${COMPILER_FLAGS[@]}" -c "${source_file}" -o "${object_file}" > "${translation_log}" 2>&1; then
            COMPILED_MODULES=$((COMPILED_MODULES + 1))
        else
            display_error "Translation exception in entity: ${base_name}"
            cat "${translation_log}"
            FAILED_MODULES=$((FAILED_MODULES + 1))
        fi
    done
    if [ ${FAILED_MODULES} -gt 0 ]; then
        COMPILATION_STATUS="FAILED"
        display_error "C-Module subsystem verification rejected. Failed entities: ${FAILED_MODULES}"
        terminate_pipeline 1
    else
        COMPILATION_STATUS="SUCCESS"
        display_success "C-Module subsystem translated successfully."
    fi
}

unify_binary_objects() {
    trigger_phase "BINARY UNIFICATION AND LINKAGE"
    local object_artifacts=("${OUT_DIR}"/*.o)
    if [ "${#object_artifacts[@]}" -eq 0 ] || [ ! -e "${object_artifacts[0]}" ]; then
        display_error "Object artifacts missing. Linkage phase blocked."
        LINKAGE_STATUS="FAILED"
        terminate_pipeline 1
    fi
    local linkage_log="${LOG_DIR}/linker.log"
    display_info "Invoking linker unification layer targeting ${KERNEL_BIN}"
    if ${LINKER_CMD} "${LINKER_FLAGS[@]}" -o "${KERNEL_BIN}" "${object_artifacts[@]}" > "${linkage_log}" 2>&1; then
        display_success "Object artifacts unified. Execution matrix generated."
        LINKAGE_STATUS="SUCCESS"
    else
        display_error "Linkage constraint violations detected."
        cat "${linkage_log}"
        LINKAGE_STATUS="FAILED"
        terminate_pipeline 1
    fi
}

extract_binary_heuristics() {
    trigger_phase "POST-LINKAGE HEURISTIC EXTRACTION"
    if [ ! -f "${KERNEL_BIN}" ]; then
        display_warning "Kernel binary unreadable. Bypassing heuristic extraction."
        return
    fi
    local binary_size
    binary_size=$(stat -c%s "${KERNEL_BIN}" 2>/dev/null || stat -f%z "${KERNEL_BIN}" 2>/dev/null || echo "0")
    display_info "Compiled Matrix Size: ${binary_size} bytes"
    local checksum_signature
    checksum_signature=$(${CHECKSUM_CMD} "${KERNEL_BIN}" | awk '{print $1}')
    display_info "Cryptographic Signature (SHA-256): ${checksum_signature}"
    ${ELF_READER_CMD} -h "${KERNEL_BIN}" > "${ARTIFACT_DIR}/elf_headers.txt" || true
    ${BINARY_DUMP_CMD} -h "${KERNEL_BIN}" > "${ARTIFACT_DIR}/section_headers.txt" || true
    display_success "Heuristic telemetry exported to artifact matrix."
}

deploy_hypervisor_sandbox() {
    trigger_phase "HYPERVISOR SANDBOX DEPLOYMENT"
    if [ "${LINKAGE_STATUS}" != "SUCCESS" ]; then
        display_error "Linkage dependency failure. Bypassing hypervisor deployment."
        EMULATION_STATUS="BYPASSED"
        return
    fi
    display_info "Provisioning QEMU hypervisor sandbox. Timeout threshold: ${EMULATION_TIMEOUT_SECONDS}s"
    ${EMULATOR_CMD} -kernel "${KERNEL_BIN}" -debugcon file:"${TELEMETRY_LOG}" "${EMULATOR_FLAGS[@]}" > logs/qemu.log 2>&1 &
    EMULATOR_PID=$!
    display_info "Hypervisor dispatched. Process ID attached: ${EMULATOR_PID}"
    local visual_indicators=("[   ]" "[.  ]" "[.. ]" "[...]" "[ ..]" "[  .]")
    local indicator_idx=0
    for (( timer_tick=1; timer_tick<=EMULATION_TIMEOUT_SECONDS; timer_tick++ )); do
        indicator_idx=$(( (indicator_idx + 1) % 6 ))
        printf "\r${COLOR_YELLOW}${visual_indicators[${indicator_idx}]} Monitoring hypervisor telemetry... T-Minus: %d/%d${COLOR_RESET}" "${timer_tick}" "${EMULATION_TIMEOUT_SECONDS}"
        sleep 1
        if ! kill -0 "${EMULATOR_PID}" 2>/dev/null; then
            printf "\n"
            display_error "Hypervisor process terminated unexpectedly."
            EMULATION_STATUS="ABORTED"
            return
        fi
    done
    printf "\n"
    if kill -0 "${EMULATOR_PID}" 2>/dev/null; then
        display_info "Emulation timeframe concluded. Transmitting termination signal to hypervisor."
        kill -9 "${EMULATOR_PID}" 2>/dev/null
        EMULATION_STATUS="COMPLETED"
    else
        EMULATION_STATUS="ABORTED"
    fi
}

scan_hypervisor_telemetry() {
    trigger_phase "HYPERVISOR TELEMETRY INSPECTION"
    if [ ! -s "${TELEMETRY_LOG}" ]; then
        display_warning "Telemetry stream empty. Kernel fault prior to I/O port binding."
        return
    fi
    local panic_signatures="KERNEL PANIC|CRITICAL FAULT|SYSTEM HALTED|Division by zero|ERR_MAIN_LOOP_EXIT|ERR_STACK_SMASH|ERR_SYSCALL_VIOLATION"
    local oom_signatures="OOM_ALLOC_FAILED_COMPLETELY|OOM Kills|Not available RAM|OOM_NO_VICTIM_FOUND|Virtual Machine allocation failed"
    local success_signatures="FS Ready|ATA Ready|Welcome to WhiteX|FS Synced to Disk|System Entropy"
    display_info "Executing deep-scan for Critical Exception signatures."
    if grep -qE "${panic_signatures}" "${TELEMETRY_LOG}"; then
        PANIC_FLAG="TRUE"
        display_error "CRITICAL EXCEPTION SIGNATURE DETECTED."
        grep -E "${panic_signatures}" "${TELEMETRY_LOG}" | while read -r line_data; do
            echo -e "${COLOR_RED}  -> ${line_data}${COLOR_RESET}"
        done
    fi
    display_info "Executing deep-scan for Memory Allocation constraints."
    if grep -qE "${oom_signatures}" "${TELEMETRY_LOG}"; then
        OOM_FLAG="TRUE"
        display_error "OUT OF MEMORY (OOM) EXCEPTION SIGNATURE DETECTED."
        grep -E "${oom_signatures}" "${TELEMETRY_LOG}" | while read -r line_data; do
            echo -e "${COLOR_RED}  -> ${line_data}${COLOR_RESET}"
        done
    fi
    display_info "Executing deep-scan for Subsystem Initialization verifications."
    if grep -qE "${success_signatures}" "${TELEMETRY_LOG}"; then
        display_success "Verified initialization signatures extracted:"
        grep -E "${success_signatures}" "${TELEMETRY_LOG}" | head -n 8 | while read -r line_data; do
            echo -e "${COLOR_GREEN}  -> ${line_data}${COLOR_RESET}"
        done
    fi
}

generate_compliance_report() {
    trigger_phase "PIPELINE COMPLIANCE REPORT"
    END_TIMESTAMP=$(date +%s)
    local duration_diff=$((END_TIMESTAMP - START_TIMESTAMP))
    local final_status="PASS"
    if [ "${LINKAGE_STATUS}" != "SUCCESS" ] || [ "${PANIC_FLAG}" = "TRUE" ] || [ "${OOM_FLAG}" = "TRUE" ]; then
        final_status="FAIL"
    fi
    cat > "${METRICS_FILE}" <<EOF
{
    "project_identifier": "${PROJECT_IDENTIFIER}",
    "pipeline_revision": "${PIPELINE_REVISION}",
    "execution_duration_sec": ${duration_diff},
    "subsystem_metrics": {
        "c_modules_total": ${TOTAL_MODULES},
        "c_modules_success": ${COMPILED_MODULES},
        "c_modules_failed": ${FAILED_MODULES}
    },
    "phase_results": {
        "assembly_layer": "${ASSEMBLY_STATUS}",
        "c_module_layer": "${COMPILATION_STATUS}",
        "linkage_layer": "${LINKAGE_STATUS}",
        "hypervisor_layer": "${EMULATION_STATUS}"
    },
    "telemetry_metrics": {
        "panic_detected": ${PANIC_FLAG,,},
        "oom_detected": ${OOM_FLAG,,}
    },
    "compliance_status": "${final_status}"
}
EOF
    echo -e "${COLOR_BLUE}${COLOR_BOLD}========================================================================================${COLOR_RESET}"
    printf "${COLOR_BOLD}%-35s${COLOR_RESET} : %-20s\n" "Total Execution Duration" "${duration_diff} seconds"
    printf "${COLOR_BOLD}%-35s${COLOR_RESET} : %-20s\n" "Assembly Translation Layer" "${ASSEMBLY_STATUS}"
    printf "${COLOR_BOLD}%-35s${COLOR_RESET} : %-20s\n" "C-Module Translation Layer" "${COMPILED_MODULES} / ${TOTAL_MODULES} Executed"
    printf "${COLOR_BOLD}%-35s${COLOR_RESET} : %-20s\n" "Binary Unification Layer" "${LINKAGE_STATUS}"
    printf "${COLOR_BOLD}%-35s${COLOR_RESET} : %-20s\n" "Hypervisor Execution Layer" "${EMULATION_STATUS}"
    printf "${COLOR_BOLD}%-35s${COLOR_RESET} : %-20s\n" "Telemetry Panic Signature" "${PANIC_FLAG}"
    printf "${COLOR_BOLD}%-35s${COLOR_RESET} : %-20s\n" "Telemetry OOM Signature" "${OOM_FLAG}"
    printf "${COLOR_BOLD}%-35s${COLOR_RESET} : %-20s\n" "Total Line" "${TOTAL_LINE}"
    echo -e "${COLOR_BLUE}${COLOR_BOLD}========================================================================================${COLOR_RESET}"

    if [ "${final_status}" = "PASS" ]; then
        echo -e "${COLOR_GREEN}${COLOR_BOLD}>>> PIPELINE COMPLIANCE VERIFIED. DEPLOYMENT AUTHORIZED. <<<${COLOR_RESET}"
        terminate_pipeline 0
    else
        echo -e "${COLOR_RED}${COLOR_BOLD}>>> PIPELINE COMPLIANCE REJECTED. CRITICAL FAULTS DETECTED. <<<${COLOR_RESET}"
        terminate_pipeline 1
    fi
}

initialize_header
provision_environment
verify_dependencies
translate_assembly_layer
translate_c_layer
unify_binary_objects
extract_binary_heuristics
deploy_hypervisor_sandbox
scan_hypervisor_telemetry
generate_compliance_report
