# FRFD - Windows Network Connections
# Collects current network connections and related artifacts

param(
    [string]$OutputPath = "C:\CSIRT\Evidence\Network"
)

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$outputDir = Join-Path $OutputPath "NetworkConnections_$timestamp"
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

Write-Host "[FRFD] Collecting network connection artifacts..."

# 1. Active TCP/UDP connections
Write-Host "[FRFD] Collecting active connections..."
$connections = Get-NetTCPConnection -ErrorAction SilentlyContinue | Select-Object `
    LocalAddress, LocalPort, RemoteAddress, RemotePort, State,
    OwningProcess, CreationTime,
    @{Name="ProcessName";Expression={(Get-Process -Id $_.OwningProcess -ErrorAction SilentlyContinue).Name}},
    @{Name="ProcessPath";Expression={(Get-Process -Id $_.OwningProcess -ErrorAction SilentlyContinue).Path}}

$udpConnections = Get-NetUDPEndpoint -ErrorAction SilentlyContinue | Select-Object `
    LocalAddress, LocalPort,
    OwningProcess, CreationTime,
    @{Name="ProcessName";Expression={(Get-Process -Id $_.OwningProcess -ErrorAction SilentlyContinue).Name}},
    @{Name="ProcessPath";Expression={(Get-Process -Id $_.OwningProcess -ErrorAction SilentlyContinue).Path}}

# 2. DNS Cache
Write-Host "[FRFD] Collecting DNS cache..."
$dnsCache = Get-DnsClientCache -ErrorAction SilentlyContinue | Select-Object `
    Entry, Name, Type, Status, Section, TimeToLive, DataLength, Data

# 3. ARP Cache
Write-Host "[FRFD] Collecting ARP cache..."
$arpCache = Get-NetNeighbor -ErrorAction SilentlyContinue | Select-Object `
    IPAddress, LinkLayerAddress, State, InterfaceAlias, InterfaceIndex

# 4. Routing Table
Write-Host "[FRFD] Collecting routing table..."
$routes = Get-NetRoute -ErrorAction SilentlyContinue | Select-Object `
    DestinationPrefix, NextHop, InterfaceAlias, RouteMetric, Protocol

# 5. Network Adapters
Write-Host "[FRFD] Collecting network adapter information..."
$adapters = Get-NetAdapter -ErrorAction SilentlyContinue | Select-Object `
    Name, InterfaceDescription, Status, MacAddress, LinkSpeed, MediaType

# 6. Firewall Rules (active)
Write-Host "[FRFD] Collecting active firewall rules..."
$firewallRules = Get-NetFirewallRule -Enabled True -ErrorAction SilentlyContinue | Select-Object `
    Name, DisplayName, Description, Direction, Action, Enabled, Profile | Select-Object -First 500

# 7. Network shares
Write-Host "[FRFD] Collecting network shares..."
$shares = Get-SmbShare -ErrorAction SilentlyContinue | Select-Object `
    Name, Path, Description, CurrentUsers

# 8. SMB Sessions
Write-Host "[FRFD] Collecting SMB sessions..."
$smbSessions = Get-SmbSession -ErrorAction SilentlyContinue | Select-Object `
    SessionId, ClientComputerName, ClientUserName, NumOpens, SecondsIdle

# Export all data
Write-Host "[FRFD] Exporting collected data..."

$connections | Export-Csv -Path (Join-Path $outputDir "tcp_connections.csv") -NoTypeInformation -Encoding UTF8
$udpConnections | Export-Csv -Path (Join-Path $outputDir "udp_endpoints.csv") -NoTypeInformation -Encoding UTF8
$dnsCache | Export-Csv -Path (Join-Path $outputDir "dns_cache.csv") -NoTypeInformation -Encoding UTF8
$arpCache | Export-Csv -Path (Join-Path $outputDir "arp_cache.csv") -NoTypeInformation -Encoding UTF8
$routes | Export-Csv -Path (Join-Path $outputDir "routing_table.csv") -NoTypeInformation -Encoding UTF8
$adapters | Export-Csv -Path (Join-Path $outputDir "network_adapters.csv") -NoTypeInformation -Encoding UTF8
$firewallRules | Export-Csv -Path (Join-Path $outputDir "firewall_rules.csv") -NoTypeInformation -Encoding UTF8
$shares | Export-Csv -Path (Join-Path $outputDir "network_shares.csv") -NoTypeInformation -Encoding UTF8
$smbSessions | Export-Csv -Path (Join-Path $outputDir "smb_sessions.csv") -NoTypeInformation -Encoding UTF8

# Create combined JSON report
$report = @{
    Timestamp = (Get-Date).ToString("o")
    Hostname = $env:COMPUTERNAME
    TCPConnections = $connections
    UDPEndpoints = $udpConnections
    DNSCache = $dnsCache
    ARPCache = $arpCache
    Routes = $routes
    Adapters = $adapters
    FirewallRules = $firewallRules | Select-Object -First 100
    Shares = $shares
    SMBSessions = $smbSessions
    Statistics = @{
        TCPConnectionCount = $connections.Count
        UDPEndpointCount = $udpConnections.Count
        DNSEntries = $dnsCache.Count
        ARPEntries = $arpCache.Count
        ActiveShares = $shares.Count
        ActiveSMBSessions = $smbSessions.Count
    }
}

$report | ConvertTo-Json -Depth 10 | Out-File -FilePath (Join-Path $outputDir "network_summary.json") -Encoding UTF8

# Also run netstat for legacy format
netstat -anob > (Join-Path $outputDir "netstat_output.txt") 2>&1

Write-Host "[FRFD] Network collection complete"
Write-Host "[FRFD] TCP Connections: $($connections.Count)"
Write-Host "[FRFD] UDP Endpoints: $($udpConnections.Count)"
Write-Host "[FRFD] DNS Entries: $($dnsCache.Count)"
Write-Host "[FRFD] Output: $outputDir"

return @{
    Success = $true
    OutputPath = $outputDir
    Statistics = $report.Statistics
}
