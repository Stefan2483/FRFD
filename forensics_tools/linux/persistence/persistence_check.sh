#!/bin/bash
# FRFD - Linux Persistence Mechanisms Check
# Comprehensive check for Linux persistence mechanisms

OUTPUT_PATH="${1:-/tmp/csirt/evidence}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="${OUTPUT_PATH}/Persistence_${TIMESTAMP}"

mkdir -p "${OUTPUT_DIR}"

echo "[FRFD] Checking Linux persistence mechanisms..."
echo "[FRFD] Output directory: ${OUTPUT_DIR}"

# Function to collect safely
collect() {
    local name=$1
    local cmd=$2
    local output_file="${OUTPUT_DIR}/${name}.txt"

    echo "[FRFD] Collecting: ${name}"
    echo "=== ${name} ===" > "${output_file}"
    echo "Timestamp: $(date -Iseconds)" >> "${output_file}"
    echo "" >> "${output_file}"

    eval "${cmd}" >> "${output_file}" 2>&1 || echo "Error or not found" >> "${output_file}"
}

# 1. Cron Jobs
echo "[FRFD] Collecting cron jobs..."
{
    echo "=== Root Crontab ==="
    crontab -l 2>/dev/null || echo "No root crontab"

    echo -e "\n=== All User Crontabs ==="
    for user in $(cut -f1 -d: /etc/passwd); do
        echo "--- User: ${user} ---"
        crontab -u "${user}" -l 2>/dev/null || echo "No crontab for ${user}"
    done

    echo -e "\n=== /etc/cron.d/ ==="
    ls -la /etc/cron.d/ 2>/dev/null
    cat /etc/cron.d/* 2>/dev/null

    echo -e "\n=== /etc/cron.daily/ ==="
    ls -la /etc/cron.daily/ 2>/dev/null

    echo -e "\n=== /etc/cron.hourly/ ==="
    ls -la /etc/cron.hourly/ 2>/dev/null

    echo -e "\n=== /etc/cron.weekly/ ==="
    ls -la /etc/cron.weekly/ 2>/dev/null

    echo -e "\n=== /etc/cron.monthly/ ==="
    ls -la /etc/cron.monthly/ 2>/dev/null

    echo -e "\n=== /var/spool/cron/ ==="
    ls -laR /var/spool/cron/ 2>/dev/null

} > "${OUTPUT_DIR}/cron_jobs.txt"

# 2. Systemd Services and Timers
echo "[FRFD] Collecting systemd persistence..."
collect "systemd_enabled_services" "systemctl list-unit-files --state=enabled --type=service"
collect "systemd_enabled_timers" "systemctl list-unit-files --state=enabled --type=timer"
collect "systemd_timers_active" "systemctl list-timers --all"
collect "systemd_failed" "systemctl --failed"

# Copy suspicious systemd unit files
SYSTEMD_DIR="${OUTPUT_DIR}/systemd_units"
mkdir -p "${SYSTEMD_DIR}"

echo "[FRFD] Copying systemd unit files from user directories..."
find /home -name "*.service" -o -name "*.timer" 2>/dev/null | while read -r file; do
    cp "${file}" "${SYSTEMD_DIR}/" 2>/dev/null
done

# 3. Init Scripts (SysV)
echo "[FRFD] Collecting init scripts..."
collect "init_scripts" "ls -la /etc/init.d/"
collect "rc_local" "cat /etc/rc.local"
collect "rclocal_service" "systemctl status rc-local"

# 4. Startup Files
echo "[FRFD] Collecting startup files..."
{
    echo "=== /etc/profile ==="
    cat /etc/profile 2>/dev/null

    echo -e "\n=== /etc/profile.d/ ==="
    ls -la /etc/profile.d/ 2>/dev/null
    cat /etc/profile.d/* 2>/dev/null

    echo -e "\n=== /etc/bash.bashrc ==="
    cat /etc/bash.bashrc 2>/dev/null

    echo -e "\n=== /etc/environment ==="
    cat /etc/environment 2>/dev/null

} > "${OUTPUT_DIR}/system_startup_files.txt"

# 5. User Startup Files
echo "[FRFD] Collecting user startup files..."
USER_STARTUP_DIR="${OUTPUT_DIR}/user_startup_files"
mkdir -p "${USER_STARTUP_DIR}"

for user_home in /root /home/*; do
    if [ -d "${user_home}" ]; then
        username=$(basename "${user_home}")
        echo "[FRFD]   User: ${username}"

        {
            echo "=== Startup files for: ${username} ==="

            for file in .bashrc .bash_profile .profile .zshrc .zprofile .config/autostart; do
                filepath="${user_home}/${file}"
                if [ -e "${filepath}" ]; then
                    echo "--- ${file} ---"
                    cat "${filepath}" 2>/dev/null
                    echo ""
                fi
            done

        } > "${USER_STARTUP_DIR}/${username}_startup.txt"

        # Check for suspicious entries
        if grep -qE "curl|wget|nc|bash -i|/dev/tcp|base64|eval" "${user_home}"/.bashrc "${user_home}"/.profile 2>/dev/null; then
            echo "[FRFD]   ⚠️  SUSPICIOUS entries in ${username} startup files"
        fi
    fi
done

# 6. SSH Persistence
echo "[FRFD] Checking SSH persistence mechanisms..."
{
    echo "=== SSH Authorized Keys ==="
    find /home -name "authorized_keys" -o -name "authorized_keys2" 2>/dev/null | while read -r file; do
        echo "--- ${file} ---"
        ls -la "${file}"
        cat "${file}" 2>/dev/null
        echo ""
    done

    if [ -f "/root/.ssh/authorized_keys" ]; then
        echo "--- /root/.ssh/authorized_keys ---"
        ls -la /root/.ssh/authorized_keys
        cat /root/.ssh/authorized_keys
    fi

    echo -e "\n=== SSH Configuration ==="
    cat /etc/ssh/sshd_config 2>/dev/null

    echo -e "\n=== SSH Host Keys ==="
    ls -la /etc/ssh/ssh_host_*

} > "${OUTPUT_DIR}/ssh_persistence.txt"

# 7. At Jobs
collect "at_jobs" "atq"
collect "at_spool" "ls -la /var/spool/cron/atjobs/ 2>/dev/null || echo 'No at jobs'"

# 8. Kernel Module Autoloading
collect "modules_load" "cat /etc/modules-load.d/*"
collect "modprobe_conf" "cat /etc/modprobe.d/*"

# 9. LD_PRELOAD and Library Injection
echo "[FRFD] Checking for library injection..."
{
    echo "=== /etc/ld.so.preload ==="
    if [ -f "/etc/ld.so.preload" ]; then
        echo "⚠️  WARNING: /etc/ld.so.preload exists!"
        ls -la /etc/ld.so.preload
        cat /etc/ld.so.preload
    else
        echo "File does not exist (normal)"
    fi

    echo -e "\n=== LD_LIBRARY_PATH and LD_PRELOAD in environment ==="
    env | grep -E "LD_PRELOAD|LD_LIBRARY_PATH" || echo "Not set"

    echo -e "\n=== Library Configuration ==="
    cat /etc/ld.so.conf 2>/dev/null
    cat /etc/ld.so.conf.d/* 2>/dev/null

} > "${OUTPUT_DIR}/library_injection.txt"

# 10. SUID/SGID Files
echo "[FRFD] Finding SUID/SGID files (may take a while)..."
find / -type f \( -perm -4000 -o -perm -2000 \) -ls 2>/dev/null > "${OUTPUT_DIR}/suid_sgid_files.txt" &
FIND_PID=$!

# 11. Docker/Container Persistence (if applicable)
if command -v docker &> /dev/null; then
    echo "[FRFD] Checking Docker persistence..."
    collect "docker_containers" "docker ps -a"
    collect "docker_images" "docker images"
    collect "docker_volumes" "docker volume ls"
fi

# 12. PAM Configuration
echo "[FRFD] Collecting PAM configuration..."
collect "pam_conf" "ls -la /etc/pam.d/ && cat /etc/pam.d/*"
collect "pam_modules" "ls -la /lib/security/ /lib64/security/ /usr/lib/security/ 2>/dev/null"

# 13. Suspicious File Locations
echo "[FRFD] Checking suspicious file locations..."
{
    echo "=== /tmp/ executable files ==="
    find /tmp -type f -executable 2>/dev/null | head -50

    echo -e "\n=== /dev/shm/ files ==="
    find /dev/shm -type f 2>/dev/null

    echo -e "\n=== Hidden files in /tmp ==="
    find /tmp -name ".*" -type f 2>/dev/null | head -50

    echo -e "\n=== Recently modified files in system directories ==="
    find /bin /sbin /usr/bin /usr/sbin -type f -mtime -7 2>/dev/null | head -50

} > "${OUTPUT_DIR}/suspicious_locations.txt"

# 14. Webshells (common web directories)
echo "[FRFD] Checking for webshells..."
{
    for webdir in /var/www /usr/share/nginx /opt/lampp/htdocs /var/lib/tomcat; do
        if [ -d "${webdir}" ]; then
            echo "=== Checking ${webdir} ==="
            find "${webdir}" -name "*.php" -o -name "*.jsp" -o -name "*.asp" 2>/dev/null | while read -r file; do
                # Check for common webshell patterns
                if grep -qE "eval|base64_decode|shell_exec|system\(|passthru|exec\(" "${file}" 2>/dev/null; then
                    echo "⚠️  Suspicious: ${file}"
                fi
            done
        fi
    done
} > "${OUTPUT_DIR}/webshell_check.txt"

# Wait for SUID/SGID search to complete (with timeout)
wait $FIND_PID 2>/dev/null

# 15. Create Comprehensive Analysis
echo "[FRFD] Creating persistence analysis summary..."
{
    echo "=== FRFD Linux Persistence Analysis ==="
    echo "Generated: $(date -Iseconds)"
    echo "Hostname: $(hostname -f)"
    echo ""

    echo "=== Summary ==="
    echo "Cron jobs found: $(find /etc/cron* /var/spool/cron -type f 2>/dev/null | wc -l)"
    echo "Systemd enabled services: $(systemctl list-unit-files --state=enabled --type=service 2>/dev/null | grep -c enabled)"
    echo "Systemd timers: $(systemctl list-timers --all 2>/dev/null | grep -c timer)"
    echo "At jobs: $(atq 2>/dev/null | wc -l)"
    echo "SUID files: $(wc -l < "${OUTPUT_DIR}/suid_sgid_files.txt")"

    echo ""
    echo "=== High Priority Checks ==="

    # Check ld.so.preload
    if [ -f "/etc/ld.so.preload" ]; then
        echo "⚠️  CRITICAL: /etc/ld.so.preload exists!"
    fi

    # Check for suspicious cron entries
    if grep -rE "curl|wget.*sh|bash -i|nc.*-e|/dev/tcp" /etc/cron* /var/spool/cron 2>/dev/null; then
        echo "⚠️  SUSPICIOUS: Potentially malicious cron entries found"
    fi

    # Check for SSH key injection
    SSH_KEY_COUNT=$(find /home -name "authorized_keys" -exec wc -l {} \; 2>/dev/null | awk '{sum+=$1} END {print sum}')
    echo "SSH authorized keys total entries: ${SSH_KEY_COUNT}"

    echo ""
    echo "=== Recommendations ==="
    echo "1. Review all cron jobs for suspicious commands"
    echo "2. Check systemd services not from package manager"
    echo "3. Verify SSH authorized_keys for all users"
    echo "4. Review SUID/SGID files for anomalies"
    echo "5. Check startup files (.bashrc, .profile) for malicious code"

} > "${OUTPUT_DIR}/persistence_summary.txt"

cat "${OUTPUT_DIR}/persistence_summary.txt"

# Create JSON summary
cat > "${OUTPUT_DIR}/summary.json" <<EOF
{
  "timestamp": "$(date -Iseconds)",
  "hostname": "$(hostname -f)",
  "collection_type": "persistence_check",
  "output_path": "${OUTPUT_DIR}",
  "statistics": {
    "cron_jobs": $(find /etc/cron* /var/spool/cron -type f 2>/dev/null | wc -l),
    "systemd_enabled": $(systemctl list-unit-files --state=enabled 2>/dev/null | grep -c enabled),
    "at_jobs": $(atq 2>/dev/null | wc -l),
    "suid_files": $(wc -l < "${OUTPUT_DIR}/suid_sgid_files.txt")
  }
}
EOF

echo "[FRFD] Persistence check complete"
echo "[FRFD] Output: ${OUTPUT_DIR}"

# Create tarball
TARBALL="${OUTPUT_PATH}/Persistence_${TIMESTAMP}.tar.gz"
tar -czf "${TARBALL}" -C "$(dirname ${OUTPUT_DIR})" "$(basename ${OUTPUT_DIR})" 2>/dev/null
echo "[FRFD] Archive created: ${TARBALL}"

echo "${OUTPUT_DIR}"
