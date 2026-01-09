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
    for (int i = 0; i < 6; i++) out[i].clear();

    size_t start = 0;
    int field = 0;

    while (field < 6) {
        size_t pos = line.find(',', start);
        if (pos == string::npos) {
            out[field++] = trim(line.substr(start));
            break;
        }
        out[field++] = trim(line.substr(start, pos - start));
        start = pos + 1;
    }
    return true;
}

static bool allDigits(const string& s) {
    if (s.empty()) return false;
    for (unsigned char c : s) {
        if (!isdigit(c)) return false;
    }
    return true;
}

static bool isLeap(int y) {
    if (y % 400 == 0) return true;
    if (y % 100 == 0) return false;
    return (y % 4 == 0);
}

static int daysInMonth(int y, int m) {
    static const int d[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m < 1 || m > 12) return 0;
    if (m == 2) return d[m - 1] + (isLeap(y) ? 1 : 0);
    return d[m - 1];
}

bool TripAnalyzer::parseHour(const string& dtRaw, int& hourOut) {
    string s = trim(dtRaw);

    int nums[6];
    int n = 0;

    for (size_t i = 0; i < s.size() && n < 6; ) {
        if (!isdigit((unsigned char)s[i])) { i++; continue; }
        int val = 0;
        while (i < s.size() && isdigit((unsigned char)s[i])) {
            val = val * 10 + (s[i] - '0');
            i++;
        }
        nums[n++] = val;
    }

    if (n < 5) return false;

    int y = nums[0];
    int mo = nums[1];
    int d = nums[2];
    int h = nums[3];
    int mi = nums[4];
    int se = (n >= 6) ? nums[5] : 0;

    if (y <= 0 || y > 9999) return false;
    if (mo < 1 || mo > 12) return false;
    int dim = daysInMonth(y, mo);
    if (d < 1 || d > dim) return false;

    if (h < 0 || h > 23) return false;
    if (mi < 0 || mi > 59) return false;
    if (se < 0 || se > 59) return false;

    hourOut = h;
    return true;
}

void TripAnalyzer::processLine(const string& line) {
    if (line.empty()) return;

    string f[6];
    if (!split6(line, f)) return;

    const string& tripId = f[0];
    const string& zone = f[1];
    const string& dt = f[3];

    if (!allDigits(tripId)) return;
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
            if (line.find("TripID") != string::npos && line.find("PickupZoneID") != string::npos)
                continue;
        }
        processLine(line);
    }
}

vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    if (k <= 0 || stats.empty()) return {};

    vector<ZoneCount> v;
    v.reserve(stats.size());
    for (const auto& kv : stats) v.push_back({kv.first, kv.second.total});

    sort(v.begin(), v.end(), [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    });

    if ((int)v.size() > k) v.resize(k);
    return v;
}

vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    if (k <= 0 || stats.empty()) return {};

    vector<SlotCount> v;
    v.reserve(stats.size() * 4);

    for (const auto& kv : stats) {
        const string& z = kv.first;
        const ZoneStats& zs = kv.second;
        for (int h = 0; h < 24; h++) {
            long long c = zs.byHour[h];
            if (c > 0) v.push_back({z, h, c});
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
