#include "ioc_matcher.h"

IOCMatcher::IOCMatcher() {
}

bool IOCMatcher::addRule(const IOCRule& rule) {
    rules.push_back(rule);
    Serial.printf("[IOC] Added rule: %s\n", rule.name.c_str());
    return true;
}

void IOCMatcher::clearRules() {
    rules.clear();
}

size_t IOCMatcher::getRuleCount() {
    return rules.size();
}

bool IOCMatcher::containsString(const String& content, const String& pattern) {
    // Case-insensitive search
    String contentLower = content;
    String patternLower = pattern;
    contentLower.toLowerCase();
    patternLower.toLowerCase();

    return contentLower.indexOf(patternLower) >= 0;
}

bool IOCMatcher::containsHex(const String& content, const String& hexPattern) {
    // Simple hex pattern matching
    // Convert hex pattern to bytes and search
    // This is a simplified implementation
    return content.indexOf(hexPattern) >= 0;
}

bool IOCMatcher::matchRule(const String& content, const IOCRule& rule) {
    std::vector<String> matched;

    // Check string patterns
    for (const String& pattern : rule.strings) {
        if (containsString(content, pattern)) {
            matched.push_back(pattern);
        }
    }

    // Check hex patterns
    for (const String& hexPattern : rule.hexPatterns) {
        if (containsHex(content, hexPattern)) {
            matched.push_back(hexPattern);
        }
    }

    // Evaluate condition
    bool ruleMatched = false;

    if (rule.condition == "any" && matched.size() > 0) {
        ruleMatched = true;
    } else if (rule.condition == "all") {
        size_t totalPatterns = rule.strings.size() + rule.hexPatterns.size();
        ruleMatched = (matched.size() == totalPatterns);
    } else if (rule.condition.startsWith("2 of")) {
        ruleMatched = (matched.size() >= 2);
    } else if (rule.condition.startsWith("3 of")) {
        ruleMatched = (matched.size() >= 3);
    }

    return ruleMatched && matched.size() > 0;
}

bool IOCMatcher::scanContent(const String& content, const String& fileName) {
    bool foundMatch = false;

    for (const IOCRule& rule : rules) {
        if (matchRule(content, rule)) {
            IOCMatch match;
            match.ruleName = rule.name;
            match.fileName = fileName;
            match.severity = rule.severity;
            match.timestamp = millis();

            // Collect matched strings
            for (const String& pattern : rule.strings) {
                if (containsString(content, pattern)) {
                    match.matchedStrings.push_back(pattern);
                }
            }

            matches.push_back(match);
            foundMatch = true;

            Serial.printf("[IOC] ⚠️  MATCH: %s in %s (Severity: %s)\n",
                         rule.name.c_str(),
                         fileName.c_str(),
                         rule.severity.c_str());
        }
    }

    return foundMatch;
}

bool IOCMatcher::scanFile(const String& filePath) {
    // This would read the file and scan its content
    // For now, placeholder
    Serial.printf("[IOC] Scanning file: %s\n", filePath.c_str());
    return false;
}

std::vector<IOCMatch> IOCMatcher::getMatches() {
    return matches;
}

void IOCMatcher::clearMatches() {
    matches.clear();
}

size_t IOCMatcher::getMatchCount() {
    return matches.size();
}

bool IOCMatcher::hasMatches() {
    return matches.size() > 0;
}

void IOCMatcher::loadDefaultRules() {
    Serial.println("[IOC] Loading default rules...");
    loadWindowsRules();
    loadLinuxRules();
    loadWebshellRules();
    loadNetworkRules();
}

void IOCMatcher::loadWindowsRules() {
    // Rule 1: Malicious PowerShell
    IOCRule psRule;
    psRule.name = "Malicious_PowerShell_Commands";
    psRule.description = "Detects suspicious PowerShell patterns";
    psRule.severity = "high";
    psRule.strings = {
        "IEX",
        "Invoke-Expression",
        "DownloadString",
        "Net.WebClient",
        "EncodedCommand",
        "-enc",
        "bypass",
        "hidden",
        "noprofile"
    };
    psRule.condition = "2 of them";
    addRule(psRule);

    // Rule 2: Credential Dumping
    IOCRule credRule;
    credRule.name = "Credential_Dumping_Tools";
    credRule.description = "Detects credential theft tools";
    credRule.severity = "critical";
    credRule.strings = {
        "mimikatz",
        "sekurlsa",
        "lsadump",
        "procdump",
        "lsass",
        "dump"
    };
    credRule.condition = "2 of them";
    addRule(credRule);

    // Rule 3: Persistence Mechanisms
    IOCRule persistRule;
    persistRule.name = "Registry_Persistence";
    persistRule.description = "Detects registry-based persistence";
    persistRule.severity = "medium";
    persistRule.strings = {
        "CurrentVersion\\Run",
        "\\Policies\\Explorer\\Run",
        "UserInitMprLogonScript",
        "Winlogon\\Shell"
    };
    persistRule.condition = "any";
    addRule(persistRule);

    // Rule 4: Remote Access Tools
    IOCRule ratRule;
    ratRule.name = "Remote_Access_Tools";
    ratRule.description = "Detects RAT artifacts";
    ratRule.severity = "high";
    ratRule.strings = {
        "anydesk",
        "teamviewer",
        "psexec",
        "winvnc",
        "remotepc",
        "ammyy"
    };
    ratRule.condition = "any";
    addRule(ratRule);

    // Rule 5: Ransomware Indicators
    IOCRule ransomRule;
    ransomRule.name = "Ransomware_Indicators";
    ransomRule.description = "Detects ransomware patterns";
    ransomRule.severity = "critical";
    ransomRule.strings = {
        ".locked",
        ".encrypted",
        "DECRYPT",
        "RANSOM",
        "bitcoin",
        "wallet address"
    };
    ransomRule.condition = "2 of them";
    addRule(ransomRule);
}

