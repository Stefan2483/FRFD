# FRFD - Windows Process Memory Dump
# Collects process memory dumps for forensic analysis

param(
    [string]$OutputPath = "C:\CSIRT\Evidence\Memory",
    [switch]$AllProcesses = $false,
    [string[]]$ProcessNames = @()
)

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$outputDir = Join-Path $OutputPath "ProcessDumps_$timestamp"

# Create output directory
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

Write-Host "[FRFD] Starting process memory dump collection..."
Write-Host "[FRFD] Output directory: $outputDir"

# Requires administrative privileges
if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Warning "This script requires administrative privileges!"
    exit 1
}

# Get list of processes to dump
$processes = @()
if ($AllProcesses) {
    $processes = Get-Process | Where-Object { $_.Id -ne $PID }
} elseif ($ProcessNames.Count -gt 0) {
    foreach ($name in $ProcessNames) {
        $processes += Get-Process -Name $name -ErrorAction SilentlyContinue
    }
} else {
    # Default: dump interesting processes
    $interestingProcesses = @(
        "lsass", "services", "winlogon", "csrss", "smss",
        "explorer", "svchost", "chrome", "firefox", "iexplore",
        "powershell", "cmd", "wscript", "cscript"
    )
    foreach ($name in $interestingProcesses) {
        $processes += Get-Process -Name $name -ErrorAction SilentlyContinue
    }
}

$dumpCount = 0
$totalProcesses = $processes.Count
Write-Host "[FRFD] Found $totalProcesses processes to dump"

# Create metadata file
$metadata = @{
    Timestamp = (Get-Date).ToString("o")
    Hostname = $env:COMPUTERNAME
    Username = $env:USERNAME
    ProcessCount = $totalProcesses
    Processes = @()
}

foreach ($process in $processes) {
    try {
        $dumpFile = Join-Path $outputDir "$($process.Name)_$($process.Id).dmp"

        Write-Host "[FRFD] Dumping: $($process.Name) (PID: $($process.Id))"

        # Use rundll32 method (built-in Windows tool)
        $proc = $process.Id
        $null = & rundll32.exe comsvcs.dll MiniDump $proc $dumpFile full

        if (Test-Path $dumpFile) {
            $fileInfo = Get-Item $dumpFile
            $hash = (Get-FileHash -Path $dumpFile -Algorithm SHA256).Hash

            $metadata.Processes += @{
                Name = $process.Name
                PID = $process.Id
                Path = $process.Path
                DumpFile = $dumpFile
                Size = $fileInfo.Length
                Hash = $hash
            }

            $dumpCount++
            Write-Host "[FRFD] ✓ Dumped successfully: $($fileInfo.Length) bytes"
        }
    } catch {
        Write-Warning "[FRFD] Failed to dump $($process.Name): $_"
    }
}

# Save metadata
$metadataFile = Join-Path $outputDir "metadata.json"
$metadata | ConvertTo-Json -Depth 10 | Out-File -FilePath $metadataFile -Encoding UTF8

Write-Host "[FRFD] Process dump complete: $dumpCount/$totalProcesses processes dumped"
Write-Host "[FRFD] Evidence saved to: $outputDir"

# Return summary
return @{
    Success = $true
    DumpCount = $dumpCount
    TotalProcesses = $totalProcesses
    OutputPath = $outputDir
    MetadataFile = $metadataFile
}
