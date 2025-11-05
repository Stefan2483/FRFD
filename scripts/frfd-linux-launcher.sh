#!/bin/bash
#
# FRFD - First Responder Forensics Dongle - Linux Launcher
# Main launcher script for FRFD on Linux systems
#

set -e

# FRFD Configuration
FRFD_VERSION="0.1.0"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FORENSICS_DIR="$(dirname "$SCRIPT_DIR")/forensics_tools/linux"

# Default parameters
MODE="${1:-triage}"
OUTPUT_PATH="${2:-/tmp/csirt/evidence}"
CASE_ID="${3:-}"
RESPONDER="${4:-$USER}"

START_TIME=$(date +%s)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${CYAN}[$(date +'%Y-%m-%d %H:%M:%S')]${NC} ${MAGENTA}[FRFD]${NC} $1"
}

log_success() {
    echo -e "${CYAN}[$(date +'%Y-%m-%d %H:%M:%S')]${NC} ${MAGENTA}[FRFD]${NC} ${GREEN}$1${NC}"
}

log_warning() {
    echo -e "${CYAN}[$(date +'%Y-%m-%d %H:%M:%S')]${NC} ${MAGENTA}[FRFD]${NC} ${YELLOW}$1${NC}"
}

log_error() {
    echo -e "${CYAN}[$(date +'%Y-%m-%d %H:%M:%S')]${NC} ${MAGENTA}[FRFD]${NC} ${RED}$1${NC}"
}

show_banner() {
    cat << "EOF"

    ███████╗██████╗ ███████╗██████╗
    ██╔════╝██╔══██╗██╔════╝██╔══██╗
    █████╗  ██████╔╝█████╗  ██║  ██║
    ██╔══╝  ██╔══██╗██╔══╝  ██║  ██║
    ██║     ██║  ██║██║     ██████╔╝
    ╚═╝     ╚═╝  ╚═╝╚═╝     ╚═════╝

    First Responder Forensics Dongle
    Linux Launcher

EOF
    log_info "CSIRT Forensics Automation Tool"
    log_info "Version: $FRFD_VERSION"
    log_info "Mode: $MODE"
    log_info "============================================================"
}

check_root() {
    if [ "$EUID" -ne 0 ]; then
        log_error "This script must be run as root (sudo)"
        exit 1
    fi
}

initialize_frfd() {
    log_info "Initializing FRFD..."

    # Create output directory
    if [ ! -d "$OUTPUT_PATH" ]; then
        mkdir -p "$OUTPUT_PATH"
        log_success "Created output directory: $OUTPUT_PATH"
    fi

    # Generate Case ID if not provided
    if [ -z "$CASE_ID" ]; then
        CASE_ID="INC-$(date +%Y%m%d-%H%M%S)"
    fi

    log_info "Case ID: $CASE_ID"
    log_info "Responder: $RESPONDER"
    log_info "Output Path: $OUTPUT_PATH"
}

run_triage_collection() {
    log_info "Starting Triage Mode..."

    local triage_dir="${OUTPUT_PATH}/triage_$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$triage_dir"

    # Quick system assessment
    log_info "Performing quick system assessment..."

    # Basic system info
    {
        echo "=== FRFD Triage Report ==="
        echo "Timestamp: $(date -Iseconds)"
        echo "Hostname: $(hostname -f)"
        echo "Kernel: $(uname -r)"
        echo "OS: $(cat /etc/os-release 2>/dev/null | grep PRETTY_NAME | cut -d'=' -f2 | tr -d '"')"
        echo "Uptime: $(uptime -p)"
        echo "Current User: $USER"
        echo ""

        echo "=== Network Connections ==="
        ss -tunap | head -20

        echo ""
        echo "=== Running Processes ==="
        ps aux | head -30

        echo ""
        echo "=== Logged In Users ==="
        w

        echo ""
        echo "=== Recent Authentication Failures ==="
        grep "Failed" /var/log/auth.log 2>/dev/null | tail -10 || echo "No auth log available"

        echo ""
        echo "=== Listening Ports ==="
        ss -tlnp

    } > "$triage_dir/triage_report.txt"

    # Create JSON summary
    cat > "$triage_dir/triage_summary.json" <<EOF
{
  "case_id": "$CASE_ID",
  "responder": "$RESPONDER",
  "timestamp": "$(date -Iseconds)",
  "hostname": "$(hostname -f)",
  "mode": "triage",
  "output_path": "$triage_dir"
}
EOF

    log_success "Triage complete: $triage_dir"
    echo "$triage_dir"
}

