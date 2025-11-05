#!/bin/bash
# FRFD - macOS Persistence Mechanisms Check
# Comprehensive check for macOS persistence mechanisms

OUTPUT_PATH="${1:-/tmp/csirt/evidence}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="${OUTPUT_PATH}/macOS_Persistence_${TIMESTAMP}"

mkdir -p "${OUTPUT_DIR}"

echo "[FRFD] Checking macOS persistence mechanisms..."
echo "[FRFD] Output directory: ${OUTPUT_DIR}"

# Function to collect
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

# 1. Launch Agents (User)
echo "[FRFD] Collecting Launch Agents (User)..."
{
    echo "=== User Launch Agents ==="
    for dir in ~/Library/LaunchAgents /Library/LaunchAgents; do
        if [ -d "${dir}" ]; then
            echo ""
            echo "Directory: ${dir}"
            ls -la "${dir}"
            echo ""
            for plist in "${dir}"/*.plist; do
                if [ -f "${plist}" ]; then
                    echo "--- $(basename ${plist}) ---"
                    cat "${plist}"
                    echo ""
                fi
            done
        fi
    done
} > "${OUTPUT_DIR}/launch_agents.txt"

# 2. Launch Daemons (System)
echo "[FRFD] Collecting Launch Daemons..."
{
    echo "=== Launch Daemons ==="
    for dir in /Library/LaunchDaemons /System/Library/LaunchDaemons; do
        if [ -d "${dir}" ]; then
            echo ""
            echo "Directory: ${dir}"
            ls -la "${dir}"
            echo ""
        fi
    done
} > "${OUTPUT_DIR}/launch_daemons.txt"

# 3. Login Items
echo "[FRFD] Collecting Login Items..."
{
    echo "=== Login Items ==="
    osascript -e 'tell application "System Events" to get the name of every login item' 2>/dev/null

    echo ""
    echo "=== Login Items (plist) ==="
    if [ -f ~/Library/Preferences/com.apple.loginitems.plist ]; then
        plutil -p ~/Library/Preferences/com.apple.loginitems.plist
    fi
} > "${OUTPUT_DIR}/login_items.txt"

# 4. Cron Jobs
echo "[FRFD] Collecting cron jobs..."
{
    echo "=== Root Crontab ==="
    sudo crontab -l 2>/dev/null || echo "No root crontab"

    echo ""
    echo "=== User Crontabs ==="
    for user in $(dscl . list /Users | grep -v '^_'); do
        echo "--- User: ${user} ---"
        sudo crontab -u "${user}" -l 2>/dev/null || echo "No crontab for ${user}"
    done

    echo ""
    echo "=== /etc/crontab ==="
    cat /etc/crontab 2>/dev/null || echo "Not found"
} > "${OUTPUT_DIR}/cron_jobs.txt"

# 5. Startup Items (Legacy)
echo "[FRFD] Checking startup items..."
collect "startup_items_system" "ls -laR /Library/StartupItems/"
collect "startup_items_core_services" "ls -laR /System/Library/CoreServices/"

# 6. Kernel Extensions
echo "[FRFD] Collecting kernel extensions..."
{
    echo "=== Loaded Kernel Extensions ==="
    kextstat

    echo ""
    echo "=== Third-Party Kernel Extensions ==="
    ls -la /Library/Extensions/

    echo ""
    echo "=== System Kernel Extensions ==="
    ls -la /System/Library/Extensions/ | head -50
} > "${OUTPUT_DIR}/kernel_extensions.txt"

# 7. Browser Extensions (Safari)
echo "[FRFD] Checking Safari extensions..."
{
    echo "=== Safari Extensions ==="
    ls -la ~/Library/Safari/Extensions/ 2>/dev/null || echo "No extensions found"

    echo ""
    echo "=== Safari Extension Settings ==="
    defaults read com.apple.Safari Extensions 2>/dev/null || echo "Cannot read"
} > "${OUTPUT_DIR}/safari_extensions.txt"

# 8. Periodic Scripts
echo "[FRFD] Collecting periodic scripts..."
collect "periodic_daily" "ls -la /etc/periodic/daily/"
collect "periodic_weekly" "ls -la /etc/periodic/weekly/"
collect "periodic_monthly" "ls -la /etc/periodic/monthly/"

# 9. Shell Profiles
echo "[FRFD] Collecting shell profiles..."
{
    echo "=== System Shell Profiles ==="
    cat /etc/profile 2>/dev/null
    cat /etc/bashrc 2>/dev/null
    cat /etc/zshrc 2>/dev/null

    echo ""
    echo "=== User Shell Profiles ==="
    for file in ~/.bash_profile ~/.bashrc ~/.profile ~/.zshrc ~/.zprofile; do
        if [ -f "${file}" ]; then
            echo "--- ${file} ---"
            cat "${file}"
            echo ""
        fi
    done
} > "${OUTPUT_DIR}/shell_profiles.txt"

# 10. Emond (Event Monitor Daemon)
echo "[FRFD] Checking emond..."
{
    echo "=== Emond Rules ==="
    ls -la /etc/emond.d/rules/ 2>/dev/null || echo "No emond rules"

    echo ""
    if [ -d /etc/emond.d/rules/ ]; then
        for rule in /etc/emond.d/rules/*; do
            if [ -f "${rule}" ]; then
                echo "--- $(basename ${rule}) ---"
                cat "${rule}"
                echo ""
            fi
        done
    fi
} > "${OUTPUT_DIR}/emond.txt"

# 11. Authorization Plugins
echo "[FRFD] Checking authorization plugins..."
{
    echo "=== Authorization Database ==="
    security authorizationdb read system.login.console 2>/dev/null

    echo ""
    echo "=== Authorization Plugins ==="
    ls -la /Library/Security/SecurityAgentPlugins/ 2>/dev/null
} > "${OUTPUT_DIR}/authorization_plugins.txt"

# 12. Dylib Hijacking Check
echo "[FRFD] Checking for potential dylib hijacking..."
{
    echo "=== DYLD Environment Variables ==="
    launchctl getenv DYLD_INSERT_LIBRARIES 2>/dev/null || echo "Not set"
    launchctl getenv DYLD_FRAMEWORK_PATH 2>/dev/null || echo "Not set"
    launchctl getenv DYLD_LIBRARY_PATH 2>/dev/null || echo "Not set"
} > "${OUTPUT_DIR}/dylib_check.txt"

# 13. Application Support
echo "[FRFD] Checking Application Support directories..."
{
    echo "=== User Application Support ==="
    ls -la ~/Library/Application\ Support/ 2>/dev/null | head -50

    echo ""
    echo "=== System Application Support ==="
    ls -la /Library/Application\ Support/ 2>/dev/null | head -50
} > "${OUTPUT_DIR}/application_support.txt"

# 14. Suspicious Analysis
echo "[FRFD] Creating persistence analysis summary..."
{
    echo "=== FRFD macOS Persistence Analysis ==="
    echo "Generated: $(date -Iseconds)"
    echo "Hostname: $(hostname)"
    echo ""

    echo "=== Summary ==="
    echo "User Launch Agents: $(ls ~/Library/LaunchAgents/*.plist 2>/dev/null | wc -l)"
    echo "System Launch Agents: $(ls /Library/LaunchAgents/*.plist 2>/dev/null | wc -l)"
    echo "Launch Daemons: $(ls /Library/LaunchDaemons/*.plist 2>/dev/null | wc -l)"
    echo "Kernel Extensions (3rd party): $(ls /Library/Extensions/ 2>/dev/null | wc -l)"
    echo "Login Items: $(osascript -e 'tell application \"System Events\" to get the name of every login item' 2>/dev/null | wc -w)"

    echo ""
    echo "=== Suspicious Indicators ==="

    # Check for Launch Agents with network connections
    echo "Launch Agents with network keywords:"
    grep -r "socket" ~/Library/LaunchAgents/*.plist /Library/LaunchAgents/*.plist 2>/dev/null | cut -d: -f1 | sort -u

    # Check for hidden files
    echo ""
    echo "Hidden Launch Agents:"
    ls -la ~/Library/LaunchAgents/ /Library/LaunchAgents/ 2>/dev/null | grep '^\.'

    # Check for suspicious program arguments
    echo ""
    echo "Launch Agents with suspicious patterns:"
    grep -rE "curl|wget|bash|python|perl" ~/Library/LaunchAgents/*.plist /Library/LaunchAgents/*.plist 2>/dev/null | cut -d: -f1 | sort -u

    echo ""
    echo "=== Recommendations ==="
    echo "1. Review all Launch Agents and Daemons for unknown items"
    echo "2. Check kernel extensions for third-party kexts"
    echo "3. Verify login items are legitimate"
    echo "4. Review shell profiles for suspicious commands"
    echo "5. Check browser extensions"

} > "${OUTPUT_DIR}/persistence_summary.txt"

cat "${OUTPUT_DIR}/persistence_summary.txt"

# Create JSON summary
cat > "${OUTPUT_DIR}/summary.json" <<EOF
{
  "timestamp": "$(date -Iseconds)",
  "hostname": "$(hostname)",
  "collection_type": "persistence_check",
  "platform": "macos",
  "output_path": "${OUTPUT_DIR}",
  "statistics": {
    "user_launch_agents": $(ls ~/Library/LaunchAgents/*.plist 2>/dev/null | wc -l),
    "system_launch_agents": $(ls /Library/LaunchAgents/*.plist 2>/dev/null | wc -l),
    "launch_daemons": $(ls /Library/LaunchDaemons/*.plist 2>/dev/null | wc -l),
    "third_party_kexts": $(ls /Library/Extensions/ 2>/dev/null | wc -l)
  }
}
EOF

echo "[FRFD] macOS persistence check complete"
echo "[FRFD] Output: ${OUTPUT_DIR}"

# Create tarball
TARBALL="${OUTPUT_PATH}/macOS_Persistence_${TIMESTAMP}.tar.gz"
tar -czf "${TARBALL}" -C "$(dirname ${OUTPUT_DIR})" "$(basename ${OUTPUT_DIR})"
echo "[FRFD] Archive created: ${TARBALL}"

echo "${OUTPUT_DIR}"
