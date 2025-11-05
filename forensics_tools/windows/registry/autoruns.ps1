# FRFD - Windows Autorun Registry Entries
# Collects all autorun/persistence registry keys

param(
    [string]$OutputPath = "C:\CSIRT\Evidence\Registry"
)

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$outputDir = Join-Path $OutputPath "Autoruns_$timestamp"
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

Write-Host "[FRFD] Collecting autorun registry entries..."

# Common autorun registry locations
$autorunKeys = @(
    # Current User
    "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run",
    "HKCU:\Software\Microsoft\Windows\CurrentVersion\RunOnce",
    "HKCU:\Software\Microsoft\Windows\CurrentVersion\RunServices",
    "HKCU:\Software\Microsoft\Windows\CurrentVersion\RunServicesOnce",
    "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders",
    "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders",
    "HKCU:\Software\Microsoft\Windows NT\CurrentVersion\Windows\Run",

    # Local Machine
    "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run",
    "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\RunOnce",
    "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\RunServices",
    "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\RunServicesOnce",
    "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\RunOnceEx",
    "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer\Run",

    # Startup folders
    "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run",
    "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run32",
    "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run",
    "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run32",

    # Services
    "HKLM:\SYSTEM\CurrentControlSet\Services",

    # Winlogon
    "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon",
    "HKCU:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon"
)

$results = @()

foreach ($key in $autorunKeys) {
    try {
        if (Test-Path $key) {
            Write-Host "[FRFD] Collecting: $key"

            $items = Get-ItemProperty -Path $key -ErrorAction SilentlyContinue

            if ($items) {
                $properties = $items.PSObject.Properties | Where-Object { $_.Name -notlike "PS*" }

                foreach ($prop in $properties) {
                    $results += [PSCustomObject]@{
                        RegistryKey = $key
                        Name = $prop.Name
                        Value = $prop.Value
                        Type = $prop.TypeNameOfValue
                        Timestamp = (Get-Date).ToString("o")
                    }
                }
            }

            # For Services, get subkeys
            if ($key -like "*Services*") {
                $services = Get-ChildItem -Path $key -ErrorAction SilentlyContinue | Select-Object -First 100
                foreach ($service in $services) {
                    try {
                        $svcProps = Get-ItemProperty -Path $service.PSPath -ErrorAction SilentlyContinue
                        if ($svcProps.ImagePath) {
                            $results += [PSCustomObject]@{
                                RegistryKey = $service.PSPath
                                Name = $service.PSChildName
                                Value = $svcProps.ImagePath
                                Type = "Service"
                                Timestamp = (Get-Date).ToString("o")
                            }
                        }
                    } catch {
                        # Skip inaccessible services
                    }
                }
            }
        }
    } catch {
        Write-Warning "[FRFD] Failed to access: $key - $_"
    }
}

# Export results
$csvFile = Join-Path $outputDir "autoruns.csv"
$jsonFile = Join-Path $outputDir "autoruns.json"

$results | Export-Csv -Path $csvFile -NoTypeInformation -Encoding UTF8
$results | ConvertTo-Json -Depth 10 | Out-File -FilePath $jsonFile -Encoding UTF8

Write-Host "[FRFD] Collected $($results.Count) autorun entries"
Write-Host "[FRFD] CSV: $csvFile"
Write-Host "[FRFD] JSON: $jsonFile"

return @{
    Success = $true
    EntryCount = $results.Count
    OutputPath = $outputDir
    CSVFile = $csvFile
    JSONFile = $jsonFile
}
