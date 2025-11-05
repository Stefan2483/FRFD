# FRFD - Windows Scheduled Tasks Collection
# Enumerates all scheduled tasks for persistence analysis

param(
    [string]$OutputPath = "C:\CSIRT\Evidence\Persistence"
)

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$outputDir = Join-Path $OutputPath "ScheduledTasks_$timestamp"
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

Write-Host "[FRFD] Collecting Windows Scheduled Tasks..."

# Get all scheduled tasks
try {
    $tasks = Get-ScheduledTask -ErrorAction Stop
    Write-Host "[FRFD] Found $($tasks.Count) scheduled tasks"
} catch {
    Write-Warning "[FRFD] Failed to enumerate tasks: $_"
    return @{Success = $false; Reason = $_}
}

$results = @()
$suspiciousCount = 0

foreach ($task in $tasks) {
    try {
        # Get task info
        $taskInfo = Get-ScheduledTaskInfo -TaskName $task.TaskName -TaskPath $task.TaskPath -ErrorAction SilentlyContinue

        $isSuspicious = $false
        $suspiciousReasons = @()

        # Analyze task properties for suspicious indicators
        $actions = $task.Actions
        foreach ($action in $actions) {
            # Check for suspicious execution patterns
            if ($action.Execute) {
                $exec = $action.Execute.ToLower()
                $args = if ($action.Arguments) { $action.Arguments.ToLower() } else { "" }

                # Suspicious patterns
                if ($exec -match 'powershell|cmd|wscript|cscript|mshta|rundll32') {
                    $suspiciousReasons += "Uses scripting/system binary: $exec"
                    $isSuspicious = $true
                }

                if ($args -match 'hidden|bypass|encodedcommand|-enc|-e |-w hidden') {
                    $suspiciousReasons += "Suspicious arguments: hidden/bypass/encoded"
                    $isSuspicious = $true
                }

                if ($exec -match '\\temp\\|\\appdata\\|\\users\\public\\') {
                    $suspiciousReasons += "Executes from temp/user directory"
                    $isSuspicious = $true
                }

                if ($args -match 'http://|https://|ftp://|\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}') {
                    $suspiciousReasons += "Contains URL or IP address"
                    $isSuspicious = $true
                }
            }
        }

        # Check principals
        if ($task.Principal.UserId -eq 'SYSTEM' -or $task.Principal.UserId -eq 'Administrators') {
            if ($isSuspicious) {
                $suspiciousReasons += "Runs with elevated privileges"
            }
        }

        # Check triggers
        $triggerInfo = @()
        foreach ($trigger in $task.Triggers) {
            $triggerInfo += "$($trigger.CimClass.CimClassName): $(if($trigger.Enabled){'Enabled'}else{'Disabled'})"

            # Suspicious: triggers at logon or boot
            if ($trigger.CimClass.CimClassName -match 'LogonTrigger|BootTrigger') {
                if ($isSuspicious) {
                    $suspiciousReasons += "Runs at logon/boot"
                }
            }
        }

        if ($isSuspicious) {
            $suspiciousCount++
        }

        $taskData = @{
            TaskName = $task.TaskName
            TaskPath = $task.TaskPath
            State = $task.State
            Enabled = $task.Settings.Enabled
            Author = $task.Author
            Description = $task.Description
            UserId = $task.Principal.UserId
            RunLevel = $task.Principal.RunLevel
            LastRunTime = $taskInfo.LastRunTime
            NextRunTime = $taskInfo.NextRunTime
            LastTaskResult = $taskInfo.LastTaskResult
            NumberOfMissedRuns = $taskInfo.NumberOfMissedRuns
            Actions = ($actions | ForEach-Object {
                if ($_.Execute) {
                    "$($_.Execute) $($_.Arguments)"
                } else {
                    $_.ToString()
                }
            }) -join "; "
            Triggers = $triggerInfo -join "; "
            IsSuspicious = $isSuspicious
            SuspiciousReasons = ($suspiciousReasons -join "; ")
        }

        $results += [PSCustomObject]$taskData

        if ($isSuspicious) {
            Write-Host "[FRFD] ⚠️  SUSPICIOUS: $($task.TaskPath)$($task.TaskName)" -ForegroundColor Yellow
        }

    } catch {
        Write-Warning "[FRFD] Failed to process task $($task.TaskName): $_"
    }
}

# Sort: suspicious first, then by path
$results = $results | Sort-Object @{Expression="IsSuspicious";Descending=$true}, TaskPath

# Export results
$csvFile = Join-Path $outputDir "scheduled_tasks.csv"
$jsonFile = Join-Path $outputDir "scheduled_tasks.json"
$suspiciousFile = Join-Path $outputDir "suspicious_tasks.txt"

$results | Export-Csv -Path $csvFile -NoTypeInformation -Encoding UTF8
$results | ConvertTo-Json -Depth 10 | Out-File -FilePath $jsonFile -Encoding UTF8

# Export suspicious tasks separately
$suspicious = $results | Where-Object { $_.IsSuspicious }
if ($suspicious) {
    $suspicious | Format-List | Out-File -FilePath $suspiciousFile -Encoding UTF8
}

# Export task XML files for suspicious tasks
$xmlDir = Join-Path $outputDir "task_xml"
New-Item -ItemType Directory -Force -Path $xmlDir | Out-Null

