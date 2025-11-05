# FRFD - Windows Event Log Collection
# Collects critical Windows event logs for forensic analysis

param(
    [string]$OutputPath = "C:\CSIRT\Evidence\EventLogs",
    [int]$MaxEvents = 10000,
    [int]$DaysBack = 7
)

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$outputDir = Join-Path $OutputPath "EventLogs_$timestamp"
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

Write-Host "[FRFD] Collecting Windows Event Logs..."
Write-Host "[FRFD] Max events per log: $MaxEvents"
Write-Host "[FRFD] Days back: $DaysBack"

$startDate = (Get-Date).AddDays(-$DaysBack)

# Critical event logs to collect
$eventLogs = @(
    @{Name="Security"; File="Security.evtx"},
    @{Name="System"; File="System.evtx"},
    @{Name="Application"; File="Application.evtx"},
    @{Name="Microsoft-Windows-Sysmon/Operational"; File="Sysmon.evtx"},
    @{Name="Microsoft-Windows-PowerShell/Operational"; File="PowerShell-Operational.evtx"},
    @{Name="Windows PowerShell"; File="PowerShell.evtx"},
    @{Name="Microsoft-Windows-TaskScheduler/Operational"; File="TaskScheduler.evtx"},
    @{Name="Microsoft-Windows-Windows Defender/Operational"; File="Defender.evtx"},
    @{Name="Microsoft-Windows-Windows Firewall With Advanced Security/Firewall"; File="Firewall.evtx"},
    @{Name="Microsoft-Windows-TerminalServices-LocalSessionManager/Operational"; File="RDP-LocalSession.evtx"},
    @{Name="Microsoft-Windows-TerminalServices-RemoteConnectionManager/Operational"; File="RDP-RemoteConnection.evtx"}
)

$summary = @{
    Timestamp = (Get-Date).ToString("o")
    Hostname = $env:COMPUTERNAME
    StartDate = $startDate.ToString("o")
    LogsCollected = @()
    CriticalEvents = @()
}

# Export raw event log files
$rawLogsDir = Join-Path $outputDir "RawLogs"
New-Item -ItemType Directory -Force -Path $rawLogsDir | Out-Null

foreach ($log in $eventLogs) {
    try {
        Write-Host "[FRFD] Processing: $($log.Name)"

        # Check if log exists
        $logExists = Get-WinEvent -ListLog $log.Name -ErrorAction SilentlyContinue
        if (-not $logExists) {
            Write-Host "[FRFD]   Log not found, skipping..."
            continue
        }

        # Export raw .evtx file
        $evtxFile = Join-Path $rawLogsDir $log.File
        try {
            wevtutil epl $log.Name $evtxFile
            Write-Host "[FRFD]   Exported raw log: $evtxFile"
        } catch {
            Write-Warning "[FRFD]   Failed to export raw log: $_"
        }

        # Get recent events
        $events = Get-WinEvent -LogName $log.Name -MaxEvents $MaxEvents -ErrorAction SilentlyContinue |
            Where-Object { $_.TimeCreated -ge $startDate }

        if ($events) {
            $eventCount = $events.Count
            Write-Host "[FRFD]   Found $eventCount events"

            # Export to CSV
            $csvFile = Join-Path $outputDir "$($log.File -replace '.evtx','.csv')"
            $events | Select-Object TimeCreated, Id, LevelDisplayName, ProviderName, Message |
                Export-Csv -Path $csvFile -NoTypeInformation -Encoding UTF8

            # Analyze for critical events
            $critical = $events | Where-Object { $_.Level -le 3 } | Select-Object -First 100

            $summary.LogsCollected += @{
                LogName = $log.Name
                EventCount = $eventCount
                CriticalCount = $critical.Count
                OutputFile = $csvFile
                RawFile = $evtxFile
            }

            foreach ($event in $critical) {
                $summary.CriticalEvents += @{
                    LogName = $log.Name
                    TimeCreated = $event.TimeCreated
                    EventID = $event.Id
                    Level = $event.LevelDisplayName
                    Provider = $event.ProviderName
                    Message = $event.Message
                }
            }
        } else {
            Write-Host "[FRFD]   No events found in date range"
        }

    } catch {
        Write-Warning "[FRFD] Failed to collect $($log.Name): $_"
    }
}

