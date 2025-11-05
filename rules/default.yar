/*
    FRFD Default IOC Rules
    Compatible with YARA syntax for future migration
    Lightweight rules for ESP32-S3 implementation
*/

rule Malicious_PowerShell_Commands
{
    meta:
        description = "Detects suspicious PowerShell command patterns"
        severity = "high"
        category = "execution"

    strings:
        $s1 = "IEX" nocase
        $s2 = "Invoke-Expression" nocase
        $s3 = "DownloadString" nocase
        $s4 = "Net.WebClient" nocase
        $s5 = "EncodedCommand" nocase
        $s6 = "-enc" nocase
        $s7 = "bypass" nocase
        $s8 = "hidden" nocase
        $s9 = "noprofile" nocase

    condition:
        2 of them
}

rule Credential_Dumping_Tools
{
    meta:
        description = "Detects credential theft and dumping tools"
        severity = "critical"
        category = "credential_access"

    strings:
        $s1 = "mimikatz" nocase
        $s2 = "sekurlsa" nocase
        $s3 = "lsadump" nocase
        $s4 = "procdump" nocase
        $s5 = "lsass" nocase
        $s6 = "dump" nocase
        $s7 = "passwords" nocase

    condition:
        2 of them
}

rule Registry_Persistence
{
    meta:
        description = "Windows registry-based persistence mechanisms"
        severity = "medium"
        category = "persistence"

    strings:
        $s1 = "CurrentVersion\\Run" nocase
        $s2 = "\\Policies\\Explorer\\Run" nocase
        $s3 = "UserInitMprLogonScript" nocase
        $s4 = "Winlogon\\Shell" nocase
        $s5 = "\\Image File Execution Options\\" nocase

    condition:
        any of them
}

rule Ransomware_Indicators
{
    meta:
        description = "Common ransomware file and pattern indicators"
        severity = "critical"
        category = "impact"

    strings:
        $ext1 = ".locked" nocase
        $ext2 = ".encrypted" nocase
        $ext3 = ".crypto" nocase
        $s1 = "DECRYPT" nocase
        $s2 = "RANSOM" nocase
        $s3 = "bitcoin" nocase
        $s4 = "wallet address" nocase
        $s5 = "YOUR FILES HAVE BEEN ENCRYPTED" nocase

    condition:
        2 of them
}

rule LKM_Rootkit_Names
{
    meta:
        description = "Known Linux Kernel Module rootkit names"
        severity = "critical"
        category = "persistence"
        platform = "linux"

    strings:
        $s1 = "diamorphine" nocase
        $s2 = "reptile" nocase
        $s3 = "suterusu" nocase
        $s4 = "kovid" nocase
        $s5 = "rkduck" nocase
        $s6 = "adore" nocase
        $s7 = "knark" nocase

    condition:
        any of them
}

rule Suspicious_Shell_Commands
{
    meta:
        description = "Suspicious bash/shell command patterns"
        severity = "high"
        category = "execution"
        platform = "linux"

    strings:
        $s1 = "curl | bash" nocase
        $s2 = "wget | sh" nocase
        $s3 = "/dev/tcp/" nocase
        $s4 = "bash -i" nocase
        $s5 = "nc -e" nocase
        $s6 = "python -c" nocase
        $s7 = "perl -e" nocase
        $s8 = "base64 -d" nocase

    condition:
        any of them
}

rule PHP_Webshell
{
    meta:
        description = "PHP-based webshell detection"
        severity = "critical"
        category = "persistence"
        filetype = "php"

    strings:
        $s1 = "eval(" nocase
        $s2 = "base64_decode" nocase
        $s3 = "shell_exec" nocase
        $s4 = "system(" nocase
        $s5 = "passthru" nocase
        $s6 = "exec(" nocase
        $s7 = "$_POST" nocase
        $s8 = "$_GET" nocase
        $s9 = "assert(" nocase

    condition:
        3 of them
}