foreach ($task in $suspicious) {
    try {
        $xmlFile = Join-Path $xmlDir "$($task.TaskName -replace '[\\/:*?"<>|]','_').xml"
        $xml = Export-ScheduledTask -TaskName $task.TaskName -TaskPath $task.TaskPath
        $xml | Out-File -FilePath $xmlFile -Encoding UTF8
    } catch {
        Write-Warning "[FRFD] Failed to export XML for $($task.TaskName): $_"
    }
}

# Create HTML report
$htmlFile = Join-Path $outputDir "scheduled_tasks_report.html"

$html = @"
<!DOCTYPE html>
<html>
<head>
    <title>FRFD Scheduled Tasks Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        h1 { color: #2c3e50; }
        table { border-collapse: collapse; width: 100%; margin: 20px 0; font-size: 12px; }
        th, td { border: 1px solid #ddd; padding: 6px; text-align: left; }
        th { background-color: #3498db; color: white; position: sticky; top: 0; }
        tr:nth-child(even) { background-color: #f2f2f2; }
        .suspicious { background-color: #ffe6e6; }
        .disabled { color: #999; }
        .stats { background-color: #e8f4f8; padding: 10px; margin: 10px 0; }
        .reason { color: #d9534f; font-weight: bold; }
    </style>
</head>
<body>
    <h1>FRFD Scheduled Tasks Analysis</h1>
    <div class="stats">
        <p><strong>System:</strong> $($env:COMPUTERNAME)</p>
        <p><strong>Collection Time:</strong> $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")</p>
        <p><strong>Total Tasks:</strong> $($results.Count)</p>
        <p><strong>Enabled Tasks:</strong> $($($results | Where-Object Enabled).Count)</p>
        <p><strong>Suspicious Tasks:</strong> <span style="color: red; font-weight: bold;">$suspiciousCount</span></p>
    </div>

    <h2>Suspicious Tasks</h2>
    <table>
        <tr>
            <th>Task Name</th>
            <th>State</th>
            <th>User</th>
            <th>Actions</th>
            <th>Reasons</th>
            <th>Last Run</th>
        </tr>
"@

foreach ($task in $suspicious) {
    $stateClass = if (-not $task.Enabled) { "disabled" } else { "" }
    $html += @"
        <tr class="suspicious $stateClass">
            <td>$($task.TaskPath)$($task.TaskName)</td>
            <td>$($task.State)</td>
            <td>$($task.UserId)</td>
            <td style="font-size: 10px;">$($task.Actions)</td>
            <td class="reason">$($task.SuspiciousReasons)</td>
            <td>$(if($task.LastRunTime){$task.LastRunTime.ToString("yyyy-MM-dd HH:mm")}else{"Never"})</td>
        </tr>
"@
}

if ($suspiciousCount -eq 0) {
    $html += "<tr><td colspan='6' style='text-align: center; color: green;'>No suspicious tasks detected</td></tr>"
}

$html += @"
    </table>

    <h2>All Scheduled Tasks</h2>
    <table>
        <tr>
            <th>Task Name</th>
            <th>State</th>
            <th>Enabled</th>
            <th>User</th>
            <th>Actions</th>
            <th>Last Run</th>
            <th>Next Run</th>
        </tr>
"@

foreach ($task in $results) {
    $rowClass = if ($task.IsSuspicious) { "suspicious" } else { "" }
    $stateClass = if (-not $task.Enabled) { "disabled" } else { "" }

    $html += @"
        <tr class="$rowClass $stateClass">
            <td>$($task.TaskPath)$($task.TaskName)</td>
            <td>$($task.State)</td>
            <td>$(if($task.Enabled){"✓"}else{"-"})</td>
            <td>$($task.UserId)</td>
            <td style="font-size: 10px;">$([System.Web.HttpUtility]::HtmlEncode($task.Actions.Substring(0, [Math]::Min(100, $task.Actions.Length))))</td>
            <td style="font-size: 10px;">$(if($task.LastRunTime){$task.LastRunTime.ToString("yyyy-MM-dd HH:mm")}else{"-"})</td>
            <td style="font-size: 10px;">$(if($task.NextRunTime){$task.NextRunTime.ToString("yyyy-MM-dd HH:mm")}else{"-"})</td>
        </tr>
"@
}

$html += @"
    </table>

    <h2>Analysis Summary</h2>
    <ul>
        <li><strong>Total Tasks:</strong> $($results.Count)</li>
        <li><strong>Enabled:</strong> $($($results | Where-Object Enabled).Count)</li>
        <li><strong>Disabled:</strong> $($($results | Where-Object {-not $_.Enabled}).Count)</li>
        <li><strong>Running as SYSTEM:</strong> $($($results | Where-Object {$_.UserId -eq "SYSTEM"}).Count)</li>
        <li><strong>Ready State:</strong> $($($results | Where-Object {$_.State -eq "Ready"}).Count)</li>
        <li><strong>Suspicious:</strong> <span style="color: red;">$suspiciousCount</span></li>
    </ul>
</body>
</html>
"@

$html | Out-File -FilePath $htmlFile -Encoding UTF8

Write-Host "[FRFD] Scheduled tasks collection complete"
Write-Host "[FRFD] Total tasks: $($results.Count)"
Write-Host "[FRFD] Suspicious: $suspiciousCount" -ForegroundColor $(if($suspiciousCount -gt 0){"Red"}else{"Green"})
Write-Host "[FRFD] Output: $outputDir"
Write-Host "[FRFD] HTML Report: $htmlFile"

return @{
    Success = $true
    TotalTasks = $results.Count
    EnabledTasks = ($results | Where-Object Enabled).Count
    SuspiciousTasks = $suspiciousCount
    OutputPath = $outputDir
    HTMLReport = $htmlFile
}
