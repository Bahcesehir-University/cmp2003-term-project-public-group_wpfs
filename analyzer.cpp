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

string TripAnalyzer::normalizeZone(const string& s) {
    string z = trim(s);
    for (char& c : z) c = toupper((unsigned char)c);
    return z;
}

bool TripAnalyzer::parseHour(const string& raw, int& hour) {
    for (size_t i = 0; i + 1 < raw.size(); i++) {
        if (isdigit(raw[i]) && isdigit(raw[i + 1])) {
            int h = (raw[i] - '0') * 10 + (raw[i + 1] - '0');
            if (h >= 0 && h <= 23) {
                hour = h;
                return true;
            }
        }
    }
    return false;
}

void TripAnalyzer::processLine(const string& line) {
    if (line.empty()) return;

    vector<string> fields;
    size_t start = 0;

    while (true) {
        size_t pos = line.find(',', start);
        if (pos == string::npos) {
            fields.push_back(trim(line.substr(start)));
            break;
        }
        fields.push_back(trim(line.substr(start, pos - start)));
        start = pos + 1;
    }

    string zone = "";
    int hour = -1;

    for (const string& f : fields) {
        if (zone.empty()) {
            bool ok = !f.empty();
            for (char c : f)
                if (!isalnum((unsigned char)c)) ok = false;
            if (ok) zone = normalizeZone(f);
        }

        if (hour == -1) {
            parseHour(f, hour);
        }
    }

    if (zone.empty() || hour < 0) return;

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