run_full_collection() {
    log_info "Starting Full Collection Mode..."

    local collection_results=()

    # System Information
    log_info "Collecting system information..."
    if [ -f "$FORENSICS_DIR/system/system_info.sh" ]; then
        if bash "$FORENSICS_DIR/system/system_info.sh" "$OUTPUT_PATH"; then
            collection_results+=("system_info:success")
            log_success "System information collected"
        else
            collection_results+=("system_info:failed")
            log_warning "System information collection failed"
        fi
    else
        log_warning "System info script not found"
    fi

    # Authentication Logs
    log_info "Collecting authentication logs..."
    if [ -f "$FORENSICS_DIR/logs/auth_logs.sh" ]; then
        if bash "$FORENSICS_DIR/logs/auth_logs.sh" "$OUTPUT_PATH"; then
            collection_results+=("auth_logs:success")
            log_success "Authentication logs collected"
        else
            collection_results+=("auth_logs:failed")
            log_warning "Authentication log collection failed"
        fi
    else
        log_warning "Auth logs script not found"
    fi

    # Network Information
    log_info "Collecting network information..."
    if [ -f "$FORENSICS_DIR/network/netstat.sh" ]; then
        if bash "$FORENSICS_DIR/network/netstat.sh" "$OUTPUT_PATH"; then
            collection_results+=("network:success")
            log_success "Network information collected"
        else
            collection_results+=("network:failed")
            log_warning "Network collection failed"
        fi
    else
        log_warning "Network script not found"
    fi

    # Persistence mechanisms
    log_info "Checking persistence mechanisms..."
    local persist_dir="${OUTPUT_PATH}/persistence_$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$persist_dir"

    # Crontabs
    {
        echo "=== Root Crontab ==="
        crontab -l 2>/dev/null || echo "No root crontab"

        echo ""
        echo "=== User Crontabs ==="
        for user in $(cut -f1 -d: /etc/passwd); do
            echo "User: $user"
            crontab -u "$user" -l 2>/dev/null || echo "No crontab for $user"
        done

        echo ""
        echo "=== Systemd Timers ==="
        systemctl list-timers --all
    } > "$persist_dir/scheduled_tasks.txt"

    # Systemd services
    {
        echo "=== Enabled Services ==="
        systemctl list-unit-files --state=enabled --type=service
    } > "$persist_dir/enabled_services.txt"

    collection_results+=("persistence:success")

    # Kernel Modules Analysis
    log_info "Analyzing kernel modules..."
    if [ -f "$FORENSICS_DIR/system/kernel_modules.sh" ]; then
        if bash "$FORENSICS_DIR/system/kernel_modules.sh" "$OUTPUT_PATH"; then
            collection_results+=("kernel_modules:success")
            log_success "Kernel modules analyzed"
        else
            collection_results+=("kernel_modules:failed")
            log_warning "Kernel modules analysis failed"
        fi
    else
        log_warning "Kernel modules script not found"
    fi

    # Comprehensive Persistence Check
    log_info "Running comprehensive persistence check..."
    if [ -f "$FORENSICS_DIR/persistence/persistence_check.sh" ]; then
        if bash "$FORENSICS_DIR/persistence/persistence_check.sh" "$OUTPUT_PATH"; then
            collection_results+=("persistence_check:success")
            log_success "Persistence check complete"
        else
            collection_results+=("persistence_check:failed")
            log_warning "Persistence check failed"
        fi
    else
        log_warning "Persistence check script not found"
    fi

    log_success "Full collection complete!"

    # Print summary
    log_info "Collection Summary:"
    for result in "${collection_results[@]}"; do
        log_info "  - $result"
    done
}

