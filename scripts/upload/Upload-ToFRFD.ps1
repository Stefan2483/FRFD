<#
.SYNOPSIS
    Upload forensic artifacts to FRFD dongle via WiFi
.DESCRIPTION
    This PowerShell function uploads files to the FRFD forensics dongle
    over WiFi. It's designed to be embedded in HID automation scripts.
.PARAMETER FilePath
    Path to the file to upload
.PARAMETER ArtifactType
    Type of artifact (memory, registry, logs, network, etc.)
.PARAMETER SourcePath
    Original source path on target system
.PARAMETER FRFDIP
    IP address of FRFD dongle (default: 192.168.4.1)
#>
function Upload-ToFRFD {
    param(
        [Parameter(Mandatory=$true)]
        [string]$FilePath,

        [Parameter(Mandatory=$false)]
        [string]$ArtifactType = "unknown",

        [Parameter(Mandatory=$false)]
        [string]$SourcePath = "",

        [Parameter(Mandatory=$false)]
        [string]$FRFDIP = "192.168.4.1"
    )

    try {
        # Verify file exists
        if (-not (Test-Path $FilePath)) {
            Write-Error "File not found: $FilePath"
            return $false
        }

        # Get file info
        $file = Get-Item $FilePath
        $fileName = $file.Name
        $fileSize = $file.Length

        Write-Host "[FRFD] Uploading: $fileName ($fileSize bytes)"
        Write-Host "[FRFD] Type: $ArtifactType"

        # Build upload URL
        $uploadUrl = "http://$FRFDIP/upload"

        # Create multipart form data
        $boundary = [System.Guid]::NewGuid().ToString()
        $LF = "`r`n"

        # Read file as bytes
        $fileBytes = [System.IO.File]::ReadAllBytes($FilePath)

        # Build multipart body
        $bodyLines = @(
            "--$boundary",
            "Content-Disposition: form-data; name=`"type`"$LF",
            $ArtifactType,
            "--$boundary",
            "Content-Disposition: form-data; name=`"source_path`"$LF",
            $SourcePath,
            "--$boundary",
            "Content-Disposition: form-data; name=`"file`"; filename=`"$fileName`"",
            "Content-Type: application/octet-stream$LF"
        ) -join $LF

        # Combine text and binary parts
        $bodyLinesBytes = [System.Text.Encoding]::UTF8.GetBytes($bodyLines)
        $endBoundaryBytes = [System.Text.Encoding]::UTF8.GetBytes("$LF--$boundary--$LF")

        # Create complete body
        $requestBody = New-Object byte[] ($bodyLinesBytes.Length + $fileBytes.Length + $endBoundaryBytes.Length)
        [System.Array]::Copy($bodyLinesBytes, 0, $requestBody, 0, $bodyLinesBytes.Length)
        [System.Array]::Copy($fileBytes, 0, $requestBody, $bodyLinesBytes.Length, $fileBytes.Length)
        [System.Array]::Copy($endBoundaryBytes, 0, $requestBody, $bodyLinesBytes.Length + $fileBytes.Length, $endBoundaryBytes.Length)

        # Upload with retry logic
        $maxRetries = 3
        $retryCount = 0
        $success = $false

        while (-not $success -and $retryCount -lt $maxRetries) {
            try {
                $response = Invoke-WebRequest -Uri $uploadUrl `
                    -Method Post `
                    -ContentType "multipart/form-data; boundary=$boundary" `
                    -Body $requestBody `
                    -TimeoutSec 60

                if ($response.StatusCode -eq 200) {
                    $result = $response.Content | ConvertFrom-Json
                    Write-Host "[FRFD] Upload successful!"
                    Write-Host "[FRFD] Artifact ID: $($result.artifact_id)"
                    Write-Host "[FRFD] Speed: $($result.speed_kbps) KB/s"
                    $success = $true
                    return $true
                }
            }
            catch {
                $retryCount++
                Write-Warning "[FRFD] Upload failed (attempt $retryCount/$maxRetries): $_"
                if ($retryCount -lt $maxRetries) {
                    Start-Sleep -Seconds 2
                }
            }
        }

        if (-not $success) {
            Write-Error "[FRFD] Upload failed after $maxRetries attempts"
            return $false
        }
    }
    catch {
        Write-Error "[FRFD] Upload error: $_"
        return $false
    }
}

# Inline version for HID automation (compact, no comments)
function Upload-ToFRFD-Inline {
    param([string]$f,[string]$t="unknown",[string]$s="",[string]$ip="192.168.4.1")
    try{if(-not(Test-Path $f)){return $false};$fi=Get-Item $f;$fb=[IO.File]::ReadAllBytes($f);$b=[Guid]::NewGuid().ToString();$lf="`r`n";$bl=@("--$b","Content-Disposition: form-data; name=`"type`"$lf",$t,"--$b","Content-Disposition: form-data; name=`"source_path`"$lf",$s,"--$b","Content-Disposition: form-data; name=`"file`"; filename=`"$($fi.Name)`"","Content-Type: application/octet-stream$lf")-join $lf;$blb=[Text.Encoding]::UTF8.GetBytes($bl);$ebb=[Text.Encoding]::UTF8.GetBytes("$lf--$b--$lf");$rb=New-Object byte[]($blb.Length+$fb.Length+$ebb.Length);[Array]::Copy($blb,0,$rb,0,$blb.Length);[Array]::Copy($fb,0,$rb,$blb.Length,$fb.Length);[Array]::Copy($ebb,0,$rb,$blb.Length+$fb.Length,$ebb.Length);$r=Invoke-WebRequest -Uri "http://$ip/upload" -Method Post -ContentType "multipart/form-data; boundary=$b" -Body $rb -TimeoutSec 60;return ($r.StatusCode -eq 200)}catch{return $false}}

# Export functions
Export-ModuleMember -Function Upload-ToFRFD, Upload-ToFRFD-Inline
