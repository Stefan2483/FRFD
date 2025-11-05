#!/bin/bash
# FRFD - macOS System Information Collection
# Collects comprehensive macOS system information

OUTPUT_PATH="${1:-/tmp/csirt/evidence}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="${OUTPUT_PATH}/macOS_SystemInfo_${TIMESTAMP}"

mkdir -p "${OUTPUT_DIR}"

echo "[FRFD] Collecting macOS system information..."
echo "[FRFD] Output directory: ${OUTPUT_DIR}"

# Function to collect command output
collect() {
    local name=$1
    local cmd=$2
    local output_file="${OUTPUT_DIR}/${name}.txt"

    echo "[FRFD] Collecting: ${name}"
    echo "=== ${name} ===" > "${output_file}"
    echo "Command: ${cmd}" >> "${output_file}"
    echo "Timestamp: $(date -Iseconds)" >> "${output_file}"
    echo "" >> "${output_file}"

    eval "${cmd}" >> "${output_file}" 2>&1 || echo "Error executing command" >> "${output_file}"
}

# System Information
collect "hostname" "hostname"
collect "system_version" "sw_vers"
collect "system_profiler" "system_profiler SPSoftwareDataType SPHardwareDataType"
collect "kernel_version" "uname -a"
collect "uptime" "uptime"
collect "date" "date"

# Hardware
collect "hardware_overview" "system_profiler SPHardwareDataType"
collect "storage" "diskutil list"
collect "disk_info" "df -h"

# User Information
collect "users" "dscl . list /Users"
collect "current_user" "whoami"
collect "logged_in_users" "who"
collect "last_logins" "last"

# Network Information
collect "network_interfaces" "ifconfig -a"
collect "network_routes" "netstat -rn"
collect "network_connections" "netstat -an"
collect "listening_ports" "lsof -iTCP -sTCP:LISTEN -P -n"
collect "dns_config" "scutil --dns"
collect "wifi_info" "/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport -I"

# Process Information
collect "processes" "ps auxww"
collect "top_snapshot" "top -l 1"

# Launch Agents and Daemons
collect "launch_agents_user" "ls -la ~/Library/LaunchAgents/"
collect "launch_agents_system" "ls -la /Library/LaunchAgents/"
collect "launch_daemons" "ls -la /Library/LaunchDaemons/"
collect "launch_daemons_system" "ls -la /System/Library/LaunchDaemons/"

# Installed Applications
collect "applications" "ls -la /Applications/"
collect "user_applications" "ls -la ~/Applications/"

# Startup Items
collect "login_items" "osascript -e 'tell application \"System Events\" to get the name of every login item'"

# File System
collect "mounts" "mount"

# Kernel Extensions
collect "kexts_loaded" "kextstat"
collect "kexts_system" "ls -la /System/Library/Extensions/"
collect "kexts_library" "ls -la /Library/Extensions/"

# Firewall
collect "firewall_status" "/usr/libexec/ApplicationFirewall/socketfilterfw --getglobalstate"
collect "firewall_apps" "/usr/libexec/ApplicationFirewall/socketfilterfw --listapps"

# Security Settings
collect "gatekeeper" "spctl --status"
collect "sip_status" "csrutil status"
collect "filevault" "fdesetup status"

# Installed Packages
collect "homebrew_packages" "brew list 2>/dev/null || echo 'Homebrew not installed'"
collect "macports_packages" "port installed 2>/dev/null || echo 'MacPorts not installed'"

# Environment
collect "environment" "printenv"

# Shell Configuration
collect "bash_profile" "cat ~/.bash_profile 2>/dev/null || echo 'Not found'"
collect "bashrc" "cat ~/.bashrc 2>/dev/null || echo 'Not found'"
collect "zshrc" "cat ~/.zshrc 2>/dev/null || echo 'Not found'"

# System Logs (recent)
collect "system_log" "log show --predicate 'eventMessage contains \"error\" || eventMessage contains \"fail\"' --last 1h --info"

# Quarantine Database
collect "quarantine_db" "sqlite3 ~/Library/Preferences/com.apple.LaunchServices.QuarantineEventsV2 'select * from LSQuarantineEvent' 2>/dev/null || echo 'Cannot access'"

# Browser History (Safari)
collect "safari_history" "sqlite3 ~/Library/Safari/History.db 'select url, datetime(visit_time + 978307200, \"unixepoch\", \"localtime\") from history_visits order by visit_time desc limit 100' 2>/dev/null || echo 'Cannot access'"

# Generate JSON summary
cat > "${OUTPUT_DIR}/summary.json" <<EOF
{
  "timestamp": "$(date -Iseconds)",
  "hostname": "$(hostname)",
  "os_version": "$(sw_vers -productVersion)",
  "build": "$(sw_vers -buildVersion)",
  "kernel": "$(uname -r)",
  "collection_path": "${OUTPUT_DIR}",
  "artifacts_collected": $(ls -1 "${OUTPUT_DIR}" | wc -l)
}
EOF

echo "[FRFD] macOS system information collection complete"
echo "[FRFD] Artifacts collected: $(ls -1 "${OUTPUT_DIR}" | wc -l)"
echo "[FRFD] Output: ${OUTPUT_DIR}"

# Create tarball
TARBALL="${OUTPUT_PATH}/macOS_SystemInfo_${TIMESTAMP}.tar.gz"
tar -czf "${TARBALL}" -C "$(dirname ${OUTPUT_DIR})" "$(basename ${OUTPUT_DIR})"
echo "[FRFD] Archive created: ${TARBALL}"

echo "${OUTPUT_DIR}"
