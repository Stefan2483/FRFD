#ifndef IOC_MATCHER_H
#define IOC_MATCHER_H

#include <Arduino.h>
#include <vector>

// Simplified YARA-like rule structure
struct IOCRule {
    String name;
    String description;
    String severity; // low, medium, high, critical
    std::vector<String> strings;
    std::vector<String> hexPatterns;
    String condition; // "any", "all", or count like "2 of them"
};

struct IOCMatch {
    String ruleName;
    String fileName;
    String severity;
    std::vector<String> matchedStrings;
    unsigned long timestamp;
};

class IOCMatcher {
private:
    std::vector<IOCRule> rules;
    std::vector<IOCMatch> matches;

    bool matchRule(const String& content, const IOCRule& rule);
    bool containsString(const String& content, const String& pattern);
    bool containsHex(const String& content, const String& hexPattern);

public:
    IOCMatcher();

    // Rule management
    bool loadRulesFromFile(const String& filePath);
    bool addRule(const IOCRule& rule);
    void clearRules();
    size_t getRuleCount();

    // Scanning
    bool scanContent(const String& content, const String& fileName);
    bool scanFile(const String& filePath);

    // Results
    std::vector<IOCMatch> getMatches();
    void clearMatches();
    size_t getMatchCount();
    bool hasMatches();

    // Built-in rules
    void loadDefaultRules();
    void loadWindowsRules();
    void loadLinuxRules();
    void loadWebshellRules();
    void loadNetworkRules();

    // Reporting
    String generateMatchReport();
    void printMatches();
};

#endif // IOC_MATCHER_H
