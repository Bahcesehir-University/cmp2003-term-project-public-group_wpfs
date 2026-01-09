#include "analyzer.h"
#include <fstream>
#include <algorithm>
#include <cctype>

string TripAnalyzer::trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

bool TripAnalyzer::split6(const string& line, string out[6]) {
    size_t start = 0;
    for (int i = 0; i < 6; i++) {
        size_t pos = (i == 5) ? string::npos : line.find(',', start);
        if (pos == string::npos && i != 5) return false;
        out[i] = trim(line.substr(start, pos - start));
        start = (pos == string::npos) ? line.size() : pos + 1;
    }
    return true;
}

bool TripAnalyzer::parseHour(const string& dtRaw, int& hourOut) {
    string s = trim(dtRaw);
    size_t c = s.find(':');
    if (c == string::npos) return false;

    int i = (int)c - 1;
    if (i < 0 || !isdigit(s[i])) return false;

    int h = s[i] - '0';
    i--;
    if (i >= 0 && isdigit(s[i])) h = (s[i] - '0') * 10 + h;

    if (h < 0 || h > 23) return false;
    hourOut = h;
    return true;
}

void TripAnalyzer::processLine(const string& line) {
    if (line.empty()) return;

    string f[6];
    if (!split6(line, f)) return;

    const string& zone = f[2];
    const string& dt   = f[4];

    if (zone.empty() || dt.empty()) return;

    int h;
    if (!parseHour(dt, h)) return;

    ZoneStats& z = stats[zone];
    z.total++;
    z.byHour[h]++;
}

void TripAnalyzer::ingestFile(const string& csvPath) {
    stats.clear();

    ifstream file(csvPath);
    if (!file.is_open()) return;

    string line;
    bool first = true;

    while (getline(file, line)) {
        if (first) {
            first = false;
            continue;
        }
        processLine(line);
    }
}

vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    vector<ZoneCount> v;
    for (const auto& kv : stats)
        v.push_back({kv.first, kv.second.total});

    sort(v.begin(), v.end(), [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    });

    if ((int)v.size() > k) v.resize(k);
    return v;
}

vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    vector<SlotCount> v;

    for (const auto& kv : stats) {
        for (int h = 0; h < 24; h++) {
            if (kv.second.byHour[h] > 0)
                v.push_back({kv.first, h, kv.second.byHour[h]});
        }
    }

    sort(v.begin(), v.end(), [](const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone != b.zone) return a.zone < b.zone;
        return a.hour < b.hour;
    });

    if ((int)v.size() > k) v.resize(k);
    return v;
}
