#!/bin/bash
# FRFD - Linux Kernel Modules Collection
# Collects information about loaded kernel modules and LKM rootkit detection

OUTPUT_PATH="${1:-/tmp/csirt/evidence}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="${OUTPUT_PATH}/KernelModules_${TIMESTAMP}"

mkdir -p "${OUTPUT_DIR}"

echo "[FRFD] Collecting Linux kernel modules..."
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

# List all loaded modules
collect "lsmod" "lsmod"
collect "lsmod_sorted" "lsmod | sort"

# Module information from /proc
collect "proc_modules" "cat /proc/modules"

# Detailed module information
echo "[FRFD] Collecting detailed module information..."
MODULES_DIR="${OUTPUT_DIR}/module_details"
mkdir -p "${MODULES_DIR}"

lsmod | tail -n +2 | awk '{print $1}' | while read -r module; do
    echo "[FRFD]   Module: ${module}"

    # Module info
    modinfo "${module}" > "${MODULES_DIR}/${module}_info.txt" 2>&1

    # Module parameters
    if [ -d "/sys/module/${module}/parameters" ]; then
        {
            echo "=== Parameters for ${module} ==="
            for param in /sys/module/${module}/parameters/*; do
                if [ -f "${param}" ]; then
                    echo "$(basename ${param}): $(cat ${param} 2>/dev/null)"
                fi
            done
        } > "${MODULES_DIR}/${module}_parameters.txt"
    fi
done

# Module dependencies
collect "module_deps" "cat /lib/modules/$(uname -r)/modules.dep"

# Kernel module signing info
collect "module_signing" "cat /proc/sys/kernel/modules_disabled 2>/dev/null || echo 'Kernel module loading is ENABLED'"

# Check for module signing enforcement
if [ -f "/proc/sys/kernel/module_sig_enforce" ]; then
    collect "module_sig_enforce" "cat /proc/sys/kernel/module_sig_enforce"
fi

# Suspicious module detection
echo "[FRFD] Analyzing for suspicious modules..."

{
    echo "=== FRFD Suspicious Module Analysis ==="
    echo "Generated: $(date -Iseconds)"
    echo ""

    # Check for modules without proper signatures
    echo "=== Unsigned/Unverified Modules ==="
    lsmod | tail -n +2 | awk '{print $1}' | while read -r module; do
        modinfo "${module}" 2>/dev/null | grep -q "^sig_id:" || echo "  - ${module} (no signature info)"
    done

    echo ""
    echo "=== Modules from Unusual Locations ==="
    lsmod | tail -n +2 | awk '{print $1}' | while read -r module; do
        filename=$(modinfo -n "${module}" 2>/dev/null)
        if [ -n "${filename}" ]; then
            # Check if not in standard kernel module path
            if ! echo "${filename}" | grep -q "^/lib/modules"; then
                echo "  - ${module}: ${filename}"
            fi
        fi
    done

    echo ""
    echo "=== Recently Loaded Modules (last 24h) ==="
    lsmod | tail -n +2 | awk '{print $1}' | while read -r module; do
        filename=$(modinfo -n "${module}" 2>/dev/null)
        if [ -n "${filename}" ] && [ -f "${filename}" ]; then
            # Check file modification time
            if find "${filename}" -mtime -1 2>/dev/null | grep -q .; then
                echo "  - ${module}: ${filename} (modified in last 24h)"
            fi
        fi
    done

    echo ""
    echo "=== Modules with Network Capabilities ==="
    lsmod | tail -n +2 | awk '{print $1}' | while read -r module; do
        if modinfo "${module}" 2>/dev/null | grep -iq "network\|net\|tcp\|udp\|socket"; then
            echo "  - ${module}"
        fi
    done

    echo ""
    echo "=== Hidden Modules Check ==="
    echo "Comparing lsmod vs /proc/modules vs /sys/module..."

    # Get module lists
    lsmod | tail -n +2 | awk '{print $1}' | sort > /tmp/lsmod_list.$$
    cat /proc/modules | awk '{print $1}' | sort > /tmp/proc_modules_list.$$
    ls /sys/module 2>/dev/null | sort > /tmp/sys_modules_list.$$

    # Compare
    DIFF1=$(comm -23 /tmp/proc_modules_list.$$ /tmp/lsmod_list.$$)
    DIFF2=$(comm -23 /tmp/sys_modules_list.$$ /tmp/lsmod_list.$$)

    if [ -n "${DIFF1}" ]; then
        echo "Modules in /proc/modules but not in lsmod:"
        echo "${DIFF1}"
    else
        echo "No discrepancies found between /proc/modules and lsmod"
    fi

    if [ -n "${DIFF2}" ]; then
        echo "Modules in /sys/module but not in lsmod:"
        echo "${DIFF2}"
    fi

    # Cleanup temp files
    rm -f /tmp/lsmod_list.$$ /tmp/proc_modules_list.$$ /tmp/sys_modules_list.$$

    echo ""
    echo "=== Common Rootkit Module Names ==="
    ROOTKIT_PATTERNS="diamorphine|reptile|suterusu|kovid|rkduck|adore|knark|rtkit"

    if lsmod | grep -iE "${ROOTKIT_PATTERNS}"; then
        echo "⚠️  SUSPICIOUS: Potential rootkit module name detected!"
    else
        echo "No known rootkit module names detected"
    fi

} > "${OUTPUT_DIR}/suspicious_analysis.txt"

# Module blacklist
if [ -f "/etc/modprobe.d/blacklist.conf" ]; then
    collect "blacklist" "cat /etc/modprobe.d/blacklist.conf"
fi

collect "modprobe_all" "cat /etc/modprobe.d/*"

# Kernel configuration related to modules
if [ -f "/boot/config-$(uname -r)" ]; then
    collect "kernel_config_modules" "grep 'MODULE' /boot/config-$(uname -r)"
fi

# Check for known rootkit signatures
echo "[FRFD] Checking for rootkit signatures..."

{
    echo "=== Rootkit Signature Check ==="

    # Check for LKM rootkit artifacts
    if [ -d "/dev/shm" ]; then
        echo "Checking /dev/shm for suspicious files..."
        find /dev/shm -type f -name "*.ko" 2>/dev/null
    fi

    if [ -d "/tmp" ]; then
        echo "Checking /tmp for kernel modules..."
        find /tmp -type f -name "*.ko" 2>/dev/null
    fi

    # Check for suspicious proc entries
    echo "Checking for suspicious /proc entries..."
    ls -la /proc/ | grep -E "^d.*[0-9]{10,}" || echo "No suspicious proc entries found"

} > "${OUTPUT_DIR}/rootkit_check.txt"

# System call table analysis (requires root)
if [ "$EUID" -eq 0 ]; then
    collect "kallsyms_syscall" "grep sys_call_table /proc/kallsyms"
    collect "kallsyms_suspicious" "grep -E 'hidden|rootkit|backdoor' /proc/kallsyms"
fi

# Create JSON summary
cat > "${OUTPUT_DIR}/summary.json" <<EOF
{
  "timestamp": "$(date -Iseconds)",
  "hostname": "$(hostname -f)",
  "kernel_version": "$(uname -r)",
  "collection_type": "kernel_modules",
  "output_path": "${OUTPUT_DIR}",
  "statistics": {
    "loaded_modules": $(lsmod | tail -n +2 | wc -l),
    "module_files_collected": $(ls -1 "${MODULES_DIR}" 2>/dev/null | wc -l)
  }
}
EOF

echo "[FRFD] Kernel module collection complete"
echo "[FRFD] Loaded modules: $(lsmod | tail -n +2 | wc -l)"
echo "[FRFD] Output: ${OUTPUT_DIR}"

# Create tarball
TARBALL="${OUTPUT_PATH}/KernelModules_${TIMESTAMP}.tar.gz"
tar -czf "${TARBALL}" -C "$(dirname ${OUTPUT_DIR})" "$(basename ${OUTPUT_DIR})" 2>/dev/null
echo "[FRFD] Archive created: ${TARBALL}"

echo "${OUTPUT_DIR}"
