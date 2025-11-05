#!/bin/bash
# FRFD - Linux System Information Collection
# Collects comprehensive system information for forensic analysis

OUTPUT_PATH="${1:-/tmp/csirt/evidence}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="${OUTPUT_PATH}/SystemInfo_${TIMESTAMP}"

mkdir -p "${OUTPUT_DIR}"

echo "[FRFD] Collecting Linux system information..."
echo "[FRFD] Output directory: ${OUTPUT_DIR}"

# Function to safely execute and save command output
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
collect "hostname" "hostname -f"
collect "kernel" "uname -a"
collect "os_release" "cat /etc/os-release"
collect "lsb_release" "lsb_release -a"
collect "uptime" "uptime -p"
collect "date" "date"
collect "timezone" "timedatectl"

# Hardware Information
collect "cpu_info" "cat /proc/cpuinfo"
collect "memory_info" "cat /proc/meminfo"
collect "lspci" "lspci -vv"
collect "lsusb" "lsusb -v"
collect "lsblk" "lsblk -a"
collect "df" "df -h"
collect "mount" "mount"
collect "dmidecode" "dmidecode"

# User Information
collect "users" "cat /etc/passwd"
collect "groups" "cat /etc/group"
collect "shadow" "cat /etc/shadow"
collect "sudoers" "cat /etc/sudoers"
collect "logged_in_users" "w"
collect "last_logins" "last -F"
collect "lastb" "lastb"
collect "who" "who -a"

# Network Information
collect "ip_addr" "ip addr show"
collect "ip_route" "ip route show"
collect "ss" "ss -tunap"
collect "netstat" "netstat -tulpn"
collect "arp" "arp -an"
collect "iptables" "iptables -L -n -v"
collect "ip6tables" "ip6tables -L -n -v"
collect "resolv_conf" "cat /etc/resolv.conf"
collect "hosts" "cat /etc/hosts"
collect "hostname_file" "cat /etc/hostname"

# Process Information
collect "ps" "ps auxf"
collect "pstree" "pstree -p"
collect "top_snapshot" "top -b -n 1"

# Service Information
collect "systemctl" "systemctl list-units --all"
collect "systemctl_failed" "systemctl --failed"
collect "service_status" "service --status-all"

# Scheduled Tasks
collect "crontab_root" "crontab -l"
collect "cron_d" "ls -la /etc/cron.d/ && cat /etc/cron.d/*"
collect "cron_daily" "ls -la /etc/cron.daily/"
collect "cron_hourly" "ls -la /etc/cron.hourly/"
collect "cron_weekly" "ls -la /etc/cron.weekly/"
collect "cron_monthly" "ls -la /etc/cron.monthly/"
collect "anacron" "cat /etc/anacrontab"
collect "at_jobs" "atq && ls -la /var/spool/cron/atjobs/"

# Kernel Modules
collect "lsmod" "lsmod"
collect "modprobe" "cat /etc/modprobe.d/*"

# Environment Variables
collect "env" "env"
collect "printenv" "printenv"

# Package Information
if command -v dpkg &> /dev/null; then
    collect "dpkg_list" "dpkg -l"
fi

if command -v rpm &> /dev/null; then
    collect "rpm_list" "rpm -qa"
fi

if command -v apt &> /dev/null; then
    collect "apt_history" "cat /var/log/apt/history.log"
fi

# Startup and Init
collect "rc_local" "cat /etc/rc.local"
collect "systemd_default" "systemctl get-default"
collect "systemd_boot" "systemctl list-unit-files --type=service --state=enabled"

# File System and Open Files
collect "lsof" "lsof"
collect "fuser" "fuser -v /"

# Security
collect "selinux" "sestatus"
collect "apparmor" "aa-status"
collect "ufw" "ufw status verbose"

# SSH Configuration
collect "ssh_config" "cat /etc/ssh/sshd_config"
collect "ssh_authorized_keys" "find /home -name authorized_keys -exec echo {} \; -exec cat {} \;"
collect "ssh_known_hosts" "find /home -name known_hosts -exec echo {} \; -exec cat {} \;"

# Generate JSON summary
cat > "${OUTPUT_DIR}/summary.json" <<EOF
{
  "timestamp": "$(date -Iseconds)",
  "hostname": "$(hostname -f)",
  "kernel": "$(uname -r)",
  "os": "$(cat /etc/os-release | grep PRETTY_NAME | cut -d'"' -f2)",
  "uptime": "$(uptime -p)",
  "collection_path": "${OUTPUT_DIR}",
  "artifacts_collected": $(ls -1 "${OUTPUT_DIR}" | wc -l)
}
EOF

echo "[FRFD] System information collection complete"
echo "[FRFD] Artifacts collected: $(ls -1 "${OUTPUT_DIR}" | wc -l)"
echo "[FRFD] Output: ${OUTPUT_DIR}"

# Create tarball
TARBALL="${OUTPUT_PATH}/SystemInfo_${TIMESTAMP}.tar.gz"
tar -czf "${TARBALL}" -C "$(dirname ${OUTPUT_DIR})" "$(basename ${OUTPUT_DIR})"
echo "[FRFD] Archive created: ${TARBALL}"

echo "${OUTPUT_DIR}"
