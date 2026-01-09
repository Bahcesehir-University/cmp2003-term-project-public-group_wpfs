#pragma once
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

struct ZoneCount {
    string zone;
    long long count;
};

struct SlotCount {
    string zone;
    int hour;              // 0–23
    long long count;
};

class TripAnalyzer {
public:
    // Parse Trips.csv, skip dirty rows, never crash
    void ingestFile(const string& csvPath);

    // Top K zones: count desc, zone asc
    vector<ZoneCount> topZones(int k = 10) const;

    // Top K slots: count desc, zone asc, hour asc
    vector<SlotCount> topBusySlots(int k = 10) const;

private:
    struct ZoneStats {
        long long total = 0;
        long long byHour[24] = {0};
    };

    unordered_map<string, ZoneStats> stats;

    static string trim(const string& s);
    static bool split6(const string& line, string out[6]);
    static bool parseHour(const string& dtRaw, int& hourOut);

    void processLine(const string& line);
};