run_containment_procedure() {
    log_warning "Starting Containment Mode..."
    log_warning "WARNING: This will implement network isolation and security controls!"

    echo -n "Type 'CONTAIN' to proceed with containment actions: "
    read confirmation

    if [ "$confirmation" != "CONTAIN" ]; then
        log_info "Containment cancelled by user"
        return
    fi

    local containment_log="${OUTPUT_PATH}/containment_$(date +%Y%m%d_%H%M%S).json"

    log_info "Implementing network isolation..."

    # Block all outbound traffic (with exception for localhost)
    if command -v iptables &> /dev/null; then
        log_info "Configuring iptables rules..."

        # Backup current rules
        iptables-save > "${OUTPUT_PATH}/iptables_backup_$(date +%Y%m%d_%H%M%S).txt"

        # Block outbound traffic
        iptables -P OUTPUT DROP
        iptables -A OUTPUT -o lo -j ACCEPT
        iptables -A OUTPUT -m state --state ESTABLISHED,RELATED -j ACCEPT

        log_success "Outbound traffic blocked (localhost and established connections allowed)"
    else
        log_warning "iptables not available"
    fi

    # Log containment action
    cat > "$containment_log" <<EOF
{
  "timestamp": "$(date -Iseconds)",
  "action": "network_isolation",
  "responder": "$RESPONDER",
  "case_id": "$CASE_ID",
  "hostname": "$(hostname -f)",
  "firewall_rules_applied": true
}
EOF

    log_success "Containment procedures complete"
    log_info "Containment log: $containment_log"
}

run_analysis() {
    log_info "Starting Analysis Mode..."
    log_warning "Analysis mode not yet fully implemented"

    # Placeholder for analysis functionality
    # Would include:
    # - IOC matching with YARA
    # - Timeline generation
    # - Anomaly detection

    log_info "Analysis complete"
}

generate_chain_of_custody() {
    log_info "Generating Chain of Custody documentation..."

    local end_time=$(date +%s)
    local duration=$((end_time - START_TIME))

    local custody_file="${OUTPUT_PATH}/chain_of_custody_$(date +%Y%m%d_%H%M%S).json"

    # Create JSON structure
    cat > "$custody_file" <<EOF
{
  "case_id": "$CASE_ID",
  "responder": "$RESPONDER",
  "device_id": "FRFD-001",
  "hostname": "$(hostname -f)",
  "start_time": "$(date -d @$START_TIME -Iseconds)",
  "end_time": "$(date -Iseconds)",
  "duration_seconds": $duration,
  "mode": "$MODE",
  "output_path": "$OUTPUT_PATH",
  "artifacts": [
EOF

    # List artifacts
    local first=true
    find "$OUTPUT_PATH" -type f 2>/dev/null | while read -r file; do
        if [ "$first" = true ]; then
            first=false
        else
            echo "," >> "$custody_file"
        fi

        local hash=$(sha256sum "$file" 2>/dev/null | awk '{print $1}')
        local size=$(stat -f %z "$file" 2>/dev/null || stat -c %s "$file" 2>/dev/null)

        cat >> "$custody_file" <<ARTIFACT
    {
      "filename": "$(basename "$file")",
      "path": "$file",
      "size": $size,
      "hash": "sha256:$hash"
    }
ARTIFACT
    done

    cat >> "$custody_file" <<EOF

  ]
}
EOF

    log_success "Chain of Custody: $custody_file"
}

send_results_to_device() {
    log_info "Preparing results for transfer to device..."

    # In real implementation, this would:
    # 1. Compress collected artifacts
    # 2. Send via serial/USB to dongle
    # 3. Or upload to WiFi endpoint

    cat <<EOF
{
  "status": "complete",
  "mode": "$MODE",
  "case_id": "$CASE_ID",
  "output_path": "$OUTPUT_PATH",
  "timestamp": "$(date -Iseconds)"
}
EOF
}

show_usage() {
    cat <<EOF
Usage: $0 [MODE] [OUTPUT_PATH] [CASE_ID] [RESPONDER]

Modes:
  triage   - Quick system assessment (default)
  collect  - Full forensic collection
  contain  - Implement containment procedures
  analyze  - Run analysis on collected data

Examples:
  sudo $0 triage
  sudo $0 collect /evidence INC-2024-001 john.doe
  sudo $0 contain /evidence INC-2024-001 john.doe

EOF
}

# Main execution
main() {
    show_banner

    # Check for help
    if [ "$MODE" = "-h" ] || [ "$MODE" = "--help" ]; then
        show_usage
        exit 0
    fi

    check_root
    initialize_frfd

    case "$MODE" in
        triage)
            run_triage_collection
            ;;
        collect)
            run_full_collection
            ;;
        contain)
            run_containment_procedure
            ;;
        analyze)
            run_analysis
            ;;
        *)
            log_error "Unknown mode: $MODE"
            show_usage
            exit 1
            ;;
    esac

    # Generate chain of custody
    generate_chain_of_custody

    # Send results
    send_results_to_device

    local end_time=$(date +%s)
    local duration=$((end_time - START_TIME))

    log_success "FRFD execution complete in ${duration} seconds"
}

# Run main function
main "$@"