rule China_Chopper_Webshell
{
    meta:
        description = "China Chopper webshell detection"
        severity = "critical"
        category = "persistence"
        reference = "https://www.fireeye.com/blog/threat-research/2013/08/breaking-down-the-china-chopper-web-shell-part-i.html"

    strings:
        $s1 = "eval(Request" nocase
        $s2 = "Execute(Request" nocase
        $s3 = "eval(base64_decode($_POST" nocase
        $chopper = /eval\s*\(\s*(\$_POST\[|Request\[)/

    condition:
        any of them
}

rule C2_Communication_Patterns
{
    meta:
        description = "Command and Control communication indicators"
        severity = "high"
        category = "command_and_control"

    strings:
        $s1 = "beacon" nocase
        $s2 = "checkin" nocase
        $s3 = "heartbeat" nocase
        $s4 = "/admin/get.php" nocase
        $s5 = "/gate.php" nocase
        $s6 = "X-Session-Id:" nocase

    condition:
        any of them
}

rule Data_Exfiltration_Services
{
    meta:
        description = "Common data exfiltration services"
        severity = "high"
        category = "exfiltration"

    strings:
        $s1 = "paste.ee" nocase
        $s2 = "pastebin.com" nocase
        $s3 = "transfer.sh" nocase
        $s4 = "file.io" nocase
        $s5 = "gofile.io" nocase
        $s6 = "anonfiles.com" nocase

    condition:
        any of them
}

rule Remote_Access_Tools
{
    meta:
        description = "Remote access tool indicators"
        severity = "medium"
        category = "command_and_control"

    strings:
        $s1 = "anydesk" nocase
        $s2 = "teamviewer" nocase
        $s3 = "psexec" nocase
        $s4 = "winvnc" nocase
        $s5 = "remotepc" nocase
        $s6 = "ammyy" nocase
        $s7 = "logmein" nocase

    condition:
        any of them
}

rule Suspicious_Cron_Jobs
{
    meta:
        description = "Malicious cron job patterns"
        severity = "medium"
        category = "persistence"
        platform = "linux"

    strings:
        $s1 = "curl" nocase
        $s2 = "wget" nocase
        $s3 = "/tmp/" nocase
        $s4 = "base64" nocase
        $s5 = "python -c" nocase
        $s6 = "nc " nocase
        $s7 = "/dev/tcp/" nocase

    condition:
        any of them
}

rule SSH_Backdoor_Keys
{
    meta:
        description = "Suspicious SSH authorized_keys entries"
        severity = "high"
        category = "persistence"
        platform = "linux"

    strings:
        $s1 = "from=\"*\"" nocase
        $s2 = "command=" nocase
        $s3 = "no-pty" nocase
        $s4 = "no-X11-forwarding" nocase

    condition:
        2 of them
}

rule LD_PRELOAD_Injection
{
    meta:
        description = "LD_PRELOAD library injection"
        severity = "critical"
        category = "persistence"
        platform = "linux"

    strings:
        $s1 = "/etc/ld.so.preload" nocase
        $s2 = "LD_PRELOAD=" nocase

    condition:
        any of them
}

rule Cobalt_Strike_Beacon
{
    meta:
        description = "Cobalt Strike beacon indicators"
        severity = "critical"
        category = "command_and_control"

    strings:
        $s1 = "beacon" nocase
        $s2 = "cobaltstrike" nocase
        $s3 = "malleable" nocase
        $s4 = "MSSE-" nocase

    condition:
        2 of them
}

rule Metasploit_Indicators
{
    meta:
        description = "Metasploit framework indicators"
        severity = "high"
        category = "execution"

    strings:
        $s1 = "meterpreter" nocase
        $s2 = "metasploit" nocase
        $s3 = "msfvenom" nocase
        $s4 = "/tmp/msf" nocase
        $s5 = "reverse_tcp" nocase

    condition:
        any of them
}

rule Suspicious_Python_Scripts
{
    meta:
        description = "Suspicious Python script patterns"
        severity = "medium"
        category = "execution"

    strings:
        $s1 = "socket.socket" nocase
        $s2 = "subprocess.Popen" nocase
        $s3 = "os.system" nocase
        $s4 = "eval(compile" nocase
        $s5 = "__import__" nocase
        $s6 = "base64.b64decode" nocase

    condition:
        3 of them
}

rule Suspicious_PowerShell_Encoded
{
    meta:
        description = "Encoded/obfuscated PowerShell commands"
        severity = "high"
        category = "execution"

    strings:
        $s1 = "-encodedcommand" nocase
        $s2 = "FromBase64String" nocase
        $s3 = "::UTF8.GetString" nocase
        $s4 = "IO.Compression" nocase
        $s5 = "IO.MemoryStream" nocase

    condition:
        2 of them
}

rule LOLBAS_Abuse
{
    meta:
        description = "Living Off the Land Binaries abuse"
        severity = "medium"
        category = "defense_evasion"

    strings:
        $s1 = "certutil" nocase
        $s2 = "bitsadmin" nocase
        $s3 = "mshta" nocase
        $s4 = "regsvr32" nocase
        $s5 = "rundll32" nocase
        $s6 = "cscript" nocase
        $s7 = "wscript" nocase

    condition:
        any of them
}

rule WMI_Persistence
{
    meta:
        description = "WMI-based persistence mechanisms"
        severity = "high"
        category = "persistence"

    strings:
        $s1 = "ActiveScriptEventConsumer" nocase
        $s2 = "CommandLineEventConsumer" nocase
        $s3 = "__EventFilter" nocase
        $s4 = "__FilterToConsumerBinding" nocase

    condition:
        any of them
}
