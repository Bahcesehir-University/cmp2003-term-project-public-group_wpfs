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
    int i = 0;

    while (i < 6) {
        size_t pos = line.find(',', start);
        if (pos == string::npos) {
            out[i++] = trim(line.substr(start));
            break;
        }
        out[i++] = trim(line.substr(start, pos - start));
        start = pos + 1;
    }

    return i == 6;
}

bool TripAnalyzer::parseHour(const string& dt, int& hour) {
    size_t c = dt.find(':');
    if (c == string::npos || c < 2) return false;

    if (!isdigit(dt[c - 1])) return false;

    int h = dt[c - 1] - '0';
    if (c >= 2 && isdigit(dt[c - 2])) {
        h = (dt[c - 2] - '0') * 10 + h;
    }

    if (h < 0 || h > 23) return false;
    hour = h;
    return true;
}

void TripAnalyzer::processLine(const string& line) {
    if (line.empty()) return;

    string f[6];
    if (!split6(line, f)) return;

    string zone = trim(f[1]);
    string dt   = trim(f[3]);

    if (zone.empty() || dt.empty()) return;

    int hour;
    if (!parseHour(dt, hour)) return;

    for (char& c : zone) c = toupper((unsigned char)c);

    ZoneStats& z = stats[zone];
    z.total++;
    z.byHour[hour]++;
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
            if (line.find("TripID") != string::npos &&
                line.find("PickupZoneID") != string::npos)
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
