# FRFD - Windows Prefetch Analysis
# Collects and analyzes Windows Prefetch files

param(
    [string]$OutputPath = "C:\CSIRT\Evidence\Prefetch"
)

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$outputDir = Join-Path $OutputPath "Prefetch_$timestamp"
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

Write-Host "[FRFD] Collecting Windows Prefetch files..."

# Prefetch directory
$prefetchDir = "C:\Windows\Prefetch"

if (-not (Test-Path $prefetchDir)) {
    Write-Warning "[FRFD] Prefetch directory not found"
    return @{Success = $false; Reason = "Prefetch not enabled"}
}

# Check if prefetch is enabled
$prefetchEnabled = (Get-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management\PrefetchParameters" -Name "EnablePrefetcher" -ErrorAction SilentlyContinue).EnablePrefetcher

Write-Host "[FRFD] Prefetch Status: $(if($prefetchEnabled){$prefetchEnabled}else{'Unknown'})"

# Copy all prefetch files
$prefetchFiles = Get-ChildItem -Path $prefetchDir -Filter "*.pf" -ErrorAction SilentlyContinue

if (-not $prefetchFiles) {
    Write-Warning "[FRFD] No prefetch files found"
    return @{Success = $false; Reason = "No prefetch files"}
}

Write-Host "[FRFD] Found $($prefetchFiles.Count) prefetch files"

# Create subdirectory for raw files
$rawDir = Join-Path $outputDir "raw"
New-Item -ItemType Directory -Force -Path $rawDir | Out-Null

$results = @()

foreach ($file in $prefetchFiles) {
    try {
        # Copy prefetch file
        Copy-Item -Path $file.FullName -Destination $rawDir -ErrorAction Stop

        # Extract metadata
        $fileInfo = @{
            FileName = $file.Name
            ExecutableName = ($file.Name -replace '\.pf$','') -replace '-[A-F0-9]{8}$',''
            Size = $file.Length
            Created = $file.CreationTime
            Modified = $file.LastWriteTime
            Accessed = $file.LastAccessTime
            Hash = (Get-FileHash -Path $file.FullName -Algorithm SHA256).Hash
        }

        # Extract execution count from filename hash
        if ($file.Name -match '-([A-F0-9]{8})\.pf$') {
            $fileInfo.Hash = $matches[1]
        }

        $results += [PSCustomObject]$fileInfo

        Write-Host "[FRFD]   Collected: $($file.Name)"

    } catch {
        Write-Warning "[FRFD]   Failed to process $($file.Name): $_"
    }
}

# Sort by last modified (most recent first)
$results = $results | Sort-Object Modified -Descending

# Export results
$csvFile = Join-Path $outputDir "prefetch_analysis.csv"
$jsonFile = Join-Path $outputDir "prefetch_analysis.json"
$htmlFile = Join-Path $outputDir "prefetch_report.html"

$results | Export-Csv -Path $csvFile -NoTypeInformation -Encoding UTF8
$results | ConvertTo-Json -Depth 10 | Out-File -FilePath $jsonFile -Encoding UTF8

# Create HTML report
$html = @"
<!DOCTYPE html>
<html>
<head>
    <title>FRFD Prefetch Analysis</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        h1 { color: #2c3e50; }
        table { border-collapse: collapse; width: 100%; margin: 20px 0; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #3498db; color: white; }
        tr:nth-child(even) { background-color: #f2f2f2; }
        .recent { background-color: #ffffcc; }
        .stats { background-color: #e8f4f8; padding: 10px; margin: 10px 0; }
    </style>
</head>
<body>
    <h1>FRFD Prefetch Analysis Report</h1>
    <div class="stats">
        <p><strong>System:</strong> $($env:COMPUTERNAME)</p>
        <p><strong>Collection Time:</strong> $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")</p>
        <p><strong>Total Prefetch Files:</strong> $($results.Count)</p>
        <p><strong>Prefetch Status:</strong> $(if($prefetchEnabled){"Enabled ($prefetchEnabled)"}else{"Unknown"})</p>
    </div>

    <h2>Recently Executed Programs (Last 7 Days)</h2>
    <table>
        <tr>
            <th>Executable</th>
            <th>File Name</th>
            <th>Last Modified</th>
            <th>Created</th>
            <th>Size (bytes)</th>
        </tr>
"@

$recent = $results | Where-Object { $_.Modified -ge (Get-Date).AddDays(-7) }
foreach ($item in $recent) {
    $html += @"
        <tr class="recent">
            <td>$($item.ExecutableName)</td>
            <td>$($item.FileName)</td>
            <td>$($item.Modified.ToString("yyyy-MM-dd HH:mm:ss"))</td>
            <td>$($item.Created.ToString("yyyy-MM-dd HH:mm:ss"))</td>
            <td>$($item.Size)</td>
        </tr>
"@
}

$html += @"
    </table>

    <h2>All Prefetch Files</h2>
    <table>
        <tr>
            <th>Executable</th>
            <th>File Name</th>
            <th>Last Modified</th>
            <th>Size (bytes)</th>
        </tr>
"@

foreach ($item in $results) {
    $html += @"
        <tr>
            <td>$($item.ExecutableName)</td>
            <td>$($item.FileName)</td>
            <td>$($item.Modified.ToString("yyyy-MM-dd HH:mm:ss"))</td>
            <td>$($item.Size)</td>
        </tr>
"@
}

$html += @"
    </table>

    <h2>Suspicious Indicators</h2>
    <ul>
"@

# Look for suspicious patterns
$suspicious = @()

# Executables from temp directories
$tempExec = $results | Where-Object { $_.ExecutableName -match 'TEMP|TMP|APPDATA' }
if ($tempExec) {
    $html += "<li><strong>Executables from Temp directories:</strong> $($tempExec.Count) found</li>"
    $suspicious += $tempExec
}

# Recently created and executed
$recentlyNew = $results | Where-Object {
    ($_.Modified -ge (Get-Date).AddDays(-1)) -and
    ($_.Created -ge (Get-Date).AddDays(-1))
}
if ($recentlyNew) {
    $html += "<li><strong>Recently created and executed (last 24h):</strong> $($recentlyNew.Count) found</li>"
    $suspicious += $recentlyNew
}

# Single character or very short names
$shortNames = $results | Where-Object { $_.ExecutableName.Length -le 3 }
if ($shortNames) {
    $html += "<li><strong>Very short executable names:</strong> $($shortNames.Count) found</li>"
    $suspicious += $shortNames
}

if ($suspicious.Count -eq 0) {
    $html += "<li>No obvious suspicious indicators found</li>"
}

$html += @"
    </ul>
</body>
</html>
"@

$html | Out-File -FilePath $htmlFile -Encoding UTF8

# Create archive of all prefetch files
$archiveFile = Join-Path $outputDir "prefetch_files.zip"
try {
    Compress-Archive -Path "$rawDir\*" -DestinationPath $archiveFile -ErrorAction Stop
    Write-Host "[FRFD] Created archive: $archiveFile"
} catch {
    Write-Warning "[FRFD] Failed to create archive: $_"
}

Write-Host "[FRFD] Prefetch analysis complete"
Write-Host "[FRFD] Files collected: $($results.Count)"
Write-Host "[FRFD] Recent executions (7d): $($recent.Count)"
Write-Host "[FRFD] Suspicious indicators: $($suspicious.Count)"
Write-Host "[FRFD] Output: $outputDir"
Write-Host "[FRFD] HTML Report: $htmlFile"

return @{
    Success = $true
    FilesCollected = $results.Count
    RecentExecutions = $recent.Count
    SuspiciousCount = $suspicious.Count
    OutputPath = $outputDir
    HTMLReport = $htmlFile
    Archive = $archiveFile
}
