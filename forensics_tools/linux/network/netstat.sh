#!/bin/bash
# FRFD - Linux Network Connections and State
# Collects network configuration and active connections

OUTPUT_PATH="${1:-/tmp/csirt/evidence}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="${OUTPUT_PATH}/Network_${TIMESTAMP}"

mkdir -p "${OUTPUT_DIR}"

echo "[FRFD] Collecting Linux network information..."
echo "[FRFD] Output directory: ${OUTPUT_DIR}"

# Network Interfaces
echo "[FRFD] Collecting network interfaces..."
ip addr show > "${OUTPUT_DIR}/ip_addr.txt"
ip link show > "${OUTPUT_DIR}/ip_link.txt"
ifconfig -a > "${OUTPUT_DIR}/ifconfig.txt" 2>/dev/null

# Routing
echo "[FRFD] Collecting routing information..."
ip route show > "${OUTPUT_DIR}/ip_route.txt"
route -n > "${OUTPUT_DIR}/route.txt" 2>/dev/null
netstat -rn > "${OUTPUT_DIR}/netstat_route.txt" 2>/dev/null

# Active Connections
echo "[FRFD] Collecting active connections..."
ss -tunap > "${OUTPUT_DIR}/ss_all.txt"
ss -tulpn > "${OUTPUT_DIR}/ss_listening.txt"
netstat -tunap > "${OUTPUT_DIR}/netstat_all.txt" 2>/dev/null
netstat -tulpn > "${OUTPUT_DIR}/netstat_listening.txt" 2>/dev/null

# ARP Cache
echo "[FRFD] Collecting ARP cache..."
ip neigh show > "${OUTPUT_DIR}/ip_neigh.txt"
arp -an > "${OUTPUT_DIR}/arp.txt" 2>/dev/null

# Firewall Rules
echo "[FRFD] Collecting firewall rules..."
iptables -L -n -v > "${OUTPUT_DIR}/iptables_filter.txt" 2>/dev/null
iptables -t nat -L -n -v > "${OUTPUT_DIR}/iptables_nat.txt" 2>/dev/null
iptables -t mangle -L -n -v > "${OUTPUT_DIR}/iptables_mangle.txt" 2>/dev/null
iptables-save > "${OUTPUT_DIR}/iptables_save.txt" 2>/dev/null

ip6tables -L -n -v > "${OUTPUT_DIR}/ip6tables_filter.txt" 2>/dev/null
ip6tables-save > "${OUTPUT_DIR}/ip6tables_save.txt" 2>/dev/null

# UFW (if installed)
if command -v ufw &> /dev/null; then
    ufw status verbose > "${OUTPUT_DIR}/ufw_status.txt" 2>/dev/null
fi

# DNS
echo "[FRFD] Collecting DNS configuration..."
cat /etc/resolv.conf > "${OUTPUT_DIR}/resolv_conf.txt"
cat /etc/hosts > "${OUTPUT_DIR}/hosts.txt"
cat /etc/hostname > "${OUTPUT_DIR}/hostname.txt"

# Network Statistics
echo "[FRFD] Collecting network statistics..."
netstat -s > "${OUTPUT_DIR}/netstat_statistics.txt" 2>/dev/null
ss -s > "${OUTPUT_DIR}/ss_statistics.txt"

# Network Services
echo "[FRFD] Collecting network services..."
cat /etc/services > "${OUTPUT_DIR}/services.txt"

# Listening Ports Analysis
echo "[FRFD] Analyzing listening ports..."
{
    echo "=== FRFD Network Analysis ==="
    echo "Generated: $(date -Iseconds)"
    echo ""

    echo "Listening TCP Ports:"
    ss -tlnp | grep LISTEN

    echo ""
    echo "Listening UDP Ports:"
    ss -ulnp

    echo ""
    echo "Established Connections:"
    ss -tnp | grep ESTAB

    echo ""
    echo "Established Connections by Process:"
    ss -tnp | grep ESTAB | awk '{print $6}' | sort | uniq -c | sort -rn

} > "${OUTPUT_DIR}/network_analysis.txt"

# Process-to-Port Mapping
{
    echo "=== Process to Port Mapping ==="
    echo ""
    netstat -tulpn 2>/dev/null | grep LISTEN || ss -tulpn | grep LISTEN
} > "${OUTPUT_DIR}/process_port_mapping.txt"

# External Connections
{
    echo "=== External Connections ==="
    echo ""
    ss -tnp | grep ESTAB | awk '{print $5}' | cut -d: -f1 | grep -v "127.0.0.1" | grep -v "::1" | sort -u
} > "${OUTPUT_DIR}/external_connections.txt"

# Network Configuration Files
echo "[FRFD] Copying network configuration files..."
cp -r /etc/network/ "${OUTPUT_DIR}/etc_network/" 2>/dev/null
cp -r /etc/netplan/ "${OUTPUT_DIR}/etc_netplan/" 2>/dev/null
cp -r /etc/NetworkManager/ "${OUTPUT_DIR}/etc_NetworkManager/" 2>/dev/null

# WiFi Information (if available)
if command -v iwconfig &> /dev/null; then
    iwconfig > "${OUTPUT_DIR}/iwconfig.txt" 2>/dev/null
fi

if command -v nmcli &> /dev/null; then
    nmcli dev show > "${OUTPUT_DIR}/nmcli_devices.txt" 2>/dev/null
    nmcli con show > "${OUTPUT_DIR}/nmcli_connections.txt" 2>/dev/null
fi

# Generate JSON summary
LISTENING_PORTS=$(ss -tlnp | grep -c LISTEN)
ESTABLISHED=$(ss -tnp | grep -c ESTAB)
EXTERNAL_IPS=$(ss -tnp | grep ESTAB | awk '{print $5}' | cut -d: -f1 | grep -v "127.0.0.1" | grep -v "::1" | sort -u | wc -l)

cat > "${OUTPUT_DIR}/summary.json" <<EOF
{
  "timestamp": "$(date -Iseconds)",
  "hostname": "$(hostname -f)",
  "collection_type": "network",
  "output_path": "${OUTPUT_DIR}",
  "statistics": {
    "listening_ports": ${LISTENING_PORTS},
    "established_connections": ${ESTABLISHED},
    "unique_external_ips": ${EXTERNAL_IPS},
    "network_interfaces": $(ip link show | grep -c "^[0-9]")
  },
  "interfaces": $(ip -j addr show 2>/dev/null || echo "[]")
}
EOF

echo "[FRFD] Network collection complete"
echo "[FRFD] Listening ports: ${LISTENING_PORTS}"
echo "[FRFD] Established connections: ${ESTABLISHED}"
echo "[FRFD] External IPs: ${EXTERNAL_IPS}"
echo "[FRFD] Output: ${OUTPUT_DIR}"

# Create tarball
TARBALL="${OUTPUT_PATH}/Network_${TIMESTAMP}.tar.gz"
tar -czf "${TARBALL}" -C "$(dirname ${OUTPUT_DIR})" "$(basename ${OUTPUT_DIR})"
echo "[FRFD] Archive created: ${TARBALL}"

echo "${OUTPUT_DIR}"
