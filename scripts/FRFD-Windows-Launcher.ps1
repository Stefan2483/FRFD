#Requires -Version 5.1
#Requires -RunAsAdministrator

<#
.SYNOPSIS
    FRFD - First Responder Forensics Dongle - Windows Launcher
.DESCRIPTION
    Main launcher script for FRFD on Windows systems.
    Executes forensic collection scripts based on mode.
.PARAMETER Mode
    Operating mode: Triage, Collect, Contain, Analyze
.PARAMETER OutputPath
    Path where evidence will be stored
.PARAMETER CaseId
    Case/Incident identifier
.PARAMETER Responder
    Name of the responder
#>

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet('Triage', 'Collect', 'Contain', 'Analyze')]
    [string]$Mode = 'Triage',

    [Parameter(Mandatory=$false)]
    [string]$OutputPath = 'C:\CSIRT\Evidence',

    [Parameter(Mandatory=$false)]
    [string]$CaseId = '',

    [Parameter(Mandatory=$false)]
    [string]$Responder = $env:USERNAME
)

# FRFD Configuration
$ErrorActionPreference = 'Continue'
$VerbosePreference = 'Continue'

$Script:FRFDVersion = '0.1.0'
$Script:StartTime = Get-Date
$Script:ScriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
$Script:ForensicsPath = Join-Path (Split-Path -Parent $ScriptPath) 'forensics_tools\windows'

# Colors for output
function Write-FRFDLog {
    param(
        [string]$Message,
        [ValidateSet('Info', 'Success', 'Warning', 'Error')]
        [string]$Level = 'Info'
    )

    $timestamp = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
    $color = switch ($Level) {
        'Info'    { 'Cyan' }
        'Success' { 'Green' }
        'Warning' { 'Yellow' }
        'Error'   { 'Red' }
    }

    Write-Host "[$timestamp] " -NoNewline
    Write-Host "[FRFD] " -ForegroundColor Magenta -NoNewline
    Write-Host $Message -ForegroundColor $color
}

function Show-Banner {
    Write-Host @"

    ███████╗██████╗ ███████╗██████╗
    ██╔════╝██╔══██╗██╔════╝██╔══██╗
    █████╗  ██████╔╝█████╗  ██║  ██║
    ██╔══╝  ██╔══██╗██╔══╝  ██║  ██║
    ██║     ██║  ██║██║     ██████╔╝
    ╚═╝     ╚═╝  ╚═╝╚═╝     ╚═════╝

    First Responder Forensics Dongle
    Version: $FRFDVersion
    Mode: $Mode

"@ -ForegroundColor Cyan

    Write-FRFDLog "CSIRT Forensics Automation Tool" -Level Info
    Write-FRFDLog "=" * 60 -Level Info
}

