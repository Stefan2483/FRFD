#!/bin/bash
#
# upload_to_frfd.sh - Upload forensic artifacts to FRFD dongle via WiFi
#
# Usage:
#   upload_to_frfd <file_path> [artifact_type] [source_path] [frfd_ip]
#
# Parameters:
#   file_path      - Path to the file to upload (required)
#   artifact_type  - Type of artifact: memory, registry, logs, network, etc. (default: unknown)
#   source_path    - Original source path on target system (default: empty)
#   frfd_ip        - IP address of FRFD dongle (default: 192.168.4.1)
#

upload_to_frfd() {
    local file_path="$1"
    local artifact_type="${2:-unknown}"
    local source_path="${3:-}"
    local frfd_ip="${4:-192.168.4.1}"

    # Verify file exists
    if [ ! -f "$file_path" ]; then
        echo "[FRFD] Error: File not found: $file_path" >&2
        return 1
    fi

    # Get file info
    local filename=$(basename "$file_path")
    local filesize=$(stat -f%z "$file_path" 2>/dev/null || stat -c%s "$file_path" 2>/dev/null)

    echo "[FRFD] Uploading: $filename ($filesize bytes)"
    echo "[FRFD] Type: $artifact_type"

    # Build upload URL
    local upload_url="http://$frfd_ip/upload"

    # Attempt upload with retry logic
    local max_retries=3
    local retry_count=0
    local success=0

    while [ $success -eq 0 ] && [ $retry_count -lt $max_retries ]; do
        # Use curl to upload with multipart form data
        response=$(curl -s -w "\n%{http_code}" \
            -X POST \
            -F "file=@$file_path" \
            -F "type=$artifact_type" \
            -F "source_path=$source_path" \
            --connect-timeout 10 \
            --max-time 60 \
            "$upload_url" 2>&1)

        # Extract HTTP status code (last line)
        http_code=$(echo "$response" | tail -n1)
        body=$(echo "$response" | head -n-1)

        if [ "$http_code" = "200" ]; then
            echo "[FRFD] Upload successful!"

            # Parse JSON response (if jq is available)
            if command -v jq >/dev/null 2>&1; then
                artifact_id=$(echo "$body" | jq -r '.artifact_id // "N/A"')
                speed=$(echo "$body" | jq -r '.speed_kbps // "N/A"')
                echo "[FRFD] Artifact ID: $artifact_id"
                echo "[FRFD] Speed: $speed KB/s"
            else
                echo "[FRFD] Response: $body"
            fi

            success=1
            return 0
        else
            retry_count=$((retry_count + 1))
            echo "[FRFD] Upload failed (attempt $retry_count/$max_retries): HTTP $http_code" >&2

            if [ $retry_count -lt $max_retries ]; then
                sleep 2
            fi
        fi
    done

    if [ $success -eq 0 ]; then
        echo "[FRFD] Upload failed after $max_retries attempts" >&2
        return 1
    fi
}

# Inline version for HID automation (compact, minimal output)
upload_to_frfd_inline() {
    local f="$1";local t="${2:-unknown}";local s="${3:-}";local ip="${4:-192.168.4.1}";[ ! -f "$f" ]&&return 1;for i in 1 2 3;do r=$(curl -s -w "\n%{http_code}" -X POST -F "file=@$f" -F "type=$t" -F "source_path=$s" --connect-timeout 10 --max-time 60 "http://$ip/upload" 2>&1);c=$(echo "$r"|tail -n1);[ "$c" = "200" ]&&return 0;sleep 2;done;return 1
}

# If script is executed directly (not sourced)
if [ "${BASH_SOURCE[0]}" = "${0}" ]; then
    if [ $# -lt 1 ]; then
        echo "Usage: $0 <file_path> [artifact_type] [source_path] [frfd_ip]"
        echo ""
        echo "Example:"
        echo "  $0 /tmp/evidence.tar.gz archive /tmp/frfd_collection 192.168.4.1"
        exit 1
    fi

    upload_to_frfd "$@"
fi