# Look for specific suspicious Event IDs
Write-Host "[FRFD] Searching for suspicious event IDs..."

$suspiciousEvents = @(
    @{Log="Security"; ID=4624; Description="Successful Logon"},
    @{Log="Security"; ID=4625; Description="Failed Logon"},
    @{Log="Security"; ID=4648; Description="Logon with Explicit Credentials"},
    @{Log="Security"; ID=4672; Description="Special Privileges Assigned"},
    @{Log="Security"; ID=4720; Description="User Account Created"},
    @{Log="Security"; ID=4728; Description="Member Added to Security Group"},
    @{Log="Security"; ID=4732; Description="Member Added to Local Group"},
    @{Log="Security"; ID=4756; Description="Member Added to Universal Group"},
    @{Log="System"; ID=7045; Description="New Service Installed"},
    @{Log="System"; ID=7036; Description="Service State Change"},
    @{Log="Microsoft-Windows-Sysmon/Operational"; ID=1; Description="Process Creation"},
    @{Log="Microsoft-Windows-Sysmon/Operational"; ID=3; Description="Network Connection"},
    @{Log="Microsoft-Windows-Sysmon/Operational"; ID=11; Description="File Created"}
)

$suspiciousFindings = @()
foreach ($event in $suspiciousEvents) {
    try {
        $found = Get-WinEvent -FilterHashtable @{
            LogName=$event.Log;
            ID=$event.ID;
            StartTime=$startDate
        } -MaxEvents 1000 -ErrorAction SilentlyContinue

        if ($found) {
            Write-Host "[FRFD]   Found $($found.Count) events: $($event.Description) (ID: $($event.ID))"
            $suspiciousFindings += @{
                EventID = $event.ID
                Description = $event.Description
                Count = $found.Count
                RecentEvents = $found | Select-Object -First 10
            }
        }
    } catch {
        # Event log may not exist
    }
}

$summary.SuspiciousEvents = $suspiciousFindings

# Export summary
$summaryFile = Join-Path $outputDir "event_log_summary.json"
$summary | ConvertTo-Json -Depth 10 | Out-File -FilePath $summaryFile -Encoding UTF8

$htmlReport = Join-Path $outputDir "event_log_report.html"
$html = @"
<!DOCTYPE html>
<html>
<head>
    <title>FRFD Event Log Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        h1 { color: #2c3e50; }
        table { border-collapse: collapse; width: 100%; margin: 20px 0; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #3498db; color: white; }
        .critical { background-color: #e74c3c; color: white; }
        .warning { background-color: #f39c12; }
    </style>
</head>
<body>
    <h1>FRFD Event Log Analysis Report</h1>
    <p><strong>Hostname:</strong> $($env:COMPUTERNAME)</p>
    <p><strong>Collection Time:</strong> $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")</p>
    <p><strong>Date Range:</strong> $($startDate.ToString("yyyy-MM-dd")) to $(Get-Date -Format "yyyy-MM-dd")</p>

    <h2>Logs Collected</h2>
    <table>
        <tr><th>Log Name</th><th>Events</th><th>Critical</th><th>File</th></tr>
"@

foreach ($log in $summary.LogsCollected) {
    $html += "<tr><td>$($log.LogName)</td><td>$($log.EventCount)</td><td>$($log.CriticalCount)</td><td>$($log.OutputFile)</td></tr>"
}

$html += @"
    </table>

    <h2>Critical Events Summary</h2>
    <p>Total Critical Events: $($summary.CriticalEvents.Count)</p>
</body>
</html>
"@

$html | Out-File -FilePath $htmlReport -Encoding UTF8

Write-Host "[FRFD] Event log collection complete"
Write-Host "[FRFD] Logs collected: $($summary.LogsCollected.Count)"
Write-Host "[FRFD] Critical events: $($summary.CriticalEvents.Count)"
Write-Host "[FRFD] Output: $outputDir"
Write-Host "[FRFD] HTML Report: $htmlReport"

return @{
    Success = $true
    LogsCollected = $summary.LogsCollected.Count
    CriticalEvents = $summary.CriticalEvents.Count
    OutputPath = $outputDir
    SummaryFile = $summaryFile
    HTMLReport = $htmlReport
}
