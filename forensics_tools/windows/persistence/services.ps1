# FRFD - Windows Services Enumeration
# Collects all Windows services for persistence analysis

param(
    [string]$OutputPath = "C:\CSIRT\Evidence\Persistence"
)

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$outputDir = Join-Path $OutputPath "Services_$timestamp"
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

Write-Host "[FRFD] Collecting Windows Services..."

# Get all services with detailed info
$services = Get-WmiObject Win32_Service | Select-Object `
    Name, DisplayName, State, Status, StartMode, StartName, PathName, Description,
    ProcessId, ServiceType, ErrorControl, AcceptPause, AcceptStop, DesktopInteract

Write-Host "[FRFD] Found $($services.Count) services"

$results = @()
$suspiciousCount = 0

foreach ($service in $services) {
    $isSuspicious = $false
    $suspiciousReasons = @()

    # Analyze service for suspicious indicators
    $pathName = $service.PathName

    if ($pathName) {
        $pathLower = $pathName.ToLower()

        # Check for suspicious paths
        if ($pathLower -match '\\temp\\|\\appdata\\|\\users\\public\\|\\programdata\\') {
            $suspiciousReasons += "Runs from temp/user directory"
            $isSuspicious = $true
        }

        # Check for suspicious binaries
        if ($pathLower -match 'powershell|cmd\.exe|wscript|cscript|mshta|rundll32') {
            $suspiciousReasons += "Uses scripting/system binary"
            $isSuspicious = $true
        }

        # Check for suspicious arguments
        if ($pathLower -match 'hidden|bypass|encodedcommand|-enc|-e ') {
            $suspiciousReasons += "Suspicious arguments: hidden/bypass/encoded"
            $isSuspicious = $true
        }

        # Check for network indicators
        if ($pathLower -match 'http://|https://|\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}') {
            $suspiciousReasons += "Contains URL or IP address"
            $isSuspicious = $true
        }

        # Check for unusual file extensions
        if ($pathLower -match '\.(vbs|js|bat|ps1|hta)') {
            $suspiciousReasons += "Script file as service binary"
            $isSuspicious = $true
        }
    }

    # Check for suspicious service names
    if ($service.Name -match '^[a-z0-9]{8,}$' -and $service.Name.Length -lt 32) {
        $suspiciousReasons += "Random-looking service name"
        $isSuspicious = $true
    }

    # Auto-start services are higher risk
    if ($service.StartMode -eq 'Auto' -and $isSuspicious) {
        $suspiciousReasons += "Set to auto-start"
    }

    # Running as SYSTEM but from user directory
    if ($service.StartName -match 'System|LocalSystem' -and $pathLower -match '\\users\\|\\appdata\\') {
        $suspiciousReasons += "SYSTEM service from user directory"
        $isSuspicious = $true
    }

    if ($isSuspicious) {
        $suspiciousCount++
    }

    # Get additional info from registry
    $regPath = "HKLM:\SYSTEM\CurrentControlSet\Services\$($service.Name)"
    $imagePath = ""
    $description = $service.Description

    if (Test-Path $regPath) {
        try {
            $imagePath = (Get-ItemProperty -Path $regPath -Name "ImagePath" -ErrorAction SilentlyContinue).ImagePath
            if (-not $description) {
                $description = (Get-ItemProperty -Path $regPath -Name "Description" -ErrorAction SilentlyContinue).Description
            }
        } catch {}
    }

    # Get file info if binary exists
    $binaryPath = ""
    $fileVersion = ""
    $fileCompany = ""
    $fileSigned = $false

    if ($pathName) {
        # Extract actual binary path (remove arguments)
        $binaryPath = $pathName -replace '"','' -split ' ' | Select-Object -First 1

        if (Test-Path $binaryPath) {
            try {
                $fileInfo = Get-Item $binaryPath
                $versionInfo = $fileInfo.VersionInfo

                $fileVersion = $versionInfo.FileVersion
                $fileCompany = $versionInfo.CompanyName

                # Check digital signature
                $signature = Get-AuthenticodeSignature -FilePath $binaryPath -ErrorAction SilentlyContinue
                if ($signature -and $signature.Status -eq 'Valid') {
                    $fileSigned = $true
                } else {
                    if ($isSuspicious) {
                        $suspiciousReasons += "Binary not properly signed"
                    }
                }
            } catch {}
        } else {
            $suspiciousReasons += "Binary file not found"
            $isSuspicious = $true
        }
    }

    $serviceData = @{
        Name = $service.Name
        DisplayName = $service.DisplayName
        State = $service.State
        Status = $service.Status
        StartMode = $service.StartMode
        StartName = $service.StartName
        PathName = $pathName
        BinaryPath = $binaryPath
        Description = $description
        ProcessId = $service.ProcessId
        FileVersion = $fileVersion
        Company = $fileCompany
        DigitallySigned = $fileSigned
        ServiceType = $service.ServiceType
        IsSuspicious = $isSuspicious
        SuspiciousReasons = ($suspiciousReasons -join "; ")
    }

    $results += [PSCustomObject]$serviceData

    if ($isSuspicious) {
        Write-Host "[FRFD] ⚠️  SUSPICIOUS: $($service.Name)" -ForegroundColor Yellow
    }
}

# Sort: suspicious first, then by name
$results = $results | Sort-Object @{Expression="IsSuspicious";Descending=$true}, Name

# Export results
$csvFile = Join-Path $outputDir "services.csv"
$jsonFile = Join-Path $outputDir "services.json"
$suspiciousFile = Join-Path $outputDir "suspicious_services.txt"

$results | Export-Csv -Path $csvFile -NoTypeInformation -Encoding UTF8
$results | ConvertTo-Json -Depth 10 | Out-File -FilePath $jsonFile -Encoding UTF8

# Export suspicious services
$suspicious = $results | Where-Object { $_.IsSuspicious }
if ($suspicious) {
    $suspicious | Format-List | Out-File -FilePath $suspiciousFile -Encoding UTF8
}

# Create HTML report
$htmlFile = Join-Path $outputDir "services_report.html"

$html = @"
<!DOCTYPE html>
<html>
<head>
    <title>FRFD Services Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        h1 { color: #2c3e50; }
        table { border-collapse: collapse; width: 100%; margin: 20px 0; font-size: 11px; }
        th, td { border: 1px solid #ddd; padding: 6px; text-align: left; }
        th { background-color: #3498db; color: white; position: sticky; top: 0; }
        tr:nth-child(even) { background-color: #f2f2f2; }
        .suspicious { background-color: #ffe6e6; }
        .running { color: green; font-weight: bold; }
        .stopped { color: #999; }
        .stats { background-color: #e8f4f8; padding: 10px; margin: 10px 0; }
        .reason { color: #d9534f; font-size: 10px; }
    </style>
</head>
<body>
    <h1>FRFD Windows Services Analysis</h1>
    <div class="stats">
        <p><strong>System:</strong> $($env:COMPUTERNAME)</p>
        <p><strong>Collection Time:</strong> $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")</p>
        <p><strong>Total Services:</strong> $($results.Count)</p>
        <p><strong>Running:</strong> $($($results | Where-Object {$_.State -eq "Running"}).Count)</p>
        <p><strong>Auto-Start:</strong> $($($results | Where-Object {$_.StartMode -eq "Auto"}).Count)</p>
        <p><strong>Suspicious:</strong> <span style="color: red; font-weight: bold;">$suspiciousCount</span></p>
    </div>

    <h2>Suspicious Services</h2>
    <table>
        <tr>
            <th>Name</th>
            <th>Display Name</th>
            <th>State</th>
            <th>Start Mode</th>
            <th>Path</th>
            <th>User</th>
            <th>Signed</th>
            <th>Reasons</th>
        </tr>
"@

foreach ($svc in $suspicious) {
    $stateClass = if ($svc.State -eq "Running") { "running" } else { "stopped" }
    $html += @"
        <tr class="suspicious">
            <td>$($svc.Name)</td>
            <td>$($svc.DisplayName)</td>
            <td class="$stateClass">$($svc.State)</td>
            <td>$($svc.StartMode)</td>
            <td style="font-size: 9px;">$([System.Web.HttpUtility]::HtmlEncode($svc.PathName.Substring(0, [Math]::Min(80, $svc.PathName.Length))))</td>
            <td>$($svc.StartName)</td>
            <td>$(if($svc.DigitallySigned){"✓"}else{"✗"})</td>
            <td class="reason">$($svc.SuspiciousReasons)</td>
        </tr>
"@
}

if ($suspiciousCount -eq 0) {
    $html += "<tr><td colspan='8' style='text-align: center; color: green;'>No suspicious services detected</td></tr>"
}

$html += @"
    </table>

    <h2>Running Services</h2>
    <table>
        <tr>
            <th>Name</th>
            <th>Display Name</th>
            <th>Start Mode</th>
            <th>Path</th>
            <th>User</th>
            <th>PID</th>
            <th>Company</th>
        </tr>
"@

$running = $results | Where-Object { $_.State -eq "Running" }
foreach ($svc in $running) {
    $rowClass = if ($svc.IsSuspicious) { "suspicious" } else { "" }
    $html += @"
        <tr class="$rowClass">
            <td>$($svc.Name)</td>
            <td>$($svc.DisplayName)</td>
            <td>$($svc.StartMode)</td>
            <td style="font-size: 9px;">$([System.Web.HttpUtility]::HtmlEncode($svc.BinaryPath))</td>
            <td>$($svc.StartName)</td>
            <td>$($svc.ProcessId)</td>
            <td style="font-size: 10px;">$($svc.Company)</td>
        </tr>
"@
}

$html += @"
    </table>

    <h2>Statistics</h2>
    <ul>
        <li><strong>Total Services:</strong> $($results.Count)</li>
        <li><strong>Running:</strong> $($($results | Where-Object {$_.State -eq "Running"}).Count)</li>
        <li><strong>Stopped:</strong> $($($results | Where-Object {$_.State -eq "Stopped"}).Count)</li>
        <li><strong>Auto-Start:</strong> $($($results | Where-Object {$_.StartMode -eq "Auto"}).Count)</li>
        <li><strong>Manual Start:</strong> $($($results | Where-Object {$_.StartMode -eq "Manual"}).Count)</li>
        <li><strong>Disabled:</strong> $($($results | Where-Object {$_.StartMode -eq "Disabled"}).Count)</li>
        <li><strong>Digitally Signed:</strong> $($($results | Where-Object DigitallySigned).Count)</li>
        <li><strong>Unsigned:</strong> $($($results | Where-Object {-not $_.DigitallySigned}).Count)</li>
        <li><strong>Suspicious:</strong> <span style="color: red;">$suspiciousCount</span></li>
    </ul>
</body>
</html>
"@

$html | Out-File -FilePath $htmlFile -Encoding UTF8

Write-Host "[FRFD] Services collection complete"
Write-Host "[FRFD] Total services: $($results.Count)"
Write-Host "[FRFD] Running: $($($results | Where-Object {$_.State -eq "Running"}).Count)"
Write-Host "[FRFD] Suspicious: $suspiciousCount" -ForegroundColor $(if($suspiciousCount -gt 0){"Red"}else{"Green"})
Write-Host "[FRFD] Output: $outputDir"
Write-Host "[FRFD] HTML Report: $htmlFile"

return @{
    Success = $true
    TotalServices = $results.Count
    RunningServices = ($results | Where-Object {$_.State -eq "Running"}).Count
    SuspiciousServices = $suspiciousCount
    OutputPath = $outputDir
    HTMLReport = $htmlFile
}