void IOCMatcher::loadLinuxRules() {
    // Rule 1: Suspicious Shell Commands
    IOCRule shellRule;
    shellRule.name = "Suspicious_Shell_Commands";
    shellRule.description = "Detects suspicious bash patterns";
    shellRule.severity = "high";
    shellRule.strings = {
        "curl | bash",
        "wget | sh",
        "/dev/tcp/",
        "bash -i",
        "nc -e",
        "python -c",
        "perl -e"
    };
    shellRule.condition = "any";
    addRule(shellRule);

    // Rule 2: LKM Rootkits
    IOCRule rootkitRule;
    rootkitRule.name = "LKM_Rootkit_Names";
    rootkitRule.description = "Known LKM rootkit names";
    rootkitRule.severity = "critical";
    rootkitRule.strings = {
        "diamorphine",
        "reptile",
        "suterusu",
        "kovid",
        "rkduck",
        "adore",
        "knark"
    };
    rootkitRule.condition = "any";
    addRule(rootkitRule);

    // Rule 3: Suspicious Cron Jobs
    IOCRule cronRule;
    cronRule.name = "Suspicious_Cron_Jobs";
    cronRule.description = "Detects malicious cron entries";
    cronRule.severity = "medium";
    cronRule.strings = {
        "curl",
        "wget",
        "/tmp/",
        "base64",
        "python -c",
        "nc "
    };
    cronRule.condition = "any";
    addRule(cronRule);

    // Rule 4: SSH Backdoors
    IOCRule sshRule;
    sshRule.name = "SSH_Backdoor_Keys";
    sshRule.description = "Suspicious SSH keys";
    sshRule.severity = "high";
    sshRule.strings = {
        "from=\"*\"",
        "command=",
        "PermitRootLogin yes",
        "PasswordAuthentication no"
    };
    sshRule.condition = "any";
    addRule(sshRule);
}

void IOCMatcher::loadWebshellRules() {
    // Rule 1: PHP Webshells
    IOCRule phpShellRule;
    phpShellRule.name = "PHP_Webshell";
    phpShellRule.description = "Detects PHP webshells";
    phpShellRule.severity = "critical";
    phpShellRule.strings = {
        "eval(",
        "base64_decode",
        "shell_exec",
        "system(",
        "passthru",
        "exec(",
        "$_POST",
        "$_GET"
    };
    phpShellRule.condition = "3 of them";
    addRule(phpShellRule);

    // Rule 2: JSP Webshells
    IOCRule jspShellRule;
    jspShellRule.name = "JSP_Webshell";
    jspShellRule.description = "Detects JSP webshells";
    jspShellRule.severity = "critical";
    jspShellRule.strings = {
        "Runtime.getRuntime",
        "exec(",
        "ProcessBuilder",
        "request.getParameter"
    };
    jspShellRule.condition = "2 of them";
    addRule(jspShellRule);

    // Rule 3: China Chopper
    IOCRule chopperRule;
    chopperRule.name = "China_Chopper_Webshell";
    chopperRule.description = "China Chopper webshell detection";
    chopperRule.severity = "critical";
    chopperRule.strings = {
        "eval(Request",
        "Execute(Request",
        "eval(base64_decode($_POST"
    };
    chopperRule.condition = "any";
    addRule(chopperRule);
}

void IOCMatcher::loadNetworkRules() {
    // Rule 1: C2 Communication
    IOCRule c2Rule;
    c2Rule.name = "C2_Communication_Patterns";
    c2Rule.description = "Command and Control indicators";
    c2Rule.severity = "high";
    c2Rule.strings = {
        "beacon",
        "checkin",
        "heartbeat",
        "/admin/get.php",
        "/gate.php"
    };
    c2Rule.condition = "any";
    addRule(c2Rule);

    // Rule 2: Data Exfiltration
    IOCRule exfilRule;
    exfilRule.name = "Data_Exfiltration";
    exfilRule.description = "Detects data exfiltration patterns";
    exfilRule.severity = "high";
    exfilRule.strings = {
        "paste.ee",
        "pastebin.com",
        "transfer.sh",
        "file.io"
    };
    exfilRule.condition = "any";
    addRule(exfilRule);
}

String IOCMatcher::generateMatchReport() {
    String report = "\n=== IOC Match Report ===\n";
    report += "Timestamp: " + String(millis()) + "\n";
    report += "Total Matches: " + String(matches.size()) + "\n";
    report += "Rules Loaded: " + String(rules.size()) + "\n\n";

    if (matches.size() == 0) {
        report += "No IOC matches found.\n";
        return report;
    }

    for (const IOCMatch& match : matches) {
        report += "⚠️  MATCH FOUND\n";
        report += "  Rule: " + match.ruleName + "\n";
        report += "  File: " + match.fileName + "\n";
        report += "  Severity: " + match.severity + "\n";
        report += "  Matched Patterns:\n";

        for (const String& pattern : match.matchedStrings) {
            report += "    - " + pattern + "\n";
        }

        report += "\n";
    }

    return report;
}

void IOCMatcher::printMatches() {
    Serial.println(generateMatchReport());
}