function Test-Administrator {
    $currentUser = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($currentUser)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Initialize-FRFD {
    Write-FRFDLog "Initializing FRFD..." -Level Info

    # Check administrator privileges
    if (-not (Test-Administrator)) {
        Write-FRFDLog "This script requires Administrator privileges!" -Level Error
        exit 1
    }

    # Create output directory
    if (-not (Test-Path $OutputPath)) {
        New-Item -ItemType Directory -Force -Path $OutputPath | Out-Null
        Write-FRFDLog "Created output directory: $OutputPath" -Level Success
    }

    # Generate Case ID if not provided
    if ([string]::IsNullOrEmpty($CaseId)) {
        $Script:CaseId = "INC-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
    }

    Write-FRFDLog "Case ID: $CaseId" -Level Info
    Write-FRFDLog "Responder: $Responder" -Level Info
    Write-FRFDLog "Output Path: $OutputPath" -Level Info
}

function Invoke-TriageCollection {
    Write-FRFDLog "Starting Triage Mode..." -Level Info

    $results = @()

    # Quick system assessment
    Write-FRFDLog "Performing quick system assessment..." -Level Info

    # Collect basic system info
    $systemInfo = @{
        Hostname = $env:COMPUTERNAME
        OS = (Get-WmiObject Win32_OperatingSystem).Caption
        Version = [System.Environment]::OSVersion.Version.ToString()
        Architecture = $env:PROCESSOR_ARCHITECTURE
        Domain = $env:USERDOMAIN
        CurrentUser = $env:USERNAME
        Timestamp = (Get-Date).ToString('o')
    }

    # Check for quick indicators
    Write-FRFDLog "Checking for suspicious indicators..." -Level Info

    # Network connections
    $suspiciousConnections = Get-NetTCPConnection -State Established |
        Where-Object { $_.RemoteAddress -notlike '127.*' -and $_.RemoteAddress -notlike '::1' } |
        Select-Object -First 20

    # Running processes
    $processes = Get-Process | Select-Object Name, Id, Path, Company, StartTime | Select-Object -First 50

    $results += $systemInfo
    $results += @{
        Type = 'NetworkConnections'
        Count = $suspiciousConnections.Count
        Sample = $suspiciousConnections
    }
    $results += @{
        Type = 'RunningProcesses'
        Count = $processes.Count
        Sample = $processes
    }

    # Save triage results
    $triageFile = Join-Path $OutputPath "triage_$(Get-Date -Format 'yyyyMMdd_HHmmss').json"
    $results | ConvertTo-Json -Depth 10 | Out-File -FilePath $triageFile -Encoding UTF8

    Write-FRFDLog "Triage complete: $triageFile" -Level Success
    return $results
}

function Invoke-FullCollection {
    Write-FRFDLog "Starting Full Collection Mode..." -Level Info

    $collectionResults = @()

    # Memory Collection
    Write-FRFDLog "Collecting memory artifacts..." -Level Info
    $memoryScript = Join-Path $ForensicsPath "memory\process_dump.ps1"
    if (Test-Path $memoryScript) {
        try {
            $result = & $memoryScript -OutputPath $OutputPath
            $collectionResults += @{Category = 'Memory'; Result = $result}
        } catch {
            Write-FRFDLog "Memory collection failed: $_" -Level Warning
        }
    }

    # Registry Collection
    Write-FRFDLog "Collecting registry artifacts..." -Level Info
    $registryScript = Join-Path $ForensicsPath "registry\autoruns.ps1"
    if (Test-Path $registryScript) {
        try {
            $result = & $registryScript -OutputPath $OutputPath
            $collectionResults += @{Category = 'Registry'; Result = $result}
        } catch {
            Write-FRFDLog "Registry collection failed: $_" -Level Warning
        }
    }

    # Network Collection
    Write-FRFDLog "Collecting network artifacts..." -Level Info
    $networkScript = Join-Path $ForensicsPath "network\connections.ps1"
    if (Test-Path $networkScript) {
        try {
            $result = & $networkScript -OutputPath $OutputPath
            $collectionResults += @{Category = 'Network'; Result = $result}
        } catch {
            Write-FRFDLog "Network collection failed: $_" -Level Warning
        }
    }

    # Event Logs
    Write-FRFDLog "Collecting event logs..." -Level Info
    $eventLogScript = Join-Path $ForensicsPath "filesystem\event_logs.ps1"
    if (Test-Path $eventLogScript) {
        try {
            $result = & $eventLogScript -OutputPath $OutputPath -MaxEvents 10000 -DaysBack 7
            $collectionResults += @{Category = 'EventLogs'; Result = $result}
        } catch {
            Write-FRFDLog "Event log collection failed: $_" -Level Warning
        }
    }

    Write-FRFDLog "Full collection complete!" -Level Success
    return $collectionResults
}

function Invoke-ContainmentProcedure {
    Write-FRFDLog "Starting Containment Mode..." -Level Warning
    Write-FRFDLog "WARNING: This will implement network isolation and security controls!" -Level Warning

    # Confirm containment
    $confirm = Read-Host "Type 'CONTAIN' to proceed with containment actions"
    if ($confirm -ne 'CONTAIN') {
        Write-FRFDLog "Containment cancelled by user" -Level Info
        return
    }

    # Network Isolation
    Write-FRFDLog "Implementing network isolation..." -Level Info

    # Block all outbound connections (with exceptions for domain)
    try {
        # This is a sample - actual implementation should be more sophisticated
        Write-FRFDLog "Creating firewall rules..." -Level Info

        # Create blocking rule
        New-NetFirewallRule -DisplayName "FRFD-Containment-Block-Outbound" `
            -Direction Outbound `
            -Action Block `
            -Profile Any `
            -Enabled True `
            -ErrorAction Stop

        Write-FRFDLog "Outbound traffic blocked" -Level Success
    } catch {
        Write-FRFDLog "Firewall configuration failed: $_" -Level Error
    }

    # Log containment action
    $containmentLog = @{
        Timestamp = (Get-Date).ToString('o')
        Action = 'Network Isolation'
        Responder = $Responder
        CaseId = $CaseId
        Hostname = $env:COMPUTERNAME
    }

    $containmentLog | ConvertTo-Json | Out-File -FilePath (Join-Path $OutputPath "containment_$(Get-Date -Format 'yyyyMMdd_HHmmss').json")

    Write-FRFDLog "Containment procedures complete" -Level Success
}

function New-ChainOfCustody {
    Write-FRFDLog "Generating Chain of Custody documentation..." -Level Info

    $endTime = Get-Date
    $duration = $endTime - $StartTime

    $custody = @{
        case_id = $CaseId
        responder = $Responder
        device_id = "FRFD-001"
        hostname = $env:COMPUTERNAME
        start_time = $StartTime.ToString('o')
        end_time = $endTime.ToString('o')
        duration_seconds = [int]$duration.TotalSeconds
        mode = $Mode
        output_path = $OutputPath
        artifacts = @()
    }

    # List all collected files
    if (Test-Path $OutputPath) {
        $files = Get-ChildItem -Path $OutputPath -Recurse -File | Select-Object -First 1000

        foreach ($file in $files) {
            $hash = (Get-FileHash -Path $file.FullName -Algorithm SHA256).Hash

            $custody.artifacts += @{
                filename = $file.Name
                path = $file.FullName
                size = $file.Length
                hash = "sha256:$hash"
                created = $file.CreationTime.ToString('o')
            }
        }
    }

    $custodyFile = Join-Path $OutputPath "chain_of_custody_$(Get-Date -Format 'yyyyMMdd_HHmmss').json"
    $custody | ConvertTo-Json -Depth 10 | Out-File -FilePath $custodyFile -Encoding UTF8

    Write-FRFDLog "Chain of Custody: $custodyFile" -Level Success
    Write-FRFDLog "Total artifacts: $($custody.artifacts.Count)" -Level Info
}

function Send-ResultsToDevice {
    param([string]$Path)

    Write-FRFDLog "Preparing results for transfer to device..." -Level Info

    # In real implementation, this would:
    # 1. Compress collected artifacts
    # 2. Send via serial/USB to dongle
    # 3. Or upload to WiFi endpoint

    # For now, just create a summary file
    $summary = @{
        Status = 'Complete'
        Mode = $Mode
        CaseId = $CaseId
        OutputPath = $Path
        Timestamp = (Get-Date).ToString('o')
    }

    Write-Host ($summary | ConvertTo-Json)
}

# Main Execution
try {
    Show-Banner
    Initialize-FRFD

    switch ($Mode) {
        'Triage' {
            Invoke-TriageCollection
        }
        'Collect' {
            Invoke-FullCollection
        }
        'Contain' {
            Invoke-ContainmentProcedure
        }
        'Analyze' {
            Write-FRFDLog "Analysis mode not yet implemented" -Level Warning
        }
    }

    # Generate chain of custody
    New-ChainOfCustody

    # Send results back to device
    Send-ResultsToDevice -Path $OutputPath

    $duration = (Get-Date) - $StartTime
    Write-FRFDLog "FRFD execution complete in $([int]$duration.TotalSeconds) seconds" -Level Success

} catch {
    Write-FRFDLog "Fatal error: $_" -Level Error
    Write-FRFDLog $_.ScriptStackTrace -Level Error
    exit 1
}
