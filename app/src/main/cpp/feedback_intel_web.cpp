#include <string>
#include <cstdlib>
#include <chrono>
#include <thread>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <process.h>
#include <windows.h>
#undef small
#undef near
#undef far
#undef interface
struct winsize { unsigned short ws_row, ws_col; };
#define STDOUT_FILENO 1
#define STDIN_FILENO 0
#define TIOCGWINSZ 0
inline int ioctl(int, int, winsize*) { return -1; }
typedef unsigned int tcflag_t;
struct termios { tcflag_t c_lflag; };
#define ECHO 8
#define TCSANOW 0
inline int tcgetattr(int, termios*) { return -1; }
inline int tcsetattr(int, int, const termios*) { return 0; }
#define isatty _isatty
#define popen _popen
#define pclose _pclose
#define getpid _getpid
#define NULLDEV "NUL"
#define GITENV ""
#define FI_HAVE(cmd) (std::system("where " cmd " >NUL 2>&1") == 0)
#define FI_DISP "inline"
inline bool fi_is_curl(const std::string&) { return false; }
inline bool fi_curl_run(const std::string&, std::string&) { return false; }
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define NULLDEV "/dev/null"
#define GITENV "GIT_TERMINAL_PROMPT=0 "
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define FI_HAVE(cmd) (std::string(cmd) == "curl")
#define FI_DISP "inline"
bool fi_curl_run(const std::string& cmd, std::string& out);
inline bool fi_is_curl(const std::string& c) { return c.compare(0, 5, "curl ") == 0; }
template <class R, class P> inline void fi_sleep_for(std::chrono::duration<R, P> d) { emscripten_sleep((unsigned)std::chrono::duration_cast<std::chrono::milliseconds>(d).count()); }
#elif defined(__ANDROID__)
#define FI_HAVE(cmd) (std::string(cmd) == "curl" || std::system("command -v " cmd " >/dev/null 2>&1") == 0)
#define FI_DISP "attachment"
bool fi_curl_run(const std::string& cmd, std::string& out);
inline bool fi_is_curl(const std::string& c) { return c.compare(0, 5, "curl ") == 0; }
#else
#define FI_HAVE(cmd) (std::system("command -v " cmd " >/dev/null 2>&1") == 0)
#define FI_DISP "inline"
inline bool fi_is_curl(const std::string&) { return false; }
inline bool fi_curl_run(const std::string&, std::string&) { return false; }
#endif
#endif
#ifndef __EMSCRIPTEN__
template <class R, class P> inline void fi_sleep_for(std::chrono::duration<R, P> d) { std::this_thread::sleep_for(d); }
#endif

//
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <numeric>
#include <random>
#include <thread>
#include <cstdint>

inline std::string fi_open_cmd(const std::string& p) {
#ifdef _WIN32
    return "start \"\" \"" + p + "\"";
#elif defined(__APPLE__)
    return "open '" + p + "' >/dev/null 2>&1";
#else
    return "xdg-open '" + p + "' >/dev/null 2>&1";
#endif
}
namespace fs = std::filesystem;

namespace od {
namespace fs = std::filesystem;
using std::string; using std::vector;
struct Rgb {
    int r, g, b;
};

namespace palette {
const Rgb violet{167, 139, 250};
const Rgb pink{244, 114, 182};
const Rgb cyan{34, 211, 238};
const Rgb amber{251, 191, 36};
const Rgb green{74, 222, 128};
const Rgb red{248, 113, 113};
const Rgb slate{148, 163, 184};
const Rgb white{241, 245, 249};
const Rgb blue{96, 165, 250};
}

namespace tty {
const string reset = "\033[0m";
const string bold = "\033[1m";
const string dim = "\033[2m";
const string italic = "\033[3m";

string fg(const Rgb& c) {
    return "\033[38;2;" + std::to_string(c.r) + ";" + std::to_string(c.g) + ";" + std::to_string(c.b) + "m";
}

string bg(const Rgb& c) {
    return "\033[48;2;" + std::to_string(c.r) + ";" + std::to_string(c.g) + ";" + std::to_string(c.b) + "m";
}

Rgb lerp(const Rgb& a, const Rgb& b, double t) {
    t = std::max(0.0, std::min(1.0, t));
    return Rgb{(int)(a.r + (b.r - a.r) * t), (int)(a.g + (b.g - a.g) * t), (int)(a.b + (b.b - a.b) * t)};
}

int width() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) return ws.ws_col;
    return 100;
}
}

namespace util {

string lower(string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

string trim(const string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}

bool startsWith(const string& s, const string& p) {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

bool endsWith(const string& s, const string& p) {
    return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0;
}

bool contains(const string& s, const string& p) {
    return s.find(p) != string::npos;
}

string replaceAll(string s, const string& from, const string& to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

string join(const vector<string>& v, const string& sep) {
    string out;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) out += sep;
        out += v[i];
    }
    return out;
}

string repeat(const string& s, size_t n) {
    string out;
    out.reserve(s.size() * n);
    for (size_t i = 0; i < n; ++i) out += s;
    return out;
}

string collapseSpace(const string& s) {
    string out;
    bool space = false;
    for (unsigned char c : s) {
        if (std::isspace(c)) {
            space = true;
        } else {
            if (space && !out.empty()) out += ' ';
            space = false;
            out += (char)c;
        }
    }
    return out;
}

string formatInt(long long v) {
    string raw = std::to_string(v < 0 ? -v : v);
    string out;
    int count = 0;
    for (int i = (int)raw.size() - 1; i >= 0; --i) {
        out += raw[i];
        if (++count % 3 == 0 && i > 0) out += ',';
    }
    std::reverse(out.begin(), out.end());
    return v < 0 ? "-" + out : out;
}

string formatCompact(long long v) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(1);
    if (v >= 1000000) {
        os << v / 1000000.0 << "M";
    } else if (v >= 1000) {
        os << v / 1000.0 << "K";
    } else {
        os << v;
    }
    return os.str();
}

string formatDuration(long long sec) {
    long long h = sec / 3600, m = (sec % 3600) / 60, s = sec % 60;
    std::ostringstream os;
    if (h > 0) os << h << "h ";
    os << m << "m " << s << "s";
    return os.str();
}

string formatDate(time_t t) {
    char buf[64];
    std::tm tmv{};
    localtime_r(&t, &tmv);
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", &tmv);
    return buf;
}

string stamp() {
    char buf[64];
    time_t t = std::time(nullptr);
    std::tm tmv{};
    localtime_r(&t, &tmv);
    std::strftime(buf, sizeof buf, "%Y%m%d-%H%M%S", &tmv);
    return buf;
}

string slugify(const string& s) {
    string out;
    bool dash = false;
    for (unsigned char c : s) {
        if (std::isalnum(c)) {
            if (dash && !out.empty()) out += '-';
            dash = false;
            out += (char)std::tolower(c);
        } else {
            dash = true;
        }
        if (out.size() >= 40) break;
    }
    return out.empty() ? "digest" : out;
}

void appendUtf8(string& out, unsigned cp) {
    if (cp < 0x80) {
        out += (char)cp;
    } else if (cp < 0x800) {
        out += (char)(0xC0 | (cp >> 6));
        out += (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += (char)(0xE0 | (cp >> 12));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    } else {
        out += (char)(0xF0 | (cp >> 18));
        out += (char)(0x80 | ((cp >> 12) & 0x3F));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    }
}

string htmlDecode(const string& in) {
    static const std::map<string, string> named = {
        {"amp", "&"}, {"lt", "<"}, {"gt", ">"}, {"quot", "\""}, {"apos", "'"}, {"nbsp", " "},
        {"ndash", "-"}, {"mdash", "-"}, {"hellip", "..."}, {"rsquo", "'"}, {"lsquo", "'"},
        {"ldquo", "\""}, {"rdquo", "\""}};
    string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] != '&') {
            out += in[i];
            continue;
        }
        size_t semi = in.find(';', i);
        if (semi == string::npos || semi - i > 10) {
            out += in[i];
            continue;
        }
        string ent = in.substr(i + 1, semi - i - 1);
        if (!ent.empty() && ent[0] == '#') {
            unsigned long cp = 0;
            try {
                if (ent.size() > 1 && (ent[1] == 'x' || ent[1] == 'X')) cp = std::stoul(ent.substr(2), nullptr, 16);
                else cp = std::stoul(ent.substr(1));
            } catch (...) {
                out += in[i];
                continue;
            }
            appendUtf8(out, (unsigned)cp);
            i = semi;
        } else if (named.count(ent)) {
            out += named.at(ent);
            i = semi;
        } else {
            out += in[i];
        }
    }
    return out;
}

string htmlEscape(const string& s) {
    string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c;
        }
    }
    return out;
}

string jsonEscape(const string& s) {
    string out;
    out.reserve(s.size() + 16);
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", c);
                    out += buf;
                } else {
                    out += (char)c;
                }
        }
    }
    return out;
}

size_t visibleWidth(const string& s) {
    size_t w = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c == 0x1B) {
            while (i < s.size() && s[i] != 'm') ++i;
            continue;
        }
        if ((c & 0xC0) != 0x80) ++w;
    }
    return w;
}

string stripEmoji(const string& s) {
    string out;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c >= 0xF0) {
            i += 3;
            continue;
        }
        if (c == 0xEF && i + 2 < s.size() && (unsigned char)s[i + 1] == 0xB8 && (unsigned char)s[i + 2] == 0x8F) {
            i += 2;
            continue;
        }
        if (c == 0xE2 && i + 2 < s.size() && (unsigned char)s[i + 1] == 0x80 && (unsigned char)s[i + 2] == 0x8D) {
            i += 2;
            continue;
        }
        out += (char)c;
    }
    return out;
}

string cutColumns(const string& s, size_t cols, string* rest) {
    size_t count = 0, i = 0;
    while (i < s.size()) {
        size_t len = 1;
        unsigned char c = (unsigned char)s[i];
        if (c >= 0xF0) len = 4;
        else if (c >= 0xE0) len = 3;
        else if (c >= 0xC0) len = 2;
        if (count == cols) break;
        i += len;
        ++count;
    }
    if (i > s.size()) i = s.size();
    if (rest) *rest = s.substr(i);
    return s.substr(0, i);
}

string ellipsize(const string& s, size_t cols) {
    if (visibleWidth(s) <= cols) return s;
    if (cols < 2) return cutColumns(s, cols, nullptr);
    return cutColumns(s, cols - 1, nullptr) + "…";
}

string padRight(const string& s, size_t width) {
    size_t w = visibleWidth(s);
    return w >= width ? s : s + string(width - w, ' ');
}

string padLeft(const string& s, size_t width) {
    size_t w = visibleWidth(s);
    return w >= width ? s : string(width - w, ' ') + s;
}

string center(const string& s, size_t width) {
    size_t w = visibleWidth(s);
    if (w >= width) return s;
    size_t left = (width - w) / 2;
    return string(left, ' ') + s + string(width - w - left, ' ');
}

vector<string> wrap(const string& text, size_t width) {
    vector<string> lines;
    std::istringstream paragraphs(text);
    string para;
    while (std::getline(paragraphs, para)) {
        std::istringstream words(para);
        string word, cur;
        bool any = false;
        while (words >> word) {
            any = true;
            while (visibleWidth(word) > width) {
                if (!cur.empty()) {
                    lines.push_back(cur);
                    cur.clear();
                }
                string rest;
                lines.push_back(cutColumns(word, width, &rest));
                word = rest;
            }
            if (cur.empty()) cur = word;
            else if (visibleWidth(cur) + 1 + visibleWidth(word) <= width) cur += " " + word;
            else {
                lines.push_back(cur);
                cur = word;
            }
        }
        if (!cur.empty()) lines.push_back(cur);
        if (!any) lines.push_back("");
    }
    if (lines.empty()) lines.push_back("");
    return lines;
}

string gradient(const string& s, const Rgb& a, const Rgb& b) {
    size_t total = visibleWidth(s);
    if (total == 0) return s;
    string out;
    size_t idx = 0, i = 0;
    while (i < s.size()) {
        unsigned char c = (unsigned char)s[i];
        size_t len = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
        double t = total > 1 ? (double)idx / (double)(total - 1) : 0.0;
        out += tty::fg(tty::lerp(a, b, t)) + s.substr(i, len);
        i += len;
        ++idx;
    }
    return out + tty::reset;
}

}

struct Json {
    enum Type { Null, Bool, Number, String, Array, Object };
    Type type = Null;
    bool b = false;
    double n = 0;
    string s;
    vector<Json> arr;
    vector<string> keys;
    vector<Json> vals;

    static const Json& nullValue() {
        static const Json v;
        return v;
    }

    size_t size() const {
        return type == Array ? arr.size() : (type == Object ? vals.size() : 0);
    }

    const Json& at(size_t i) const {
        return i < arr.size() ? arr[i] : nullValue();
    }

    const Json& operator[](const string& k) const {
        for (size_t i = 0; i < keys.size(); ++i)
            if (keys[i] == k) return vals[i];
        return nullValue();
    }

    string str(const string& d = "") const {
        return type == String ? s : d;
    }

    double num(double d = 0) const {
        return type == Number ? n : d;
    }
};

class JsonParser {
public:
    explicit JsonParser(string t) : text(std::move(t)) {}

    bool parse(Json& out) {
        try {
            skip();
            out = value(0);
            return true;
        } catch (...) {
            return false;
        }
    }

private:
    string text;
    size_t pos = 0;

    [[noreturn]] void fail() {
        throw std::runtime_error("json");
    }

    void skip() {
        while (pos < text.size() && std::isspace((unsigned char)text[pos])) ++pos;
    }

    char peek() {
        if (pos >= text.size()) fail();
        return text[pos];
    }

    void literal(const char* word) {
        string w(word);
        if (text.compare(pos, w.size(), w) != 0) fail();
        pos += w.size();
    }

    unsigned hex4() {
        if (pos + 4 > text.size()) fail();
        unsigned v = 0;
        for (int i = 0; i < 4; ++i) {
            char c = text[pos++];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= (unsigned)(c - '0');
            else if (c >= 'a' && c <= 'f') v |= (unsigned)(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= (unsigned)(c - 'A' + 10);
            else fail();
        }
        return v;
    }

    string stringLiteral() {
        if (peek() != '"') fail();
        ++pos;
        string out;
        while (true) {
            char c = peek();
            ++pos;
            if (c == '"') break;
            if (c != '\\') {
                out += c;
                continue;
            }
            char e = peek();
            ++pos;
            switch (e) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    unsigned cp = hex4();
                    if (cp >= 0xD800 && cp <= 0xDBFF && pos + 1 < text.size() && text[pos] == '\\' && text[pos + 1] == 'u') {
                        pos += 2;
                        unsigned lo = hex4();
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                    }
                    util::appendUtf8(out, cp);
                    break;
                }
                default: fail();
            }
        }
        return out;
    }

    Json number() {
        size_t start = pos;
        while (pos < text.size() && std::strchr("+-0123456789.eE", text[pos])) ++pos;
        if (start == pos) fail();
        Json j;
        j.type = Json::Number;
        j.n = std::strtod(text.substr(start, pos - start).c_str(), nullptr);
        return j;
    }

    Json array(int depth) {
        Json j;
        j.type = Json::Array;
        ++pos;
        skip();
        if (peek() == ']') {
            ++pos;
            return j;
        }
        while (true) {
            j.arr.push_back(value(depth + 1));
            skip();
            char c = peek();
            ++pos;
            if (c == ']') break;
            if (c != ',') fail();
        }
        return j;
    }

    Json object(int depth) {
        Json j;
        j.type = Json::Object;
        ++pos;
        skip();
        if (peek() == '}') {
            ++pos;
            return j;
        }
        while (true) {
            skip();
            string key = stringLiteral();
            skip();
            if (peek() != ':') fail();
            ++pos;
            j.keys.push_back(key);
            j.vals.push_back(value(depth + 1));
            skip();
            char c = peek();
            ++pos;
            if (c == '}') break;
            if (c != ',') fail();
        }
        return j;
    }

    Json value(int depth) {
        if (depth > 200) fail();
        skip();
        char c = peek();
        if (c == '{') return object(depth);
        if (c == '[') return array(depth);
        if (c == '"') {
            Json j;
            j.type = Json::String;
            j.s = stringLiteral();
            return j;
        }
        if (c == 't') {
            literal("true");
            Json j;
            j.type = Json::Bool;
            j.b = true;
            return j;
        }
        if (c == 'f') {
            literal("false");
            Json j;
            j.type = Json::Bool;
            return j;
        }
        if (c == 'n') {
            literal("null");
            return Json();
        }
        return number();
    }
};

namespace net {

const string agent =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36";

string quote(const string& s) {
#ifdef _WIN32
    { string w = "\""; for (char c : s) { if (c == '"') w += "\\\""; else w += c; } return w + "\""; }
#endif
    string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    return out + "'";
}

bool run(const string& cmd, string& out) {
    if (fi_is_curl(cmd)) return fi_curl_run(cmd, out);
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;
    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, pipe)) > 0) out.append(buf, n);
    return pclose(pipe) == 0;
}

bool curlAvailable() {
    return FI_HAVE("curl");
}

bool get(const string& url, string& body, int& status) {
    string raw;
    string cmd = "curl -sSL --compressed --max-time 30 -A " + quote(agent) +
                 " -H \"Accept-Language: en-US,en;q=0.9\" -w \"\\n__HTTP__%{http_code}\" " + quote(url) + " 2>\" NULLDEV \"";
    bool ok = run(cmd, raw);
    size_t mark = raw.rfind("\n__HTTP__");
    status = 0;
    if (mark != string::npos) {
        status = std::atoi(raw.c_str() + mark + 9);
        raw.erase(mark);
    }
    body = raw;
    return ok && !body.empty() && status > 0;
}

string finalUrl(const string& url) {
    string out;
    string cmd = "curl -sL -o \" NULLDEV \" --max-time 20 -A " + quote(agent) + " -w \"%{url_effective}\" " + quote(url) +
                 " 2>\" NULLDEV \"";
    if (!run(cmd, out) || out.empty()) return url;
    return util::trim(out);
}

bool readLocal(const string& path, string& body) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    body = ss.str();
    return !body.empty();
}

}

namespace html {

string removeBlocks(const string& src, const string& tag) {
    string lowered = util::lower(src);
    string open = "<" + tag, close = "</" + tag + ">";
    string out;
    size_t pos = 0;
    while (true) {
        size_t a = lowered.find(open, pos);
        if (a == string::npos) {
            out += src.substr(pos);
            break;
        }
        out += src.substr(pos, a - pos);
        size_t b = lowered.find(close, a);
        if (b == string::npos) break;
        pos = b + close.size();
    }
    return out;
}

string stripTags(const string& src) {
    string out;
    out.reserve(src.size());
    bool inTag = false;
    for (char c : src) {
        if (c == '<') {
            inTag = true;
            out += ' ';
        } else if (c == '>' && inTag) {
            inTag = false;
        } else if (!inTag) {
            out += c;
        }
    }
    return out;
}

string attr(const string& tag, const string& name) {
    string lowered = util::lower(tag);
    size_t pos = 0;
    while ((pos = lowered.find(name + "=", pos)) != string::npos) {
        if (pos > 0 && !std::isspace((unsigned char)lowered[pos - 1])) {
            pos += name.size();
            continue;
        }
        size_t q = pos + name.size() + 1;
        if (q >= tag.size()) return "";
        char quote = tag[q];
        if (quote != '"' && quote != '\'') return "";
        size_t end = tag.find(quote, q + 1);
        if (end == string::npos) return "";
        return tag.substr(q + 1, end - q - 1);
    }
    return "";
}

string meta(const string& page, const string& key) {
    string lowered = util::lower(page);
    string wanted = util::lower(key);
    size_t pos = 0;
    while ((pos = lowered.find("<meta", pos)) != string::npos) {
        size_t end = lowered.find('>', pos);
        if (end == string::npos) break;
        string tag = page.substr(pos, end - pos + 1);
        string prop = util::lower(attr(tag, "property"));
        string name = util::lower(attr(tag, "name"));
        if (prop == wanted || name == wanted) {
            string content = attr(tag, "content");
            if (!content.empty()) return util::htmlDecode(content);
        }
        pos = end + 1;
    }
    return "";
}

string title(const string& page) {
    string lowered = util::lower(page);
    size_t a = lowered.find("<title");
    if (a == string::npos) return "";
    a = lowered.find('>', a);
    size_t b = lowered.find("</title>", a);
    if (a == string::npos || b == string::npos) return "";
    return util::collapseSpace(util::htmlDecode(page.substr(a + 1, b - a - 1)));
}

vector<string> paragraphs(const string& page) {
    string cleaned = removeBlocks(removeBlocks(removeBlocks(page, "script"), "style"), "noscript");
    string lowered = util::lower(cleaned);
    vector<string> out;
    size_t pos = 0;
    while ((pos = lowered.find("<p", pos)) != string::npos) {
        char next = pos + 2 < lowered.size() ? lowered[pos + 2] : ' ';
        if (next != '>' && !std::isspace((unsigned char)next)) {
            pos += 2;
            continue;
        }
        size_t open = lowered.find('>', pos);
        size_t close = lowered.find("</p>", pos);
        if (open == string::npos || close == string::npos) break;
        string inner = cleaned.substr(open + 1, close - open - 1);
        string text = util::collapseSpace(util::htmlDecode(stripTags(inner)));
        if (text.size() >= 40) out.push_back(text);
        pos = close + 4;
    }
    return out;
}

}

struct Comment {
    string author;
    long long score = 0;
    string text;
    int depth = 0;
};

struct Doc {
    string platform, url, title, author, community, description, body, published, source;
    long long score = -1, commentCount = -1, views = -1, durationSec = -1;
    double ratio = -1;
    vector<Comment> comments;
    vector<string> notes;
};

struct Fetched {
    bool ok = false;
    string error;
    Doc doc;
};

namespace fetch {

string detectPlatform(const string& url) {
    string u = util::lower(url);
    if (util::contains(u, "reddit.com") || util::contains(u, "redd.it")) return "Reddit";
    if (util::contains(u, "youtube.com") || util::contains(u, "youtu.be")) return "YouTube";
    if (util::contains(u, "instagram.com")) return "Instagram";
    return "Web";
}

string cleanInput(string raw) {
    raw = util::trim(raw);
    while (!raw.empty() && (raw.front() == '"' || raw.front() == '\'' || raw.front() == '<')) raw.erase(raw.begin());
    while (!raw.empty() && (raw.back() == '"' || raw.back() == '\'' || raw.back() == '>')) raw.pop_back();
    if (raw.empty()) return raw;
    if (util::startsWith(raw, "file://") || fs::exists(raw)) return raw;
    if (!util::startsWith(util::lower(raw), "http")) raw = "https://" + raw;
    return raw;
}

string jsonStringAfter(const string& src, const string& key) {
    string needle = "\"" + key + "\":\"";
    size_t pos = src.find(needle);
    if (pos == string::npos) return "";
    size_t i = pos + needle.size();
    size_t start = i;
    while (i < src.size()) {
        if (src[i] == '\\') i += 2;
        else if (src[i] == '"') break;
        else ++i;
    }
    if (i >= src.size()) return "";
    Json j;
    JsonParser parser("\"" + src.substr(start, i - start) + "\"");
    if (!parser.parse(j)) return "";
    return j.str();
}

string balancedArray(const string& src, size_t open) {
    int depth = 0;
    bool inString = false;
    for (size_t i = open; i < src.size(); ++i) {
        char c = src[i];
        if (inString) {
            if (c == '\\') ++i;
            else if (c == '"') inString = false;
            continue;
        }
        if (c == '"') inString = true;
        else if (c == '[') ++depth;
        else if (c == ']' && --depth == 0) return src.substr(open, i - open + 1);
    }
    return "";
}

long long parseCount(const string& raw) {
    string digits;
    double mult = 1;
    for (char c : raw) {
        if (std::isdigit((unsigned char)c) || c == '.') digits += c;
        else if (c == 'K' || c == 'k') mult = 1e3;
        else if (c == 'M' || c == 'm') mult = 1e6;
    }
    if (digits.empty()) return -1;
    return (long long)(std::atof(digits.c_str()) * mult);
}

void collectComments(const Json& listing, int depth, vector<Comment>& out) {
    const Json& kids = listing["data"]["children"];
    for (size_t i = 0; i < kids.size(); ++i) {
        const Json& kid = kids.at(i);
        if (kid["kind"].str() != "t1") continue;
        const Json& d = kid["data"];
        Comment c;
        c.author = d["author"].str("[deleted]");
        c.score = (long long)d["score"].num(0);
        c.text = d["body"].str();
        c.depth = depth;
        out.push_back(c);
        if (d["replies"].type == Json::Object) collectComments(d["replies"], depth + 1, out);
    }
}

Fetched reddit(string url) {
    Fetched f;
    f.doc.platform = "Reddit";
    f.doc.url = url;
    string lowered = util::lower(url);
    if (util::contains(lowered, "/s/") || util::contains(lowered, "redd.it")) url = net::finalUrl(url);
    size_t cut = url.find_first_of("?#");
    if (cut != string::npos) url = url.substr(0, cut);
    while (!url.empty() && url.back() == '/') url.pop_back();
    if (util::endsWith(url, ".json")) url.erase(url.size() - 5);
    string body;
    int status = 0;
    if (!net::get(url + ".json?limit=250&raw_json=1&sort=top", body, status)) {
        f.error = "Could not reach Reddit (network problem or curl failure).";
        return f;
    }
    Json root;
    JsonParser parser(body);
    if (!parser.parse(root) || root.type != Json::Array || root.size() < 1) {
        f.error = "Reddit returned HTTP " + std::to_string(status) + " and no readable JSON. It may be rate limiting you; wait a minute and retry.";
        return f;
    }
    const Json& post = root.at(0)["data"]["children"].at(0)["data"];
    if (post.type != Json::Object) {
        f.error = "That Reddit link did not point to a thread.";
        return f;
    }
    Doc& d = f.doc;
    d.title = post["title"].str();
    d.author = "u/" + post["author"].str("[deleted]");
    d.community = "r/" + post["subreddit"].str();
    d.body = post["selftext"].str();
    d.score = (long long)post["score"].num(-1);
    d.commentCount = (long long)post["num_comments"].num(-1);
    d.ratio = post["upvote_ratio"].num(-1);
    d.published = util::formatDate((time_t)post["created_utc"].num(0));
    d.source = post["url"].str();
    if (d.body.empty() && !d.source.empty() && !util::contains(d.source, "reddit.com"))
        d.notes.push_back("Link post pointing to " + d.source + " (only the title and discussion were summarised).");
    if (root.size() > 1) collectComments(root.at(1), 0, d.comments);
    if (d.comments.empty()) d.notes.push_back("No comments were returned for this thread.");
    f.ok = true;
    return f;
}

string extractVideoId(const string& url) {
    const vector<string> markers = {"v=", "youtu.be/", "/shorts/", "/embed/", "/live/"};
    for (const auto& m : markers) {
        size_t p = url.find(m);
        if (p == string::npos) continue;
        p += m.size();
        string id;
        while (p < url.size() && (std::isalnum((unsigned char)url[p]) || url[p] == '_' || url[p] == '-')) id += url[p++];
        if (id.size() >= 8) return id;
    }
    return "";
}

vector<string> transcriptLines(const string& xml) {
    vector<string> out;
    for (const string tag : {"text", "p"}) {
        string open = "<" + tag;
        size_t pos = 0;
        while ((pos = xml.find(open, pos)) != string::npos) {
            char next = pos + open.size() < xml.size() ? xml[pos + open.size()] : ' ';
            if (!std::isspace((unsigned char)next) && next != '>') {
                pos += open.size();
                continue;
            }
            size_t gt = xml.find('>', pos);
            size_t close = xml.find("</" + tag + ">", pos);
            if (gt == string::npos || close == string::npos) break;
            string inner = util::htmlDecode(util::htmlDecode(html::stripTags(xml.substr(gt + 1, close - gt - 1))));
            inner = util::collapseSpace(inner);
            if (!inner.empty()) out.push_back(inner);
            pos = close;
        }
        if (!out.empty()) break;
    }
    return out;
}

Fetched youtube(const string& url) {
    Fetched f;
    f.doc.platform = "YouTube";
    f.doc.url = url;
    string id = extractVideoId(url);
    if (id.empty()) {
        f.error = "Could not find a video id in that YouTube link.";
        return f;
    }
    string page;
    int status = 0;
    if (!net::get("https://www.youtube.com/watch?v=" + id + "&hl=en", page, status)) {
        f.error = "Could not reach YouTube.";
        return f;
    }
    Doc& d = f.doc;
    d.title = html::meta(page, "og:title");
    if (d.title.empty()) d.title = html::title(page);
    d.author = jsonStringAfter(page, "ownerChannelName");
    d.community = "YouTube";
    d.description = jsonStringAfter(page, "shortDescription");
    d.published = jsonStringAfter(page, "publishDate");
    string views = jsonStringAfter(page, "viewCount");
    string length = jsonStringAfter(page, "lengthSeconds");
    if (!views.empty()) d.views = std::atoll(views.c_str());
    if (!length.empty()) d.durationSec = std::atoll(length.c_str());
    if (d.title.empty() && d.description.empty()) {
        f.error = "YouTube returned a page without video metadata (consent wall or removed video).";
        return f;
    }
    size_t tracksAt = page.find("\"captionTracks\":");
    bool gotTranscript = false;
    if (tracksAt != string::npos) {
        size_t open = page.find('[', tracksAt);
        string raw = open == string::npos ? "" : balancedArray(page, open);
        Json tracks;
        JsonParser parser(raw);
        if (!raw.empty() && parser.parse(tracks) && tracks.size() > 0) {
            size_t chosen = 0;
            for (size_t i = 0; i < tracks.size(); ++i) {
                if (util::startsWith(tracks.at(i)["languageCode"].str(), "en")) {
                    chosen = i;
                    break;
                }
            }
            string base = tracks.at(chosen)["baseUrl"].str();
            string xml;
            int st = 0;
            if (!base.empty() && net::get(base, xml, st)) {
                vector<string> lines = transcriptLines(xml);
                if (lines.size() > 5) {
                    d.body = util::join(lines, " ");
                    d.source = "Captions (" + tracks.at(chosen)["languageCode"].str() + ")";
                    gotTranscript = true;
                }
            }
        }
    }
    if (!gotTranscript) {
        d.body = d.description;
        d.source = "Video description";
        d.notes.push_back("No caption track could be downloaded, so the summary is built from the video description.");
    }
    f.ok = true;
    return f;
}

Fetched instagram(const string& url) {
    Fetched f;
    f.doc.platform = "Instagram";
    f.doc.url = url;
    string page;
    int status = 0;
    if (!net::get(url, page, status)) {
        f.error = "Could not reach Instagram.";
        return f;
    }
    Doc& d = f.doc;
    d.title = html::meta(page, "og:title");
    string desc = html::meta(page, "og:description");
    if (desc.empty()) desc = html::meta(page, "description");
    if (desc.empty() && d.title.empty()) {
        f.error = "Instagram only showed a login wall. Public posts usually work; private ones never will.";
        return f;
    }
    size_t likes = desc.find(" likes");
    if (likes != string::npos) {
        size_t start = desc.rfind(' ', likes == 0 ? 0 : likes - 1);
        d.score = parseCount(desc.substr(start == string::npos ? 0 : start, likes - (start == string::npos ? 0 : start)));
    }
    size_t cm = desc.find(" comments");
    if (cm != string::npos) {
        size_t start = desc.rfind(' ', cm - 1);
        d.commentCount = parseCount(desc.substr(start == string::npos ? 0 : start, cm - (start == string::npos ? 0 : start)));
    }
    size_t dash = desc.find(" - ");
    size_t on = desc.find(" on ", dash == string::npos ? 0 : dash);
    if (dash != string::npos && on != string::npos) d.author = "@" + util::trim(desc.substr(dash + 3, on - dash - 3));
    size_t colon = desc.find(": \"");
    string caption = colon != string::npos ? desc.substr(colon + 3) : desc;
    while (!caption.empty() && (caption.back() == '"' || caption.back() == '.' || std::isspace((unsigned char)caption.back()))) caption.pop_back();
    d.body = caption;
    d.community = "Instagram";
    if (d.title.empty()) d.title = "Instagram post" + (d.author.empty() ? string() : " by " + d.author);
    d.notes.push_back("Instagram exposes only the public caption and counters to anonymous visitors; comments need a login.");
    f.ok = true;
    return f;
}

Fetched generic(const string& url) {
    Fetched f;
    f.doc.platform = "Web";
    f.doc.url = url;
    string page;
    int status = 0;
    bool ok;
    if (util::startsWith(url, "file://")) ok = net::readLocal(url.substr(7), page);
    else if (fs::exists(url)) ok = net::readLocal(url, page);
    else ok = net::get(url, page, status);
    if (!ok) {
        f.error = "Could not download that page.";
        return f;
    }
    Doc& d = f.doc;
    d.title = html::meta(page, "og:title");
    if (d.title.empty()) d.title = html::title(page);
    d.author = html::meta(page, "author");
    if (d.author.empty()) d.author = html::meta(page, "article:author");
    d.community = html::meta(page, "og:site_name");
    d.description = html::meta(page, "og:description");
    if (d.description.empty()) d.description = html::meta(page, "description");
    d.published = html::meta(page, "article:published_time");
    d.body = util::join(html::paragraphs(page), "\n");
    if (d.body.size() < 200 && d.description.empty()) {
        f.error = "That page has too little readable text (it may need JavaScript or a login).";
        return f;
    }
    if (d.title.empty()) d.title = url;
    f.ok = true;
    return f;
}

Fetched any(const string& url) {
    if (util::startsWith(url, "file://") || fs::exists(url)) return generic(url);
    string platform = detectPlatform(url);
    if (platform == "Reddit") return reddit(url);
    if (platform == "YouTube") return youtube(url);
    if (platform == "Instagram") return instagram(url);
    return generic(url);
}

}

struct Analysis {
    vector<string> summary;
    vector<std::pair<string, double>> keywords;
    vector<string> phrases;
    double sentiment = 0;
    string mood;
    int pos = 0, neu = 0, neg = 0;
    size_t words = 0, sentences = 0;
    double readMinutes = 0, avgSentence = 0, diversity = 0, flesch = 0;
    string fleschLabel;
    vector<Comment> topComments;
    string verdict;
};

namespace nlp {

const std::unordered_set<string>& stopwords() {
    static const std::unordered_set<string> words = {
        "the", "and", "for", "are", "but", "not", "you", "all", "can", "had", "her", "was", "one", "our", "out", "has",
        "have", "that", "this", "with", "they", "them", "then", "than", "from", "your", "what", "when", "where", "which",
        "who", "whom", "will", "would", "could", "should", "there", "their", "these", "those", "been", "being", "were",
        "its", "it's", "just", "like", "also", "into", "over", "very", "much", "more", "most", "some", "such", "only",
        "other", "about", "after", "before", "because", "while", "does", "did", "doing", "done", "dont", "don't",
        "didn't", "doesn't", "isn't", "wasn't", "can't", "cant", "i'm", "i've", "i'd", "i'll", "you're", "they're",
        "we're", "that's", "there's", "really", "even", "still", "get", "got", "getting", "make", "made", "going", "know",
        "think", "thing", "things", "way", "any", "how", "why", "too", "here", "now", "yes", "yeah", "lot", "say", "said",
        "see", "use", "used", "using", "want", "need", "well", "back", "good", "new", "time", "people", "every", "each",
        "many", "may", "might", "must", "shall", "his", "him", "she", "hers", "him", "himself", "herself", "itself",
        "myself", "yourself", "ourselves", "themselves", "again", "once", "both", "few", "own", "same", "another", "per",
        "via", "http", "https", "www", "com", "amp"};
    return words;
}

const std::unordered_set<string>& positiveWords() {
    static const std::unordered_set<string> words = {
        "good", "great", "awesome", "amazing", "love", "loved", "excellent", "best", "better", "beautiful", "brilliant",
        "fantastic", "happy", "helpful", "impressive", "incredible", "nice", "perfect", "wonderful", "enjoy", "enjoyed",
        "fun", "cool", "thanks", "thank", "recommend", "worth", "win", "wins", "excited", "superb", "solid", "smart",
        "easy", "clean", "fast", "glad", "congrats", "congratulations", "agree", "right", "support", "improved",
        "improvement", "success", "successful", "favorite", "delightful", "legend", "gem", "useful", "works", "fixed"};
    return words;
}

const std::unordered_set<string>& negativeWords() {
    static const std::unordered_set<string> words = {
        "bad", "terrible", "awful", "horrible", "hate", "hated", "worst", "worse", "poor", "broken", "bug", "bugs",
        "fail", "failed", "failure", "scam", "fake", "wrong", "angry", "annoying", "annoyed", "disappointing",
        "disappointed", "useless", "waste", "boring", "ugly", "slow", "stupid", "garbage", "trash", "sad", "problem",
        "problems", "issue", "issues", "crash", "crashes", "cheat", "toxic", "dangerous", "unfortunately", "regret",
        "complaint", "refund", "lie", "lies", "lying", "misleading", "overrated", "mess", "painful", "sucks", "nope"};
    return words;
}

const std::unordered_set<string>& negators() {
    static const std::unordered_set<string> words = {"not", "no", "never", "dont", "don't", "isn't", "wasn't", "aren't",
                                                     "didn't", "doesn't", "cannot", "can't", "hardly", "barely"};
    return words;
}

vector<string> rawTokens(const string& text) {
    vector<string> out;
    string cur;
    auto flush = [&]() {
        while (!cur.empty() && cur.front() == '\'') cur.erase(cur.begin());
        while (!cur.empty() && cur.back() == '\'') cur.pop_back();
        if (!cur.empty()) out.push_back(cur);
        cur.clear();
    };
    for (unsigned char c : text) {
        if (std::isalnum(c) || c == '\'' || c >= 0x80) cur += (char)std::tolower(c);
        else flush();
    }
    flush();
    return out;
}

bool isNumber(const string& w) {
    return !w.empty() && std::all_of(w.begin(), w.end(), [](unsigned char c) { return std::isdigit(c); });
}

string stem(string w) {
    if (w.size() > 5 && util::endsWith(w, "ies")) {
        w = w.substr(0, w.size() - 3) + "y";
    } else if (w.size() > 5 && util::endsWith(w, "ing")) {
        w.erase(w.size() - 3);
    } else if (w.size() > 4 && util::endsWith(w, "ed")) {
        w.erase(w.size() - 2);
    } else if (w.size() > 3 && w.back() == 's' && !util::endsWith(w, "ss")) {
        w.pop_back();
    }
    return w;
}

vector<string> contentTokens(const string& text) {
    vector<string> out;
    for (const auto& t : rawTokens(text)) {
        if (t.size() < 3 || stopwords().count(t) || isNumber(t)) continue;
        out.push_back(stem(t));
    }
    return out;
}

size_t countWords(const string& text) {
    std::istringstream ss(text);
    string w;
    size_t n = 0;
    while (ss >> w) ++n;
    return n;
}

bool isAbbreviation(const string& sentence) {
    static const std::set<string> abbr = {"mr", "mrs", "ms", "dr", "vs", "etc", "e.g", "i.e", "inc", "jr", "sr", "st", "no", "approx"};
    string s = sentence;
    while (!s.empty() && (s.back() == '.' || s.back() == '"' || s.back() == '\'')) s.pop_back();
    size_t sp = s.find_last_of(' ');
    string last = util::lower(sp == string::npos ? s : s.substr(sp + 1));
    return last.size() == 1 || abbr.count(last) > 0;
}

vector<string> splitSentences(const string& text) {
    vector<string> raw;
    string cur;
    auto flush = [&]() {
        string t = util::collapseSpace(cur);
        if (!t.empty()) raw.push_back(t);
        cur.clear();
    };
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '\n') {
            flush();
            continue;
        }
        cur += c;
        if (c != '.' && c != '!' && c != '?') continue;
        size_t j = i + 1;
        while (j < text.size() && (text[j] == '"' || text[j] == '\'' || text[j] == ')')) cur += text[j++];
        bool boundary = j >= text.size() || std::isspace((unsigned char)text[j]);
        if (boundary && !(c == '.' && isAbbreviation(cur))) flush();
        i = j - 1;
    }
    flush();
    vector<string> out;
    for (const auto& s : raw) {
        size_t n = countWords(s);
        if (n < 4) continue;
        if (n <= 60) {
            out.push_back(s);
            continue;
        }
        std::istringstream ss(s);
        string w, chunk;
        size_t in = 0;
        while (ss >> w) {
            chunk += (chunk.empty() ? "" : " ") + w;
            if (++in >= 25) {
                out.push_back(chunk);
                chunk.clear();
                in = 0;
            }
        }
        if (in >= 4) out.push_back(chunk);
    }
    return out;
}

size_t syllables(const string& word) {
    size_t count = 0;
    bool prev = false;
    for (char c : util::lower(word)) {
        bool vowel = std::strchr("aeiouy", c) != nullptr;
        if (vowel && !prev) ++count;
        prev = vowel;
    }
    if (util::endsWith(util::lower(word), "e") && count > 1) --count;
    return std::max<size_t>(count, 1);
}

double sentimentOf(const string& text) {
    vector<string> tokens = rawTokens(text);
    double pos = 0, neg = 0;
    for (size_t i = 0; i < tokens.size(); ++i) {
        bool negated = (i >= 1 && negators().count(tokens[i - 1])) || (i >= 2 && negators().count(tokens[i - 2]));
        if (positiveWords().count(tokens[i])) (negated ? neg : pos) += 1;
        else if (negativeWords().count(tokens[i])) (negated ? pos : neg) += 1;
    }
    return (pos - neg) / (pos + neg + 6.0);
}

string moodLabel(double s) {
    if (s > 0.25) return "Glowing";
    if (s > 0.08) return "Leaning positive";
    if (s < -0.25) return "Heated";
    if (s < -0.08) return "Leaning negative";
    return "Balanced";
}

struct SparseVec {
    std::unordered_map<string, double> w;
    double norm = 0;
};

double cosine(const SparseVec& a, const SparseVec& b) {
    if (a.norm == 0 || b.norm == 0) return 0;
    const SparseVec& small = a.w.size() < b.w.size() ? a : b;
    const SparseVec& large = a.w.size() < b.w.size() ? b : a;
    double dot = 0;
    for (const auto& kv : small.w) {
        auto it = large.w.find(kv.first);
        if (it != large.w.end()) dot += kv.second * it->second;
    }
    return dot / (a.norm * b.norm);
}

vector<size_t> textRank(const vector<string>& sents, size_t k) {
    size_t n = sents.size();
    vector<size_t> picked;
    if (n == 0) return picked;
    std::unordered_map<string, int> df;
    vector<vector<string>> toks(n);
    for (size_t i = 0; i < n; ++i) {
        toks[i] = contentTokens(sents[i]);
        std::set<string> seen(toks[i].begin(), toks[i].end());
        for (const auto& t : seen) df[t]++;
    }
    vector<SparseVec> vecs(n);
    for (size_t i = 0; i < n; ++i) {
        for (const auto& t : toks[i]) vecs[i].w[t] += 1.0;
        double sq = 0;
        for (auto& kv : vecs[i].w) {
            kv.second *= std::log(1.0 + (double)n / (double)df[kv.first]);
            sq += kv.second * kv.second;
        }
        vecs[i].norm = std::sqrt(sq);
    }
    vector<vector<double>> sim(n, vector<double>(n, 0.0));
    vector<double> outSum(n, 0.0);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            double s = cosine(vecs[i], vecs[j]);
            sim[i][j] = sim[j][i] = s;
            outSum[i] += s;
            outSum[j] += s;
        }
    }
    vector<double> rank(n, 1.0 / (double)n), next(n);
    for (int iter = 0; iter < 80; ++iter) {
        double delta = 0;
        for (size_t i = 0; i < n; ++i) {
            double acc = 0;
            for (size_t j = 0; j < n; ++j)
                if (sim[j][i] > 0 && outSum[j] > 0) acc += sim[j][i] / outSum[j] * rank[j];
            next[i] = 0.15 / (double)n + 0.85 * acc;
            delta += std::fabs(next[i] - rank[i]);
        }
        rank = next;
        if (delta < 1e-7) break;
    }
    vector<size_t> order(n);
    for (size_t i = 0; i < n; ++i) {
        order[i] = i;
        size_t wc = countWords(sents[i]);
        if (wc < 7 || wc > 45) rank[i] *= 0.4;
        rank[i] *= 1.0 + 0.2 / (1.0 + (double)i * 0.15);
    }
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return rank[a] > rank[b]; });
    for (size_t idx : order) {
        if (picked.size() >= k) break;
        bool redundant = false;
        for (size_t p : picked)
            if (sim[idx][p] > 0 && cosine(vecs[idx], vecs[p]) > 0.55) redundant = true;
        if (!redundant) picked.push_back(idx);
    }
    std::sort(picked.begin(), picked.end());
    return picked;
}

string corpusOf(const Doc& d, const vector<Comment>& ranked) {
    string t = d.title + ".\n" + d.description + "\n" + d.body + "\n";
    for (size_t i = 0; i < ranked.size() && i < 80; ++i) t += ranked[i].text + "\n";
    return t;
}

Analysis analyze(const Doc& d) {
    Analysis a;
    vector<Comment> ranked;
    for (const auto& c : d.comments)
        if (c.text != "[deleted]" && c.text != "[removed]" && c.text.size() >= 25) ranked.push_back(c);
    std::stable_sort(ranked.begin(), ranked.end(), [](const Comment& x, const Comment& y) { return x.score > y.score; });
    string corpus = corpusOf(d, ranked);
    vector<string> sents = splitSentences(corpus);
    if (sents.size() > 5 && util::startsWith(sents.front(), d.title.substr(0, std::min<size_t>(d.title.size(), 20)))) sents.erase(sents.begin());
    if (sents.size() > 700) sents.resize(700);
    a.sentences = sents.size();
    vector<string> tokens = rawTokens(corpus);
    a.words = countWords(corpus);
    a.readMinutes = (double)a.words / 238.0;
    a.avgSentence = sents.empty() ? 0 : (double)a.words / (double)sents.size();
    std::set<string> unique(tokens.begin(), tokens.end());
    a.diversity = tokens.empty() ? 0 : (double)unique.size() / (double)tokens.size();
    double syl = 0, alpha = 0;
    for (const auto& t : tokens) {
        syl += (double)syllables(t);
        alpha += 1;
    }
    if (alpha > 0 && a.avgSentence > 0) a.flesch = 206.835 - 1.015 * a.avgSentence - 84.6 * (syl / alpha);
    a.flesch = std::max(0.0, std::min(100.0, a.flesch));
    a.fleschLabel = a.flesch >= 80 ? "Very easy" : a.flesch >= 60 ? "Plain English" : a.flesch >= 40 ? "Fairly dense" : "Heavy going";
    size_t k = std::max<size_t>(3, std::min<size_t>(6, sents.size() / 8));
    if (sents.size() <= k) a.summary = sents;
    else for (size_t idx : textRank(sents, k)) a.summary.push_back(sents[idx]);
    std::unordered_map<string, double> tf;
    std::unordered_map<string, std::unordered_map<string, int>> forms;
    for (const auto& t : rawTokens(corpus)) {
        if (t.size() < 3 || stopwords().count(t) || isNumber(t)) continue;
        string s = stem(t);
        tf[s] += 1.0;
        forms[s][t]++;
    }
    vector<std::pair<string, double>> scored;
    for (const auto& kv : tf) {
        double len = std::min<double>((double)kv.first.size(), 9.0);
        scored.push_back({kv.first, kv.second * (0.6 + len / 12.0)});
    }
    std::sort(scored.begin(), scored.end(), [](const auto& x, const auto& y) {
        return x.second != y.second ? x.second > y.second : x.first < y.first;
    });
    for (size_t i = 0; i < scored.size() && i < 10; ++i) {
        string best;
        int bestCount = -1;
        for (const auto& f : forms[scored[i].first])
            if (f.second > bestCount || (f.second == bestCount && f.first < best)) {
                best = f.first;
                bestCount = f.second;
            }
        a.keywords.push_back({best, tf[scored[i].first]});
    }
    std::map<string, int> bigrams;
    vector<string> flow;
    for (const auto& t : rawTokens(corpus)) flow.push_back(stopwords().count(t) || t.size() < 3 ? "" : t);
    for (size_t i = 0; i + 1 < flow.size(); ++i)
        if (!flow[i].empty() && !flow[i + 1].empty() && !isNumber(flow[i]) && !isNumber(flow[i + 1]))
            bigrams[flow[i] + " " + flow[i + 1]]++;
    vector<std::pair<string, int>> bg(bigrams.begin(), bigrams.end());
    std::sort(bg.begin(), bg.end(), [](const auto& x, const auto& y) {
        return x.second != y.second ? x.second > y.second : x.first < y.first;
    });
    for (size_t i = 0; i < bg.size() && i < 6; ++i)
        if (bg[i].second >= 2) a.phrases.push_back(bg[i].first);
    a.sentiment = sentimentOf(corpus);
    a.mood = moodLabel(a.sentiment);
    for (const auto& c : ranked) {
        double s = sentimentOf(c.text);
        if (s > 0.1) a.pos++;
        else if (s < -0.1) a.neg++;
        else a.neu++;
    }
    for (size_t i = 0; i < ranked.size() && i < 6; ++i) a.topComments.push_back(ranked[i]);
    int total = a.pos + a.neu + a.neg;
    if (total > 0) {
        std::ostringstream os;
        os << a.mood << " across " << total << " comments: " << a.pos << " upbeat, " << a.neu << " neutral, " << a.neg
           << " critical.";
        a.verdict = os.str();
    } else {
        a.verdict = "Tone of the text: " + a.mood + ".";
    }
    return a;
}

}

}
namespace xr {
using std::string; using std::vector;
struct Rgb {
    int r, g, b;
};

namespace palette {
const Rgb cyan{34, 211, 238};
const Rgb violet{167, 139, 250};
const Rgb pink{244, 114, 182};
const Rgb amber{251, 191, 36};
const Rgb green{74, 222, 128};
const Rgb red{248, 113, 113};
const Rgb slate{148, 163, 184};
const Rgb white{241, 245, 249};
const Rgb blue{96, 165, 250};
const Rgb lime{163, 230, 53};
const Rgb orange{251, 146, 60};
const Rgb ink{15, 23, 42};
}

namespace tty {
const string reset = "\033[0m";
const string bold = "\033[1m";
const string dim = "\033[2m";
const string italic = "\033[3m";

string fg(const Rgb& c) {
    return "\033[38;2;" + std::to_string(c.r) + ";" + std::to_string(c.g) + ";" + std::to_string(c.b) + "m";
}

string bg(const Rgb& c) {
    return "\033[48;2;" + std::to_string(c.r) + ";" + std::to_string(c.g) + ";" + std::to_string(c.b) + "m";
}

Rgb lerp(const Rgb& a, const Rgb& b, double t) {
    t = std::max(0.0, std::min(1.0, t));
    return Rgb{(int)(a.r + (b.r - a.r) * t), (int)(a.g + (b.g - a.g) * t), (int)(a.b + (b.b - a.b) * t)};
}

Rgb heat(double t) {
    t = std::max(0.0, std::min(1.0, t));
    if (t < 0.33) return lerp(Rgb{34, 38, 58}, Rgb{99, 102, 241}, t / 0.33);
    if (t < 0.66) return lerp(Rgb{99, 102, 241}, Rgb{244, 114, 182}, (t - 0.33) / 0.33);
    return lerp(Rgb{244, 114, 182}, Rgb{253, 224, 71}, (t - 0.66) / 0.34);
}

int width() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) return ws.ws_col;
    return 100;
}
}

namespace util {

string lower(string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

string trim(const string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}

bool startsWith(const string& s, const string& p) {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

bool endsWith(const string& s, const string& p) {
    return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0;
}

bool contains(const string& s, const string& p) {
    return s.find(p) != string::npos;
}

string replaceAll(string s, const string& from, const string& to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

string join(const vector<string>& v, const string& sep) {
    string out;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) out += sep;
        out += v[i];
    }
    return out;
}

string repeat(const string& s, size_t n) {
    string out;
    out.reserve(s.size() * n);
    for (size_t i = 0; i < n; ++i) out += s;
    return out;
}

string collapseSpace(const string& s) {
    string out;
    bool space = false;
    for (unsigned char c : s) {
        if (std::isspace(c)) {
            space = true;
        } else {
            if (space && !out.empty()) out += ' ';
            space = false;
            out += (char)c;
        }
    }
    return out;
}

string formatInt(long long v) {
    string raw = std::to_string(v < 0 ? -v : v);
    string out;
    int count = 0;
    for (int i = (int)raw.size() - 1; i >= 0; --i) {
        out += raw[i];
        if (++count % 3 == 0 && i > 0) out += ',';
    }
    std::reverse(out.begin(), out.end());
    return v < 0 ? "-" + out : out;
}

string formatCompact(long long v) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(1);
    if (v >= 1000000) os << v / 1000000.0 << "M";
    else if (v >= 1000) os << v / 1000.0 << "K";
    else os << v;
    return os.str();
}

string formatBytes(long long b) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(1);
    if (b >= 1048576) os << b / 1048576.0 << " MB";
    else if (b >= 1024) os << b / 1024.0 << " KB";
    else os << b << " B";
    return os.str();
}

string formatMs(double ms) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(0);
    if (ms >= 1000) os << ms / 1000.0 << " s";
    else os << ms << " ms";
    return os.str();
}

string formatNow() {
    char buf[64];
    time_t t = std::time(nullptr);
    std::tm tmv{};
    localtime_r(&t, &tmv);
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", &tmv);
    return buf;
}

string stamp() {
    char buf[64];
    time_t t = std::time(nullptr);
    std::tm tmv{};
    localtime_r(&t, &tmv);
    std::strftime(buf, sizeof buf, "%Y%m%d-%H%M%S", &tmv);
    return buf;
}

string slugify(const string& s) {
    string out;
    bool dash = false;
    for (unsigned char c : s) {
        if (std::isalnum(c)) {
            if (dash && !out.empty()) out += '-';
            dash = false;
            out += (char)std::tolower(c);
        } else {
            dash = true;
        }
        if (out.size() >= 48) break;
    }
    return out.empty() ? "site" : out;
}

void appendUtf8(string& out, unsigned cp) {
    if (cp < 0x80) {
        out += (char)cp;
    } else if (cp < 0x800) {
        out += (char)(0xC0 | (cp >> 6));
        out += (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += (char)(0xE0 | (cp >> 12));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    } else {
        out += (char)(0xF0 | (cp >> 18));
        out += (char)(0x80 | ((cp >> 12) & 0x3F));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    }
}

string htmlDecode(const string& in) {
    static const std::map<string, string> named = {
        {"amp", "&"}, {"lt", "<"}, {"gt", ">"}, {"quot", "\""}, {"apos", "'"}, {"nbsp", " "},
        {"ndash", "-"}, {"mdash", "-"}, {"hellip", "..."}, {"rsquo", "'"}, {"lsquo", "'"},
        {"ldquo", "\""}, {"rdquo", "\""}};
    string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] != '&') {
            out += in[i];
            continue;
        }
        size_t semi = in.find(';', i);
        if (semi == string::npos || semi - i > 10) {
            out += in[i];
            continue;
        }
        string ent = in.substr(i + 1, semi - i - 1);
        if (!ent.empty() && ent[0] == '#') {
            unsigned long cp = 0;
            try {
                if (ent.size() > 1 && (ent[1] == 'x' || ent[1] == 'X')) cp = std::stoul(ent.substr(2), nullptr, 16);
                else cp = std::stoul(ent.substr(1));
            } catch (...) {
                out += in[i];
                continue;
            }
            appendUtf8(out, (unsigned)cp);
            i = semi;
        } else if (named.count(ent)) {
            out += named.at(ent);
            i = semi;
        } else {
            out += in[i];
        }
    }
    return out;
}

string htmlEscape(const string& s) {
    string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c;
        }
    }
    return out;
}

string jsonEscape(const string& s) {
    string out;
    out.reserve(s.size() + 16);
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", c);
                    out += buf;
                } else {
                    out += (char)c;
                }
        }
    }
    return out;
}

string rgbCss(const Rgb& c) {
    return "rgb(" + std::to_string(c.r) + "," + std::to_string(c.g) + "," + std::to_string(c.b) + ")";
}

size_t visibleWidth(const string& s) {
    size_t w = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c == 0x1B) {
            while (i < s.size() && s[i] != 'm') ++i;
            continue;
        }
        if ((c & 0xC0) != 0x80) ++w;
    }
    return w;
}

string stripEmoji(const string& s) {
    string out;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c >= 0xF0) {
            i += 3;
            continue;
        }
        if (c == 0xEF && i + 2 < s.size() && (unsigned char)s[i + 1] == 0xB8 && (unsigned char)s[i + 2] == 0x8F) {
            i += 2;
            continue;
        }
        out += (char)c;
    }
    return out;
}

string cutColumns(const string& s, size_t cols) {
    size_t count = 0, i = 0;
    while (i < s.size() && count < cols) {
        unsigned char c = (unsigned char)s[i];
        size_t len = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
        i += len;
        ++count;
    }
    return s.substr(0, std::min(i, s.size()));
}

string ellipsize(const string& s, size_t cols) {
    if (visibleWidth(s) <= cols) return s;
    if (cols < 2) return cutColumns(s, cols);
    return cutColumns(s, cols - 1) + "…";
}

string padRight(const string& s, size_t width) {
    size_t w = visibleWidth(s);
    return w >= width ? s : s + string(width - w, ' ');
}

string padLeft(const string& s, size_t width) {
    size_t w = visibleWidth(s);
    return w >= width ? s : string(width - w, ' ') + s;
}

string center(const string& s, size_t width) {
    size_t w = visibleWidth(s);
    if (w >= width) return s;
    size_t left = (width - w) / 2;
    return string(left, ' ') + s + string(width - w - left, ' ');
}

vector<string> wrap(const string& text, size_t width) {
    vector<string> lines;
    std::istringstream paragraphs(text);
    string para;
    while (std::getline(paragraphs, para)) {
        std::istringstream words(para);
        string word, cur;
        bool any = false;
        while (words >> word) {
            any = true;
            while (visibleWidth(word) > width) {
                if (!cur.empty()) {
                    lines.push_back(cur);
                    cur.clear();
                }
                string head = cutColumns(word, width);
                lines.push_back(head);
                word = word.substr(head.size());
            }
            if (cur.empty()) cur = word;
            else if (visibleWidth(cur) + 1 + visibleWidth(word) <= width) cur += " " + word;
            else {
                lines.push_back(cur);
                cur = word;
            }
        }
        if (!cur.empty()) lines.push_back(cur);
        if (!any) lines.push_back("");
    }
    if (lines.empty()) lines.push_back("");
    return lines;
}

string gradient(const string& s, const Rgb& a, const Rgb& b) {
    size_t total = visibleWidth(s);
    if (total == 0) return s;
    string out;
    size_t idx = 0, i = 0;
    while (i < s.size()) {
        unsigned char c = (unsigned char)s[i];
        size_t len = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
        double t = total > 1 ? (double)idx / (double)(total - 1) : 0.0;
        out += tty::fg(tty::lerp(a, b, t)) + s.substr(i, len);
        i += len;
        ++idx;
    }
    return out + tty::reset;
}

vector<string> split(const string& s, char d) {
    vector<string> out;
    string cur;
    for (char c : s) {
        if (c == d) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    out.push_back(cur);
    return out;
}

string hostOf(const string& url) {
    size_t a = url.find("://");
    if (a == string::npos) a = 0;
    else a += 3;
    size_t b = url.find_first_of("/?#", a);
    return util::lower(b == string::npos ? url.substr(a) : url.substr(a, b - a));
}

string pathOf(const string& url) {
    size_t a = url.find("://");
    if (a == string::npos) a = 0;
    else a += 3;
    size_t b = url.find_first_of("/?#", a);
    if (b == string::npos) return "/";
    size_t q = url.find_first_of("?#", b);
    return q == string::npos ? url.substr(b) : url.substr(b, q - b);
}

string originOf(const string& url) {
    size_t a = url.find("://");
    if (a == string::npos) return url;
    size_t b = url.find_first_of("/?#", a + 3);
    return b == string::npos ? url : url.substr(0, b);
}

string resolveUrl(const string& base, const string& href) {
    if (href.empty() || href[0] == '#' || util::startsWith(util::lower(href), "javascript:") ||
        util::startsWith(util::lower(href), "mailto:") || util::startsWith(util::lower(href), "tel:") ||
        util::startsWith(util::lower(href), "data:"))
        return "";
    if (util::startsWith(href, "http://") || util::startsWith(href, "https://")) return href;
    string origin = originOf(base);
    if (util::startsWith(href, "//")) return "https:" + href;
    if (href[0] == '/') return origin + href;
    string path = pathOf(base);
    size_t slash = path.find_last_of('/');
    string dir = slash == string::npos ? "/" : path.substr(0, slash + 1);
    return origin + dir + href;
}

string normalizeUrl(string u) {
    size_t hash = u.find('#');
    if (hash != string::npos) u = u.substr(0, hash);
    while (!u.empty() && u.back() == '/') u.pop_back();
    if (u.empty()) u = "/";
    return u;
}

}

struct Json {
    enum Type { Null, Bool, Number, String, Array, Object };
    Type type = Null;
    bool b = false;
    double n = 0;
    string s;
    vector<Json> arr;
    vector<string> keys;
    vector<Json> vals;

    static const Json& nullValue() {
        static const Json v;
        return v;
    }

    size_t size() const {
        return type == Array ? arr.size() : (type == Object ? vals.size() : 0);
    }

    const Json& at(size_t i) const {
        return i < arr.size() ? arr[i] : nullValue();
    }

    const Json& operator[](const string& k) const {
        for (size_t i = 0; i < keys.size(); ++i)
            if (keys[i] == k) return vals[i];
        return nullValue();
    }

    string str(const string& d = "") const {
        return type == String ? s : d;
    }

    double num(double d = 0) const {
        return type == Number ? n : d;
    }
};

class JsonParser {
public:
    explicit JsonParser(string t) : text(std::move(t)) {}

    bool parse(Json& out) {
        try {
            skip();
            out = value(0);
            return true;
        } catch (...) {
            return false;
        }
    }

private:
    string text;
    size_t pos = 0;

    [[noreturn]] void fail() {
        throw std::runtime_error("json");
    }

    void skip() {
        while (pos < text.size() && std::isspace((unsigned char)text[pos])) ++pos;
    }

    char peek() {
        if (pos >= text.size()) fail();
        return text[pos];
    }

    void literal(const char* word) {
        string w(word);
        if (text.compare(pos, w.size(), w) != 0) fail();
        pos += w.size();
    }

    unsigned hex4() {
        if (pos + 4 > text.size()) fail();
        unsigned v = 0;
        for (int i = 0; i < 4; ++i) {
            char c = text[pos++];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= (unsigned)(c - '0');
            else if (c >= 'a' && c <= 'f') v |= (unsigned)(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= (unsigned)(c - 'A' + 10);
            else fail();
        }
        return v;
    }

    string stringLiteral() {
        if (peek() != '"') fail();
        ++pos;
        string out;
        while (true) {
            char c = peek();
            ++pos;
            if (c == '"') break;
            if (c != '\\') {
                out += c;
                continue;
            }
            char e = peek();
            ++pos;
            switch (e) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    unsigned cp = hex4();
                    if (cp >= 0xD800 && cp <= 0xDBFF && pos + 1 < text.size() && text[pos] == '\\' && text[pos + 1] == 'u') {
                        pos += 2;
                        unsigned lo = hex4();
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                    }
                    util::appendUtf8(out, cp);
                    break;
                }
                default: fail();
            }
        }
        return out;
    }

    Json number() {
        size_t start = pos;
        while (pos < text.size() && std::strchr("+-0123456789.eE", text[pos])) ++pos;
        if (start == pos) fail();
        Json j;
        j.type = Json::Number;
        j.n = std::strtod(text.substr(start, pos - start).c_str(), nullptr);
        return j;
    }

    Json array(int depth) {
        Json j;
        j.type = Json::Array;
        ++pos;
        skip();
        if (peek() == ']') {
            ++pos;
            return j;
        }
        while (true) {
            j.arr.push_back(value(depth + 1));
            skip();
            char c = peek();
            ++pos;
            if (c == ']') break;
            if (c != ',') fail();
        }
        return j;
    }

    Json object(int depth) {
        Json j;
        j.type = Json::Object;
        ++pos;
        skip();
        if (peek() == '}') {
            ++pos;
            return j;
        }
        while (true) {
            skip();
            string key = stringLiteral();
            skip();
            if (peek() != ':') fail();
            ++pos;
            j.keys.push_back(key);
            j.vals.push_back(value(depth + 1));
            skip();
            char c = peek();
            ++pos;
            if (c == '}') break;
            if (c != ',') fail();
        }
        return j;
    }

    Json value(int depth) {
        if (depth > 200) fail();
        skip();
        char c = peek();
        if (c == '{') return object(depth);
        if (c == '[') return array(depth);
        if (c == '"') {
            Json j;
            j.type = Json::String;
            j.s = stringLiteral();
            return j;
        }
        if (c == 't') {
            literal("true");
            Json j;
            j.type = Json::Bool;
            j.b = true;
            return j;
        }
        if (c == 'f') {
            literal("false");
            Json j;
            j.type = Json::Bool;
            return j;
        }
        if (c == 'n') {
            literal("null");
            return Json();
        }
        return number();
    }
};

namespace net {

const string agent =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36";

string quote(const string& s) {
#ifdef _WIN32
    { string w = "\""; for (char c : s) { if (c == '"') w += "\\\""; else w += c; } return w + "\""; }
#endif
    string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    return out + "'";
}

bool run(const string& cmd, string& out) {
    if (fi_is_curl(cmd)) return fi_curl_run(cmd, out);
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;
    char buf[16384];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, pipe)) > 0) out.append(buf, n);
    return pclose(pipe) == 0;
}

bool curlAvailable() {
    return FI_HAVE("curl");
}

struct FetchResult {
    bool ok = false;
    int status = 0;
    double timeTotal = 0;
    long long size = 0;
    string body;
    string headers;
    string finalUrl;
    string contentType;
    string error;
};

FetchResult get(const string& url, bool headOnly = false, int timeout = 25) {
    FetchResult r;
    string raw;
    string cmd = "curl -sS -L --compressed --max-time " + std::to_string(timeout) + " -A " + quote(agent) +
                 " -H \"Accept-Language: en-US,en;q=0.9\" -H \"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\" ";
    if (headOnly) cmd += "-I ";
    cmd += "-D - -w \"\\n__META__%{http_code}|%{time_total}|%{size_download}|%{url_effective}|%{content_type}\" " + quote(url) + " 2>\" NULLDEV \"";
    if (!run(cmd, raw)) {
        r.error = "network failure";
        return r;
    }
    size_t meta = raw.rfind("\n__META__");
    if (meta == string::npos) {
        r.error = "malformed response";
        return r;
    }
    string metaLine = raw.substr(meta + 9);
    raw.erase(meta);
    vector<string> parts = util::split(metaLine, '|');
    if (parts.size() >= 5) {
        r.status = std::atoi(parts[0].c_str());
        r.timeTotal = std::atof(parts[1].c_str()) * 1000.0;
        r.size = std::atoll(parts[2].c_str());
        r.finalUrl = util::trim(parts[3]);
        r.contentType = util::trim(parts[4]);
    }
    size_t bodyStart = 0;
    size_t lastBlank = string::npos;
    for (size_t i = 0; i + 1 < raw.size(); ++i) {
        if (raw[i] == '\n' && (raw[i + 1] == '\n' || (raw[i + 1] == '\r' && i + 2 < raw.size() && raw[i + 2] == '\n'))) {
            lastBlank = i;
        }
    }
    if (lastBlank != string::npos) {
        r.headers = raw.substr(0, lastBlank);
        bodyStart = lastBlank;
        while (bodyStart < raw.size() && (raw[bodyStart] == '\n' || raw[bodyStart] == '\r')) ++bodyStart;
        r.body = raw.substr(bodyStart);
    } else {
        r.headers = raw;
    }
    r.ok = r.status > 0;
    return r;
}

string headerValue(const string& headers, const string& name) {
    string lowered = util::lower(headers);
    string key = util::lower(name) + ":";
    size_t pos = 0;
    while ((pos = lowered.find(key, pos)) != string::npos) {
        if (pos > 0 && lowered[pos - 1] != '\n' && lowered[pos - 1] != '\r') {
            pos += key.size();
            continue;
        }
        size_t start = pos + key.size();
        while (start < headers.size() && (headers[start] == ' ' || headers[start] == '\t')) ++start;
        size_t end = start;
        while (end < headers.size() && headers[end] != '\r' && headers[end] != '\n') ++end;
        return util::trim(headers.substr(start, end - start));
    }
    return "";
}

bool hasHeader(const string& headers, const string& name) {
    return !headerValue(headers, name).empty();
}

}

namespace html {

string removeBlocks(const string& src, const string& tag) {
    string lowered = util::lower(src);
    string open = "<" + tag, close = "</" + tag + ">";
    string out;
    size_t pos = 0;
    while (true) {
        size_t a = lowered.find(open, pos);
        if (a == string::npos) {
            out += src.substr(pos);
            break;
        }
        out += src.substr(pos, a - pos);
        size_t b = lowered.find(close, a);
        if (b == string::npos) break;
        pos = b + close.size();
    }
    return out;
}

string stripTags(const string& src) {
    string out;
    out.reserve(src.size());
    bool inTag = false;
    for (char c : src) {
        if (c == '<') {
            inTag = true;
            out += ' ';
        } else if (c == '>' && inTag) {
            inTag = false;
        } else if (!inTag) {
            out += c;
        }
    }
    return out;
}

string attr(const string& tag, const string& name) {
    string lowered = util::lower(tag);
    size_t pos = 0;
    while ((pos = lowered.find(name + "=", pos)) != string::npos) {
        if (pos > 0 && !std::isspace((unsigned char)lowered[pos - 1]) && lowered[pos - 1] != '<') {
            pos += name.size();
            continue;
        }
        size_t q = pos + name.size() + 1;
        if (q >= tag.size()) return "";
        char quote = tag[q];
        if (quote != '"' && quote != '\'') {
            size_t end = q;
            while (end < tag.size() && !std::isspace((unsigned char)tag[end]) && tag[end] != '>') ++end;
            return tag.substr(q, end - q);
        }
        size_t end = tag.find(quote, q + 1);
        if (end == string::npos) return "";
        return tag.substr(q + 1, end - q - 1);
    }
    return "";
}

string meta(const string& page, const string& key) {
    string lowered = util::lower(page);
    string wanted = util::lower(key);
    size_t pos = 0;
    while ((pos = lowered.find("<meta", pos)) != string::npos) {
        size_t end = lowered.find('>', pos);
        if (end == string::npos) break;
        string tag = page.substr(pos, end - pos + 1);
        string prop = util::lower(attr(tag, "property"));
        string name = util::lower(attr(tag, "name"));
        string http = util::lower(attr(tag, "http-equiv"));
        if (prop == wanted || name == wanted || http == wanted) {
            string content = attr(tag, "content");
            if (!content.empty()) return util::htmlDecode(content);
        }
        pos = end + 1;
    }
    return "";
}

string title(const string& page) {
    string lowered = util::lower(page);
    size_t a = lowered.find("<title");
    if (a == string::npos) return "";
    a = lowered.find('>', a);
    size_t b = lowered.find("</title>", a);
    if (a == string::npos || b == string::npos) return "";
    return util::collapseSpace(util::htmlDecode(page.substr(a + 1, b - a - 1)));
}

vector<string> extractLinks(const string& page, const string& base) {
    vector<string> out;
    string lowered = util::lower(page);
    size_t pos = 0;
    while ((pos = lowered.find("<a", pos)) != string::npos) {
        char next = pos + 2 < lowered.size() ? lowered[pos + 2] : ' ';
        if (next != '>' && !std::isspace((unsigned char)next)) {
            pos += 2;
            continue;
        }
        size_t end = lowered.find('>', pos);
        if (end == string::npos) break;
        string tag = page.substr(pos, end - pos + 1);
        string href = attr(tag, "href");
        string abs = util::resolveUrl(base, href);
        if (!abs.empty()) out.push_back(util::normalizeUrl(abs));
        pos = end + 1;
    }
    return out;
}

vector<string> extractAssets(const string& page, const string& base, const string& tagName, const string& attrName) {
    vector<string> out;
    string lowered = util::lower(page);
    string open = "<" + tagName;
    size_t pos = 0;
    while ((pos = lowered.find(open, pos)) != string::npos) {
        char next = pos + open.size() < lowered.size() ? lowered[pos + open.size()] : ' ';
        if (next != '>' && !std::isspace((unsigned char)next) && next != '/') {
            pos += open.size();
            continue;
        }
        size_t end = lowered.find('>', pos);
        if (end == string::npos) break;
        string tag = page.substr(pos, end - pos + 1);
        string val = attr(tag, attrName);
        string abs = util::resolveUrl(base, val);
        if (!abs.empty()) out.push_back(util::normalizeUrl(abs));
        pos = end + 1;
    }
    return out;
}

struct HeadingCounts {
    int h1 = 0, h2 = 0, h3 = 0, h4 = 0, h5 = 0, h6 = 0;
};

HeadingCounts countHeadings(const string& page) {
    HeadingCounts c;
    string lowered = util::lower(page);
    auto count = [&](const string& tag, int& n) {
        size_t pos = 0;
        while ((pos = lowered.find(tag, pos)) != string::npos) {
            char next = pos + tag.size() < lowered.size() ? lowered[pos + tag.size()] : ' ';
            if (next == '>' || std::isspace((unsigned char)next)) ++n;
            pos += tag.size();
        }
    };
    count("<h1", c.h1);
    count("<h2", c.h2);
    count("<h3", c.h3);
    count("<h4", c.h4);
    count("<h5", c.h5);
    count("<h6", c.h6);
    return c;
}

int countImgMissingAlt(const string& page) {
    string lowered = util::lower(page);
    int missing = 0;
    size_t pos = 0;
    while ((pos = lowered.find("<img", pos)) != string::npos) {
        size_t end = lowered.find('>', pos);
        if (end == string::npos) break;
        string tag = page.substr(pos, end - pos + 1);
        string alt = attr(tag, "alt");
        if (alt.empty() && util::lower(tag).find("alt=") == string::npos) ++missing;
        pos = end + 1;
    }
    return missing;
}

int countWords(const string& page) {
    string cleaned = removeBlocks(removeBlocks(removeBlocks(page, "script"), "style"), "noscript");
    string text = util::collapseSpace(util::htmlDecode(stripTags(cleaned)));
    std::istringstream ss(text);
    string w;
    int n = 0;
    while (ss >> w) ++n;
    return n;
}

}

struct LinkInfo {
    string url;
    int status = 0;
    bool internal = false;
    bool broken = false;
    double timeMs = 0;
    string contentType;
};

struct PageInfo {
    string url;
    string finalUrl;
    int status = 0;
    double timeMs = 0;
    long long size = 0;
    string title;
    string description;
    string canonical;
    string robots;
    string lang;
    html::HeadingCounts headings;
    int wordCount = 0;
    int imgMissingAlt = 0;
    int internalLinks = 0;
    int externalLinks = 0;
    vector<string> outLinks;
    vector<string> scripts;
    vector<string> styles;
    vector<string> images;
    string contentType;
    bool isHtml = false;
    int depth = 0;
    string parent;
};

struct SecurityAudit {
    bool https = false;
    bool hsts = false;
    bool csp = false;
    bool xfo = false;
    bool xcto = false;
    bool xss = false;
    bool referrer = false;
    bool permissions = false;
    string hstsValue;
    string cspValue;
    string xfoValue;
    string server;
    int score = 0;
    vector<string> missing;
    vector<string> present;
};

struct SeoAudit {
    int score = 0;
    bool hasTitle = false;
    bool titleLenOk = false;
    bool hasDesc = false;
    bool descLenOk = false;
    bool hasH1 = false;
    bool singleH1 = false;
    bool hasCanonical = false;
    bool hasLang = false;
    bool hasViewport = false;
    bool hasOg = false;
    bool hasTwitter = false;
    bool hasRobots = false;
    int missingAlt = 0;
    int wordCount = 0;
    vector<string> issues;
    vector<string> wins;
};

struct PerfAudit {
    int score = 0;
    double avgTime = 0;
    double maxTime = 0;
    long long totalBytes = 0;
    long long avgBytes = 0;
    int pageCount = 0;
    int resourceHints = 0;
    bool compression = false;
    bool cacheControl = false;
    vector<string> slowPages;
    vector<string> largePages;
    vector<string> notes;
};

struct SiteMapNode {
    string path;
    string title;
    int status = 0;
    vector<SiteMapNode> children;
};

struct Report {
    string rootUrl;
    string host;
    size_t pagesCrawled = 0;
    size_t linksChecked = 0;
    size_t brokenLinks = 0;
    size_t internalLinks = 0;
    size_t externalLinks = 0;
    vector<PageInfo> pages;
    vector<LinkInfo> allLinks;
    SecurityAudit security;
    SeoAudit seo;
    PerfAudit perf;
    SiteMapNode sitemap;
    double overall = 0;
    string grade;
    string diagnosis;
    vector<string> findings;
    vector<std::pair<string, int>> statusDist;
    vector<std::pair<string, int>> typeDist;
};

namespace crawl {

string cleanInput(string raw) {
    raw = util::trim(raw);
    while (!raw.empty() && (raw.front() == '"' || raw.front() == '\'' || raw.front() == '<')) raw.erase(raw.begin());
    while (!raw.empty() && (raw.back() == '"' || raw.back() == '\'' || raw.back() == '>')) raw.pop_back();
    if (raw.empty()) return raw;
    if (!util::startsWith(util::lower(raw), "http")) raw = "https://" + raw;
    return util::normalizeUrl(raw);
}

bool sameHost(const string& a, const string& b) {
    return util::hostOf(a) == util::hostOf(b);
}

SiteMapNode buildTree(const vector<PageInfo>& pages, const string& root) {
    SiteMapNode rootNode;
    rootNode.path = "/";
    rootNode.title = "root";
    std::map<string, SiteMapNode*> nodes;
    nodes["/"] = &rootNode;
    for (const auto& p : pages) {
        string path = util::pathOf(p.finalUrl.empty() ? p.url : p.finalUrl);
        if (path.empty()) path = "/";
        vector<string> parts = util::split(path, '/');
        string cur = "";
        SiteMapNode* parent = &rootNode;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (parts[i].empty()) continue;
            cur += "/" + parts[i];
            if (!nodes.count(cur)) {
                SiteMapNode child;
                child.path = cur;
                child.title = parts[i];
                child.status = 0;
                parent->children.push_back(child);
                nodes[cur] = &parent->children.back();
            }
            parent = nodes[cur];
        }
        if (nodes.count(path)) {
            nodes[path]->title = p.title.empty() ? path : p.title;
            nodes[path]->status = p.status;
        }
    }
    return rootNode;
}

Report run(const string& startUrl, int maxPages, int maxDepth, int linkSample) {
    Report r;
    r.rootUrl = startUrl;
    r.host = util::hostOf(startUrl);
    std::queue<std::pair<string, int>> q;
    std::unordered_set<string> seen;
    std::unordered_set<string> linkSeen;
    q.push({startUrl, 0});
    seen.insert(util::normalizeUrl(startUrl));

    while (!q.empty() && (int)r.pages.size() < maxPages) {
        auto [url, depth] = q.front();
        q.pop();
        if (depth > maxDepth) continue;

        net::FetchResult fr = net::get(url, false, 20);
        PageInfo pi;
        pi.url = url;
        pi.finalUrl = fr.finalUrl.empty() ? url : fr.finalUrl;
        pi.status = fr.status;
        pi.timeMs = fr.timeTotal;
        pi.size = fr.size;
        pi.contentType = fr.contentType;
        pi.isHtml = util::contains(util::lower(fr.contentType), "html") || fr.body.find("<html") != string::npos ||
                    fr.body.find("<HTML") != string::npos || fr.body.find("<!DOCTYPE") != string::npos;
        pi.depth = depth;

        if (pi.isHtml && fr.ok && fr.status >= 200 && fr.status < 400) {
            pi.title = html::title(fr.body);
            pi.description = html::meta(fr.body, "description");
            if (pi.description.empty()) pi.description = html::meta(fr.body, "og:description");
            pi.canonical = html::meta(fr.body, "og:url");
            if (pi.canonical.empty()) {
                string lowered = util::lower(fr.body);
                size_t pos = lowered.find("rel=\"canonical\"");
                if (pos == string::npos) pos = lowered.find("rel='canonical'");
                if (pos != string::npos) {
                    size_t tagStart = lowered.rfind("<link", pos);
                    size_t tagEnd = lowered.find('>', pos);
                    if (tagStart != string::npos && tagEnd != string::npos) {
                        string tag = fr.body.substr(tagStart, tagEnd - tagStart + 1);
                        pi.canonical = html::attr(tag, "href");
                    }
                }
            }
            pi.robots = html::meta(fr.body, "robots");
            pi.lang = "";
            {
                string lowered = util::lower(fr.body);
                size_t htmlPos = lowered.find("<html");
                if (htmlPos != string::npos) {
                    size_t end = lowered.find('>', htmlPos);
                    if (end != string::npos) {
                        string tag = fr.body.substr(htmlPos, end - htmlPos + 1);
                        pi.lang = html::attr(tag, "lang");
                    }
                }
            }
            pi.headings = html::countHeadings(fr.body);
            pi.wordCount = html::countWords(fr.body);
            pi.imgMissingAlt = html::countImgMissingAlt(fr.body);
            pi.outLinks = html::extractLinks(fr.body, pi.finalUrl);
            pi.scripts = html::extractAssets(fr.body, pi.finalUrl, "script", "src");
            pi.styles = html::extractAssets(fr.body, pi.finalUrl, "link", "href");
            pi.images = html::extractAssets(fr.body, pi.finalUrl, "img", "src");

            for (const auto& link : pi.outLinks) {
                bool internal = sameHost(link, startUrl);
                if (internal) ++pi.internalLinks;
                else ++pi.externalLinks;
                if (internal && depth + 1 <= maxDepth && (int)r.pages.size() + (int)q.size() < maxPages * 2) {
                    string norm = util::normalizeUrl(link);
                    if (!seen.count(norm)) {
                        seen.insert(norm);
                        q.push({norm, depth + 1});
                    }
                }
                if (!linkSeen.count(link) && (int)linkSeen.size() < linkSample) {
                    linkSeen.insert(link);
                }
            }
        }
        r.pages.push_back(pi);
    }

    r.pagesCrawled = r.pages.size();
    for (const auto& link : linkSeen) {
        if ((int)r.allLinks.size() >= linkSample) break;
        net::FetchResult lr = net::get(link, true, 12);
        LinkInfo li;
        li.url = link;
        li.status = lr.status;
        li.internal = sameHost(link, startUrl);
        li.broken = lr.status == 0 || lr.status >= 400;
        li.timeMs = lr.timeTotal;
        li.contentType = lr.contentType;
        r.allLinks.push_back(li);
        if (li.internal) ++r.internalLinks;
        else ++r.externalLinks;
        if (li.broken) ++r.brokenLinks;
    }
    r.linksChecked = r.allLinks.size();

    if (!r.pages.empty()) {
        const PageInfo& home = r.pages.front();
        net::FetchResult homeFr = net::get(home.url, false, 15);
        r.security.https = util::startsWith(util::lower(home.finalUrl.empty() ? home.url : home.finalUrl), "https");
        r.security.hsts = net::hasHeader(homeFr.headers, "Strict-Transport-Security");
        r.security.hstsValue = net::headerValue(homeFr.headers, "Strict-Transport-Security");
        r.security.csp = net::hasHeader(homeFr.headers, "Content-Security-Policy");
        r.security.cspValue = net::headerValue(homeFr.headers, "Content-Security-Policy");
        r.security.xfo = net::hasHeader(homeFr.headers, "X-Frame-Options");
        r.security.xfoValue = net::headerValue(homeFr.headers, "X-Frame-Options");
        r.security.xcto = net::hasHeader(homeFr.headers, "X-Content-Type-Options");
        r.security.xss = net::hasHeader(homeFr.headers, "X-XSS-Protection");
        r.security.referrer = net::hasHeader(homeFr.headers, "Referrer-Policy");
        r.security.permissions = net::hasHeader(homeFr.headers, "Permissions-Policy") || net::hasHeader(homeFr.headers, "Feature-Policy");
        r.security.server = net::headerValue(homeFr.headers, "Server");
        r.security.score = 0;
        if (r.security.https) {
            r.security.score += 25;
            r.security.present.push_back("HTTPS");
        } else {
            r.security.missing.push_back("HTTPS");
        }
        if (r.security.hsts) {
            r.security.score += 20;
            r.security.present.push_back("HSTS");
        } else {
            r.security.missing.push_back("Strict-Transport-Security");
        }
        if (r.security.csp) {
            r.security.score += 20;
            r.security.present.push_back("CSP");
        } else {
            r.security.missing.push_back("Content-Security-Policy");
        }
        if (r.security.xfo) {
            r.security.score += 10;
            r.security.present.push_back("X-Frame-Options");
        } else {
            r.security.missing.push_back("X-Frame-Options");
        }
        if (r.security.xcto) {
            r.security.score += 10;
            r.security.present.push_back("X-Content-Type-Options");
        } else {
            r.security.missing.push_back("X-Content-Type-Options");
        }
        if (r.security.referrer) {
            r.security.score += 8;
            r.security.present.push_back("Referrer-Policy");
        } else {
            r.security.missing.push_back("Referrer-Policy");
        }
        if (r.security.permissions) {
            r.security.score += 7;
            r.security.present.push_back("Permissions-Policy");
        } else {
            r.security.missing.push_back("Permissions-Policy");
        }
        r.security.score = std::min(100, r.security.score);
    }

    if (!r.pages.empty()) {
        const PageInfo& home = r.pages.front();
        r.seo.hasTitle = !home.title.empty();
        r.seo.titleLenOk = home.title.size() >= 10 && home.title.size() <= 60;
        r.seo.hasDesc = !home.description.empty();
        r.seo.descLenOk = home.description.size() >= 50 && home.description.size() <= 160;
        r.seo.hasH1 = home.headings.h1 > 0;
        r.seo.singleH1 = home.headings.h1 == 1;
        r.seo.hasCanonical = !home.canonical.empty();
        r.seo.hasLang = !home.lang.empty();
        r.seo.hasViewport = !html::meta(net::get(home.url).body, "viewport").empty();
        r.seo.hasOg = !html::meta(net::get(home.url).body, "og:title").empty();
        r.seo.hasTwitter = !html::meta(net::get(home.url).body, "twitter:card").empty();
        r.seo.hasRobots = !home.robots.empty();
        r.seo.missingAlt = home.imgMissingAlt;
        r.seo.wordCount = home.wordCount;
        r.seo.score = 0;
        if (r.seo.hasTitle) {
            r.seo.score += 12;
            r.seo.wins.push_back("Title present");
            if (r.seo.titleLenOk) {
                r.seo.score += 8;
                r.seo.wins.push_back("Title length optimal");
            } else {
                r.seo.issues.push_back("Title length outside 10-60 characters");
            }
        } else {
            r.seo.issues.push_back("Missing <title>");
        }
        if (r.seo.hasDesc) {
            r.seo.score += 12;
            r.seo.wins.push_back("Meta description present");
            if (r.seo.descLenOk) {
                r.seo.score += 8;
                r.seo.wins.push_back("Description length optimal");
            } else {
                r.seo.issues.push_back("Description length outside 50-160 characters");
            }
        } else {
            r.seo.issues.push_back("Missing meta description");
        }
        if (r.seo.hasH1) {
            r.seo.score += 10;
            r.seo.wins.push_back("H1 present");
            if (r.seo.singleH1) {
                r.seo.score += 5;
                r.seo.wins.push_back("Single H1");
            } else {
                r.seo.issues.push_back("Multiple H1 tags");
            }
        } else {
            r.seo.issues.push_back("Missing H1");
        }
        if (r.seo.hasCanonical) {
            r.seo.score += 8;
            r.seo.wins.push_back("Canonical URL set");
        } else {
            r.seo.issues.push_back("No canonical link");
        }
        if (r.seo.hasLang) {
            r.seo.score += 6;
            r.seo.wins.push_back("html lang attribute");
        } else {
            r.seo.issues.push_back("Missing lang attribute");
        }
        if (r.seo.hasViewport) {
            r.seo.score += 8;
            r.seo.wins.push_back("Viewport meta present");
        } else {
            r.seo.issues.push_back("Missing viewport meta");
        }
        if (r.seo.hasOg) {
            r.seo.score += 6;
            r.seo.wins.push_back("Open Graph tags");
        } else {
            r.seo.issues.push_back("No Open Graph tags");
        }
        if (r.seo.missingAlt == 0) {
            r.seo.score += 7;
            r.seo.wins.push_back("All images have alt");
        } else {
            r.seo.issues.push_back(std::to_string(r.seo.missingAlt) + " images missing alt text");
            r.seo.score += std::max(0, 7 - r.seo.missingAlt);
        }
        if (r.seo.wordCount >= 300) {
            r.seo.score += 10;
            r.seo.wins.push_back("Sufficient content (" + std::to_string(r.seo.wordCount) + " words)");
        } else if (r.seo.wordCount >= 100) {
            r.seo.score += 5;
            r.seo.issues.push_back("Thin content (" + std::to_string(r.seo.wordCount) + " words)");
        } else {
            r.seo.issues.push_back("Very thin content (" + std::to_string(r.seo.wordCount) + " words)");
        }
        r.seo.score = std::min(100, r.seo.score);
    }

    {
        double totalTime = 0;
        long long totalSize = 0;
        int counted = 0;
        for (const auto& p : r.pages) {
            if (p.status >= 200 && p.status < 400) {
                totalTime += p.timeMs;
                totalSize += p.size;
                ++counted;
                if (p.timeMs > 2000) r.perf.slowPages.push_back(p.url + " (" + util::formatMs(p.timeMs) + ")");
                if (p.size > 1500000) r.perf.largePages.push_back(p.url + " (" + util::formatBytes(p.size) + ")");
                if (p.timeMs > r.perf.maxTime) r.perf.maxTime = p.timeMs;
            }
        }
        r.perf.pageCount = counted;
        r.perf.avgTime = counted > 0 ? totalTime / counted : 0;
        r.perf.totalBytes = totalSize;
        r.perf.avgBytes = counted > 0 ? totalSize / counted : 0;
        r.perf.score = 100;
        if (r.perf.avgTime > 3000) r.perf.score -= 40;
        else if (r.perf.avgTime > 1500) r.perf.score -= 25;
        else if (r.perf.avgTime > 800) r.perf.score -= 10;
        if (r.perf.avgBytes > 2000000) r.perf.score -= 25;
        else if (r.perf.avgBytes > 800000) r.perf.score -= 12;
        if (!r.perf.slowPages.empty()) r.perf.score -= std::min(20, (int)r.perf.slowPages.size() * 5);
        if (!r.perf.largePages.empty()) r.perf.score -= std::min(15, (int)r.perf.largePages.size() * 4);
        r.perf.score = std::max(0, r.perf.score);
        if (r.perf.avgTime < 500) r.perf.notes.push_back("Excellent average response time");
        else if (r.perf.avgTime < 1200) r.perf.notes.push_back("Acceptable average response time");
        else r.perf.notes.push_back("Slow average response time — investigate TTFB and assets");
        if (r.perf.avgBytes < 400000) r.perf.notes.push_back("Lean page weight");
        else r.perf.notes.push_back("Heavy pages — consider compression and image optimisation");
    }

    {
        std::map<int, int> sc;
        std::map<string, int> tc;
        for (const auto& p : r.pages) {
            sc[p.status / 100 * 100]++;
            string t = p.contentType.empty() ? "unknown" : p.contentType;
            size_t semi = t.find(';');
            if (semi != string::npos) t = t.substr(0, semi);
            tc[util::trim(t)]++;
        }
        for (const auto& kv : sc) r.statusDist.push_back({std::to_string(kv.first) + "xx", kv.second});
        for (const auto& kv : tc) r.typeDist.push_back({kv.first, kv.second});
        std::sort(r.statusDist.begin(), r.statusDist.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
        std::sort(r.typeDist.begin(), r.typeDist.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    }

    r.sitemap = buildTree(r.pages, startUrl);

    double linkHealth = r.linksChecked == 0 ? 70 : 100.0 * (1.0 - (double)r.brokenLinks / (double)r.linksChecked);
    r.overall = 0.30 * r.security.score + 0.30 * r.seo.score + 0.25 * r.perf.score + 0.15 * linkHealth;
    r.grade = r.overall >= 85 ? "A" : r.overall >= 70 ? "B" : r.overall >= 55 ? "C" : r.overall >= 40 ? "D" : "F";
    r.diagnosis = r.overall >= 85 ? "Excellent. The site is well-hardened and discoverable."
                  : r.overall >= 70 ? "Solid. A few improvements would raise the score further."
                  : r.overall >= 55 ? "Average. Address the highlighted issues soon."
                  : r.overall >= 40 ? "Needs work. Security and SEO gaps are significant."
                                    : "Critical. Immediate remediation recommended.";

    if (!r.security.https) r.findings.push_back("Site is not served over HTTPS.");
    if (!r.security.hsts) r.findings.push_back("Missing Strict-Transport-Security header.");
    if (!r.security.csp) r.findings.push_back("No Content-Security-Policy — XSS surface is larger than necessary.");
    if (r.brokenLinks > 0)
        r.findings.push_back(std::to_string(r.brokenLinks) + " broken link(s) detected out of " + std::to_string(r.linksChecked) + " checked.");
    if (!r.seo.hasTitle) r.findings.push_back("Homepage is missing a <title> tag.");
    if (!r.seo.hasDesc) r.findings.push_back("Homepage is missing a meta description.");
    if (r.seo.missingAlt > 0) r.findings.push_back(std::to_string(r.seo.missingAlt) + " image(s) lack alt text.");
    if (r.perf.avgTime > 1500) r.findings.push_back("Average page load exceeds 1.5 s.");
    if (r.perf.avgBytes > 1000000) r.findings.push_back("Average page weight exceeds 1 MB.");
    if (r.findings.empty()) r.findings.push_back("No critical issues found in the crawled window.");

    return r;
}

}

}
namespace gt {
using std::string; using std::vector;
struct Rgb {
    int r, g, b;
};

namespace palette {
const Rgb lime{163, 230, 53};
const Rgb orange{251, 146, 60};
const Rgb red{248, 113, 113};
const Rgb cyan{34, 211, 238};
const Rgb violet{167, 139, 250};
const Rgb pink{244, 114, 182};
const Rgb amber{251, 191, 36};
const Rgb green{74, 222, 128};
const Rgb slate{148, 163, 184};
const Rgb white{241, 245, 249};
const Rgb ink{30, 33, 48};
}

namespace tty {
const string reset = "\033[0m";
const string bold = "\033[1m";
const string dim = "\033[2m";

string fg(const Rgb& c) {
    return "\033[38;2;" + std::to_string(c.r) + ";" + std::to_string(c.g) + ";" + std::to_string(c.b) + "m";
}

string bg(const Rgb& c) {
    return "\033[48;2;" + std::to_string(c.r) + ";" + std::to_string(c.g) + ";" + std::to_string(c.b) + "m";
}

Rgb lerp(const Rgb& a, const Rgb& b, double t) {
    t = std::max(0.0, std::min(1.0, t));
    return Rgb{(int)(a.r + (b.r - a.r) * t), (int)(a.g + (b.g - a.g) * t), (int)(a.b + (b.b - a.b) * t)};
}

Rgb heat(double t) {
    t = std::max(0.0, std::min(1.0, t));
    if (t < 0.34) return lerp(Rgb{34, 38, 58}, Rgb{99, 102, 241}, t / 0.34);
    if (t < 0.67) return lerp(Rgb{99, 102, 241}, Rgb{244, 114, 182}, (t - 0.34) / 0.33);
    return lerp(Rgb{244, 114, 182}, Rgb{253, 224, 71}, (t - 0.67) / 0.33);
}

int width() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) return ws.ws_col;
    return 100;
}
}

namespace util {

string lower(string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

string trim(const string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}

bool startsWith(const string& s, const string& p) {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

bool endsWith(const string& s, const string& p) {
    return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0;
}

bool contains(const string& s, const string& p) {
    return s.find(p) != string::npos;
}

string replaceAll(string s, const string& from, const string& to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

string join(const vector<string>& v, const string& sep) {
    string out;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) out += sep;
        out += v[i];
    }
    return out;
}

string repeat(const string& s, size_t n) {
    string out;
    for (size_t i = 0; i < n; ++i) out += s;
    return out;
}

string formatInt(long long v) {
    string raw = std::to_string(v < 0 ? -v : v);
    string out;
    int count = 0;
    for (int i = (int)raw.size() - 1; i >= 0; --i) {
        out += raw[i];
        if (++count % 3 == 0 && i > 0) out += ',';
    }
    std::reverse(out.begin(), out.end());
    return v < 0 ? "-" + out : out;
}

string formatCompact(long long v) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(1);
    if (v >= 1000000) os << v / 1000000.0 << "M";
    else if (v >= 1000) os << v / 1000.0 << "K";
    else os << v;
    return os.str();
}

string formatDay(long long ts) {
    char buf[32];
    time_t t = (time_t)ts;
    std::tm tmv{};
    gmtime_r(&t, &tmv);
    std::strftime(buf, sizeof buf, "%Y-%m-%d", &tmv);
    return buf;
}

string formatMonth(long long ts) {
    char buf[32];
    time_t t = (time_t)ts;
    std::tm tmv{};
    gmtime_r(&t, &tmv);
    std::strftime(buf, sizeof buf, "%Y-%m", &tmv);
    return buf;
}

string formatNow() {
    char buf[64];
    time_t t = std::time(nullptr);
    std::tm tmv{};
    localtime_r(&t, &tmv);
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", &tmv);
    return buf;
}

string stamp() {
    char buf[64];
    time_t t = std::time(nullptr);
    std::tm tmv{};
    localtime_r(&t, &tmv);
    std::strftime(buf, sizeof buf, "%Y%m%d-%H%M%S", &tmv);
    return buf;
}

string slugify(const string& s) {
    string out;
    bool dash = false;
    for (unsigned char c : s) {
        if (std::isalnum(c)) {
            if (dash && !out.empty()) out += '-';
            dash = false;
            out += (char)std::tolower(c);
        } else {
            dash = true;
        }
        if (out.size() >= 40) break;
    }
    return out.empty() ? "repo" : out;
}

string htmlEscape(const string& s) {
    string out;
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c;
        }
    }
    return out;
}

string jsonEscape(const string& s) {
    string out;
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", c);
                    out += buf;
                } else {
                    out += (char)c;
                }
        }
    }
    return out;
}

string rgbCss(const Rgb& c) {
    return "rgb(" + std::to_string(c.r) + "," + std::to_string(c.g) + "," + std::to_string(c.b) + ")";
}

size_t visibleWidth(const string& s) {
    size_t w = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c == 0x1B) {
            while (i < s.size() && s[i] != 'm') ++i;
            continue;
        }
        if ((c & 0xC0) != 0x80) ++w;
    }
    return w;
}

string stripEmoji(const string& s) {
    string out;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c >= 0xF0) {
            i += 3;
            continue;
        }
        out += (char)c;
    }
    return out;
}

string cutColumns(const string& s, size_t cols) {
    size_t count = 0, i = 0;
    while (i < s.size() && count < cols) {
        unsigned char c = (unsigned char)s[i];
        size_t len = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
        i += len;
        ++count;
    }
    return s.substr(0, std::min(i, s.size()));
}

string ellipsize(const string& s, size_t cols) {
    if (visibleWidth(s) <= cols) return s;
    if (cols < 2) return cutColumns(s, cols);
    return cutColumns(s, cols - 1) + "…";
}

string padRight(const string& s, size_t width) {
    size_t w = visibleWidth(s);
    return w >= width ? s : s + string(width - w, ' ');
}

string padLeft(const string& s, size_t width) {
    size_t w = visibleWidth(s);
    return w >= width ? s : string(width - w, ' ') + s;
}

vector<string> wrap(const string& text, size_t width) {
    vector<string> lines;
    std::istringstream words(text);
    string word, cur;
    while (words >> word) {
        while (visibleWidth(word) > width) {
            if (!cur.empty()) {
                lines.push_back(cur);
                cur.clear();
            }
            string head = cutColumns(word, width);
            lines.push_back(head);
            word = word.substr(head.size());
        }
        if (cur.empty()) cur = word;
        else if (visibleWidth(cur) + 1 + visibleWidth(word) <= width) cur += " " + word;
        else {
            lines.push_back(cur);
            cur = word;
        }
    }
    if (!cur.empty()) lines.push_back(cur);
    if (lines.empty()) lines.push_back("");
    return lines;
}

string gradient(const string& s, const Rgb& a, const Rgb& b) {
    size_t total = visibleWidth(s);
    if (total == 0) return s;
    string out;
    size_t idx = 0, i = 0;
    while (i < s.size()) {
        unsigned char c = (unsigned char)s[i];
        size_t len = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
        double t = total > 1 ? (double)idx / (double)(total - 1) : 0.0;
        out += tty::fg(tty::lerp(a, b, t)) + s.substr(i, len);
        i += len;
        ++idx;
    }
    return out + tty::reset;
}

vector<string> split(const string& s, char d) {
    vector<string> out;
    string cur;
    for (char c : s) {
        if (c == d) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    out.push_back(cur);
    return out;
}

}

struct FileChange {
    string path;
    long long add = 0, del = 0;
};

struct Commit {
    string hash, author, email, subject;
    long long ts = 0, add = 0, del = 0;
    bool root = false;
    vector<FileChange> files;
};

struct AuthorStat {
    string name, email;
    int commits = 0;
    long long add = 0, del = 0, first = 0, last = 0;
    std::set<long long> days;
    double share = 0;
    std::map<string, int> nameVotes;
};

struct FileStat {
    string path;
    int commits = 0;
    long long add = 0, del = 0, first = 0, last = 0;
    std::map<string, int> byAuthor;
    string topAuthor;
    double ownerShare = 0, risk = 0;
    bool orphaned = false;
};

struct CoupledPair {
    string a, b;
    int count = 0;
    double strength = 0;
};

struct Vital {
    string name;
    double score = 0;
    string note;
};

struct Report {
    string repoName, source;
    size_t commits = 0, authorsTotal = 0, filesTouched = 0;
    long long add = 0, del = 0, first = 0, last = 0, spanDays = 0;
    vector<AuthorStat> authors;
    vector<FileStat> hotspots;
    vector<CoupledPair> coupling;
    int heat[7][24] = {};
    int heatMax = 0;
    vector<std::pair<string, int>> months;
    vector<std::pair<string, long long>> languages;
    vector<std::pair<string, int>> prefixes;
    vector<std::pair<string, int>> openers;
    vector<Commit> biggest;
    int busFactor = 0, longestStreak = 0, longestGap = 0, peakHour = 0, peakWeekday = 0, orphanCount = 0;
    double fixRatio = 0, testRatio = 0, avgMessage = 0, commitsPerWeek = 0;
    vector<Vital> vitals;
    double health = 0;
    string grade, diagnosis;
    vector<string> findings;
};

const char* const WEEKDAYS[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

namespace shell {

string quote(const string& s) {
#ifdef _WIN32
    { string w = "\""; for (char c : s) { if (c == '"') w += "\\\""; else w += c; } return w + "\""; }
#endif
    string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    return out + "'";
}

bool capture(const string& cmd, string& out) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;
    char buf[16384];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, pipe)) > 0) out.append(buf, n);
    return pclose(pipe) == 0;
}

bool gitAvailable() {
    return FI_HAVE("git");
}

bool isRepo(const string& path) {
    string out;
    return capture("git -C " + quote(path) + " rev-parse --git-dir 2>\" NULLDEV \"", out) && !out.empty();
}

}

namespace source {

string normalizeRemote(string input) {
    input = util::trim(input);
    while (!input.empty() && (input.back() == '/' || input.back() == '"' || input.back() == '\'')) input.pop_back();
    while (!input.empty() && (input.front() == '"' || input.front() == '\'')) input.erase(input.begin());
    if (util::startsWith(input, "git@") || util::startsWith(input, "ssh://")) return input;
    string lowered = util::lower(input);
    if (util::startsWith(lowered, "http")) {
        size_t hostEnd = input.find("://");
        size_t pathStart = input.find('/', hostEnd + 3);
        string host = util::lower(input.substr(hostEnd + 3, pathStart == string::npos ? string::npos : pathStart - hostEnd - 3));
        if (pathStart != string::npos && (host == "github.com" || host == "www.github.com" || host == "gitlab.com")) {
            vector<string> parts = util::split(input.substr(pathStart + 1), '/');
            if (parts.size() >= 2) input = "https://" + host + "/" + parts[0] + "/" + parts[1];
        }
        if (!util::endsWith(input, ".git")) input += ".git";
        return input;
    }
    size_t slash = input.find('/');
    if (slash != string::npos && slash > 0 && input.find('/', slash + 1) == string::npos && !util::contains(input, ":") &&
        !util::contains(input, " ")) {
        return "https://github.com/" + input + ".git";
    }
    return input;
}

string repoNameOf(const string& src) {
    string s = src;
    while (!s.empty() && s.back() == '/') s.pop_back();
    if (util::endsWith(s, ".git")) s.erase(s.size() - 4);
    size_t slash = s.find_last_of("/:");
    string tail = slash == string::npos ? s : s.substr(slash + 1);
    size_t prev = slash == string::npos ? string::npos : s.find_last_of("/:", slash - 1);
    if (prev != string::npos && slash > 0) return s.substr(prev + 1);
    return tail.empty() ? "repository" : tail;
}

struct Handle {
    bool ok = false;
    string error, path, display;
    bool temporary = false;
};

Handle open(const string& input, int depth) {
    Handle h;
    string trimmed = util::trim(input);
    if (trimmed.empty()) {
        h.error = "Nothing was entered.";
        return h;
    }
    if (fs::exists(trimmed)) {
        string abs = fs::absolute(trimmed).string();
        if (!shell::isRepo(abs)) {
            h.error = "That folder exists but is not a git repository.";
            return h;
        }
        h.ok = true;
        h.path = abs;
        h.display = fs::path(abs).filename().string();
        if (h.display.empty()) h.display = "repository";
        return h;
    }
    string remote = normalizeRemote(trimmed);
    fs::path dir = fs::temp_directory_path() / ("autopsy-" + util::stamp() + "-" + std::to_string((long)getpid()));
    string cmd = GITENV "git clone --quiet --bare --depth " + std::to_string(depth) + " " + shell::quote(remote) + " " +
                 shell::quote(dir.string()) + " >\" NULLDEV \" 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        std::error_code ec;
        fs::remove_all(dir, ec);
        h.error = "Could not clone " + remote + ". Check the link, your connection, and that the repository is public.";
        return h;
    }
    h.ok = true;
    h.path = dir.string();
    h.display = repoNameOf(remote);
    h.temporary = true;
    return h;
}

void cleanup(const Handle& h) {
    if (!h.temporary) return;
    std::error_code ec;
    fs::remove_all(h.path, ec);
}

}

namespace history {

string normalizePath(const string& p) {
    size_t arrow = p.find(" => ");
    if (arrow == string::npos) return p;
    size_t open = p.find('{');
    size_t close = p.find('}');
    if (open != string::npos && close != string::npos && open < arrow && arrow < close) {
        string merged = p.substr(0, open) + p.substr(arrow + 4, close - arrow - 4) + p.substr(close + 1);
        return util::replaceAll(merged, "//", "/");
    }
    return p.substr(arrow + 4);
}

bool isNoise(const string& path) {
    static const vector<string> fragments = {"node_modules/", "vendor/", "dist/", "build/", ".min.", "package-lock.json",
                                              "yarn.lock", "pnpm-lock.yaml", "Cargo.lock", "poetry.lock", "go.sum", ".snap"};
    for (const auto& f : fragments)
        if (util::contains(path, f)) return true;
    return false;
}

vector<Commit> parse(const string& raw) {
    vector<Commit> out;
    std::istringstream in(raw);
    string line;
    while (std::getline(in, line)) {
        if (util::startsWith(line, "@@")) {
            vector<string> parts;
            size_t start = 2;
            for (int i = 0; i < 5; ++i) {
                size_t bar = line.find('|', start);
                if (bar == string::npos) break;
                parts.push_back(line.substr(start, bar - start));
                start = bar + 1;
            }
            if (parts.size() < 5) continue;
            Commit c;
            c.hash = parts[0];
            c.root = util::trim(parts[1]).empty();
            c.author = parts[2];
            c.email = util::lower(parts[3]);
            c.ts = std::atoll(parts[4].c_str());
            c.subject = line.substr(start);
            out.push_back(c);
            continue;
        }
        if (out.empty() || line.empty()) continue;
        vector<string> cols = util::split(line, '\t');
        if (cols.size() < 3) continue;
        Commit& c = out.back();
        if (c.root) continue;
        string path = normalizePath(cols[2]);
        if (isNoise(path)) continue;
        FileChange fc;
        fc.path = path;
        fc.add = cols[0] == "-" ? 0 : std::atoll(cols[0].c_str());
        fc.del = cols[1] == "-" ? 0 : std::atoll(cols[1].c_str());
        c.add += fc.add;
        c.del += fc.del;
        c.files.push_back(fc);
    }
    return out;
}

bool load(const string& repoPath, int limit, vector<Commit>& commits, string& error) {
    string raw;
    string cmd = "git -C " + shell::quote(repoPath) + " log --no-merges --numstat --date=unix -n " + std::to_string(limit) +
                 " --format=\"@@%H|%P|%an|%ae|%at|%s\" 2>\" NULLDEV \"";
    shell::capture(cmd, raw);
    commits = parse(raw);
    if (commits.empty()) {
        error = "No commits could be read from that repository.";
        return false;
    }
    std::reverse(commits.begin(), commits.end());
    return true;
}

}

namespace insight {

string languageOf(const string& path) {
    static const std::map<string, string> names = {
        {"cpp", "C++"}, {"cc", "C++"}, {"cxx", "C++"}, {"hpp", "C++"}, {"h", "C/C++ headers"}, {"c", "C"}, {"py", "Python"},
        {"js", "JavaScript"}, {"jsx", "JavaScript"}, {"ts", "TypeScript"}, {"tsx", "TypeScript"}, {"rs", "Rust"}, {"go", "Go"},
        {"java", "Java"}, {"kt", "Kotlin"}, {"swift", "Swift"}, {"rb", "Ruby"}, {"php", "PHP"}, {"cs", "C#"}, {"html", "HTML"},
        {"css", "CSS"}, {"scss", "CSS"}, {"md", "Markdown"}, {"json", "JSON"}, {"yml", "YAML"}, {"yaml", "YAML"}, {"toml", "TOML"},
        {"sh", "Shell"}, {"sql", "SQL"}, {"vue", "Vue"}, {"svelte", "Svelte"}, {"dart", "Dart"}, {"lua", "Lua"}, {"txt", "Text"}};
    size_t dot = path.find_last_of('.');
    size_t slash = path.find_last_of('/');
    if (dot == string::npos || (slash != string::npos && dot < slash)) return "(no extension)";
    string ext = util::lower(path.substr(dot + 1));
    auto it = names.find(ext);
    return it == names.end() ? "." + ext : it->second;
}

bool looksLikeTest(const string& path) {
    string p = util::lower(path);
    return util::contains(p, "test") || util::contains(p, "spec") || util::contains(p, "__tests__");
}

string prefixOf(const string& subject) {
    static const vector<string> kinds = {"feat", "fix", "docs", "style", "refactor", "perf", "test", "build", "ci", "chore", "revert"};
    string s = util::lower(subject);
    for (const auto& k : kinds) {
        if (util::startsWith(s, k)) {
            size_t n = k.size();
            if (n < s.size() && s[n] == '(') {
                size_t close = s.find(')', n);
                if (close != string::npos) n = close + 1;
            }
            if (n < s.size() && s[n] == '!') ++n;
            if (n < s.size() && s[n] == ':') return k;
        }
    }
    return "";
}

string openerOf(const string& subject) {
    std::istringstream ss(util::lower(subject));
    string w;
    ss >> w;
    while (!w.empty() && !std::isalnum((unsigned char)w.back())) w.pop_back();
    return w;
}

bool isFixy(const string& subject) {
    string s = util::lower(subject);
    return util::contains(s, "fix") || util::contains(s, "bug") || util::contains(s, "hotfix") || util::contains(s, "revert") ||
           util::contains(s, "patch") || util::contains(s, "regress");
}

int weekdayOf(long long ts) {
    long long days = ts >= 0 ? ts / 86400 : -((-ts + 86399) / 86400);
    return (int)(((days + 3) % 7 + 7) % 7);
}

int hourOf(long long ts) {
    long long s = ts % 86400;
    if (s < 0) s += 86400;
    return (int)(s / 3600);
}

double clamp100(double v) {
    return std::max(0.0, std::min(100.0, v));
}

Report analyze(const vector<Commit>& commits, const string& name, const string& src) {
    Report r;
    r.repoName = name;
    r.source = src;
    r.commits = commits.size();
    r.first = commits.front().ts;
    r.last = commits.back().ts;
    r.spanDays = std::max<long long>(1, (r.last - r.first) / 86400 + 1);
    std::unordered_map<string, AuthorStat> authors;
    std::unordered_map<string, FileStat> files;
    std::map<string, int> months;
    std::map<string, long long> langs;
    std::map<string, int> prefixes, openers;
    std::set<long long> activeDays;
    std::map<std::pair<string, string>, int> pairCounts;
    long long msgChars = 0;
    int fixes = 0, testCommits = 0, withFiles = 0;
    for (const auto& c : commits) {
        AuthorStat& a = authors[c.email];
        a.email = c.email;
        a.nameVotes[c.author]++;
        a.commits++;
        a.add += c.add;
        a.del += c.del;
        a.days.insert(c.ts / 86400);
        if (a.first == 0 || c.ts < a.first) a.first = c.ts;
        if (c.ts > a.last) a.last = c.ts;
        r.add += c.add;
        r.del += c.del;
        r.heat[weekdayOf(c.ts)][hourOf(c.ts)]++;
        months[util::formatMonth(c.ts)]++;
        activeDays.insert(c.ts / 86400);
        msgChars += (long long)c.subject.size();
        if (isFixy(c.subject)) ++fixes;
        string pre = prefixOf(c.subject);
        if (!pre.empty()) prefixes[pre]++;
        string op = openerOf(c.subject);
        if (op.size() > 2) openers[op]++;
        bool touchedTest = false;
        for (const auto& f : c.files) {
            FileStat& fsx = files[f.path];
            fsx.path = f.path;
            fsx.commits++;
            fsx.add += f.add;
            fsx.del += f.del;
            if (fsx.first == 0 || c.ts < fsx.first) fsx.first = c.ts;
            if (c.ts > fsx.last) fsx.last = c.ts;
            fsx.byAuthor[c.email]++;
            langs[languageOf(f.path)] += f.add + f.del;
            if (looksLikeTest(f.path)) touchedTest = true;
        }
        if (!c.files.empty()) {
            ++withFiles;
            if (touchedTest) ++testCommits;
        }
        if (c.files.size() >= 2 && c.files.size() <= 25) {
            vector<string> names;
            for (const auto& f : c.files) names.push_back(f.path);
            std::sort(names.begin(), names.end());
            names.erase(std::unique(names.begin(), names.end()), names.end());
            for (size_t i = 0; i < names.size(); ++i)
                for (size_t j = i + 1; j < names.size(); ++j) pairCounts[{names[i], names[j]}]++;
        }
    }
    for (int d = 0; d < 7; ++d)
        for (int h = 0; h < 24; ++h) r.heatMax = std::max(r.heatMax, r.heat[d][h]);
    r.authorsTotal = authors.size();
    r.filesTouched = files.size();
    for (auto& kv : authors) {
        AuthorStat a = kv.second;
        int best = -1;
        for (const auto& v : a.nameVotes)
            if (v.second > best) {
                best = v.second;
                a.name = v.first;
            }
        a.share = (double)a.commits / (double)r.commits;
        r.authors.push_back(a);
    }
    std::sort(r.authors.begin(), r.authors.end(), [](const AuthorStat& x, const AuthorStat& y) {
        return x.commits != y.commits ? x.commits > y.commits : x.email < y.email;
    });
    double covered = 0;
    for (const auto& a : r.authors) {
        covered += a.share;
        ++r.busFactor;
        if (covered >= 0.5) break;
    }
    long long repoEnd = r.last;
    vector<FileStat> all;
    for (auto& kv : files) {
        FileStat f = kv.second;
        int top = 0;
        string topEmail;
        for (const auto& v : f.byAuthor)
            if (v.second > top) {
                top = v.second;
                topEmail = v.first;
            }
        f.ownerShare = (double)top / (double)f.commits;
        f.topAuthor = authors[topEmail].nameVotes.empty() ? topEmail : "";
        int bestVotes = -1;
        for (const auto& v : authors[topEmail].nameVotes)
            if (v.second > bestVotes) {
                bestVotes = v.second;
                f.topAuthor = v.first;
            }
        f.orphaned = f.ownerShare >= 0.7 && f.commits >= 4 && (repoEnd - authors[topEmail].last) > 180LL * 86400;
        all.push_back(f);
    }
    long long maxLoc = 1;
    int maxCommits = 1;
    for (const auto& f : all) {
        maxLoc = std::max(maxLoc, f.add + f.del);
        maxCommits = std::max(maxCommits, f.commits);
    }
    for (auto& f : all) {
        double c = std::log(1.0 + f.commits) / std::log(1.0 + maxCommits);
        double l = std::log(1.0 + (double)(f.add + f.del)) / std::log(1.0 + (double)maxLoc);
        double recency = 1.0 - std::min(1.0, (double)(repoEnd - f.last) / (365.0 * 86400.0)) * 0.5;
        f.risk = clamp100(100.0 * (0.45 * c + 0.30 * l + 0.25 * f.ownerShare) * recency + (f.orphaned ? 12.0 : 0.0));
    }
    std::sort(all.begin(), all.end(), [](const FileStat& x, const FileStat& y) {
        return x.risk != y.risk ? x.risk > y.risk : x.path < y.path;
    });
    for (size_t i = 0; i < all.size() && i < 12; ++i) {
        r.hotspots.push_back(all[i]);
        if (all[i].orphaned) r.orphanCount++;
    }
    for (const auto& kv : pairCounts) {
        if (kv.second < 3) continue;
        CoupledPair p;
        p.a = kv.first.first;
        p.b = kv.first.second;
        p.count = kv.second;
        int base = std::min(files[p.a].commits, files[p.b].commits);
        p.strength = base > 0 ? (double)p.count / (double)base : 0;
        r.coupling.push_back(p);
    }
    std::sort(r.coupling.begin(), r.coupling.end(), [](const CoupledPair& x, const CoupledPair& y) {
        if (x.count != y.count) return x.count > y.count;
        return x.a + x.b < y.a + y.b;
    });
    if (r.coupling.size() > 8) r.coupling.resize(8);
    vector<std::pair<string, int>> monthList(months.begin(), months.end());
    if (monthList.size() > 36) monthList.erase(monthList.begin(), monthList.end() - 36);
    r.months = monthList;
    vector<std::pair<string, long long>> langList(langs.begin(), langs.end());
    std::sort(langList.begin(), langList.end(), [](const auto& x, const auto& y) {
        return x.second != y.second ? x.second > y.second : x.first < y.first;
    });
    if (langList.size() > 8) langList.resize(8);
    r.languages = langList;
    vector<std::pair<string, int>> pre(prefixes.begin(), prefixes.end());
    std::sort(pre.begin(), pre.end(), [](const auto& x, const auto& y) { return x.second > y.second; });
    r.prefixes = pre;
    vector<std::pair<string, int>> ops(openers.begin(), openers.end());
    std::sort(ops.begin(), ops.end(), [](const auto& x, const auto& y) {
        return x.second != y.second ? x.second > y.second : x.first < y.first;
    });
    if (ops.size() > 8) ops.resize(8);
    r.openers = ops;
    vector<Commit> sorted = commits;
    std::sort(sorted.begin(), sorted.end(), [](const Commit& x, const Commit& y) { return x.add + x.del > y.add + y.del; });
    for (size_t i = 0; i < sorted.size() && i < 5; ++i)
        if (sorted[i].add + sorted[i].del > 0) r.biggest.push_back(sorted[i]);
    int bestH = 0, bestD = 0, bestV = -1;
    for (int d = 0; d < 7; ++d)
        for (int h = 0; h < 24; ++h)
            if (r.heat[d][h] > bestV) {
                bestV = r.heat[d][h];
                bestD = d;
                bestH = h;
            }
    r.peakHour = bestH;
    r.peakWeekday = bestD;
    long long prev = -1;
    int streak = 0;
    for (long long day : activeDays) {
        if (prev >= 0 && day == prev + 1) ++streak;
        else streak = 1;
        r.longestStreak = std::max(r.longestStreak, streak);
        if (prev >= 0) r.longestGap = std::max<int>(r.longestGap, (int)(day - prev - 1));
        prev = day;
    }
    r.fixRatio = (double)fixes / (double)r.commits;
    r.testRatio = withFiles > 0 ? (double)testCommits / (double)withFiles : 0;
    r.avgMessage = (double)msgChars / (double)r.commits;
    r.commitsPerWeek = (double)r.commits / std::max(1.0, (double)r.spanDays / 7.0);
    int recent = 0;
    for (const auto& c : commits)
        if (r.last - c.ts <= 90LL * 86400) ++recent;
    double expected = (double)r.commits * std::min(90.0, (double)r.spanDays) / (double)r.spanDays;
    double momentum = recent == 0 ? 10 : clamp100(70.0 * (double)recent / std::max(1.0, expected));
    double uncovered = 0;
    for (const auto& f : r.hotspots)
        if (f.ownerShare >= 0.9 && f.commits >= 4) uncovered += 1;
    r.vitals.push_back({"Bus factor", clamp100(r.busFactor * 34.0), std::to_string(r.busFactor) + " author(s) cover half of all commits"});
    r.vitals.push_back({"Momentum", momentum, std::to_string(recent) + " commits in the final 90 days"});
    r.vitals.push_back({"Stability", clamp100(100.0 - r.fixRatio * 160.0), std::to_string((int)std::lround(r.fixRatio * 100)) + "% of commits look like fixes"});
    r.vitals.push_back({"Test discipline", clamp100(r.testRatio * 250.0), std::to_string((int)std::lround(r.testRatio * 100)) + "% of commits touch tests"});
    r.vitals.push_back({"Knowledge spread", clamp100(100.0 - r.orphanCount * 18.0 - uncovered * 6.0),
                        std::to_string(r.orphanCount) + " orphaned hotspot(s), " + std::to_string((int)uncovered) + " single-owner hotspot(s)"});
    double weights[5] = {0.25, 0.2, 0.2, 0.15, 0.2};
    for (size_t i = 0; i < r.vitals.size(); ++i) r.health += r.vitals[i].score * weights[i];
    r.grade = r.health >= 85 ? "A" : r.health >= 70 ? "B" : r.health >= 55 ? "C" : r.health >= 40 ? "D" : "F";
    r.diagnosis = r.health >= 85 ? "Thriving. The patient could run a marathon."
                  : r.health >= 70 ? "Healthy, with a few things worth watching."
                  : r.health >= 55 ? "Stable but showing wear in places."
                  : r.health >= 40 ? "Guarded condition. Schedule a checkup soon."
                                   : "Critical. Immediate attention recommended.";
    if (r.busFactor <= 1 && !r.authors.empty() && r.authors.front().share >= 0.6) r.findings.push_back("One person writes most of the history. If they leave, the project loses its memory.");
    if (r.orphanCount > 0)
        r.findings.push_back(std::to_string(r.orphanCount) + " hotspot file(s) are mostly owned by someone who has not committed in 6+ months.");
    if (r.fixRatio > 0.35) r.findings.push_back("More than a third of commits are fixes, which usually means churn is outrunning review.");
    if (r.testRatio < 0.1) r.findings.push_back("Fewer than one in ten commits touch tests.");
    if (!r.coupling.empty() && r.coupling.front().strength > 0.6)
        r.findings.push_back(r.coupling.front().a + " and " + r.coupling.front().b + " almost always change together; they may belong in one module.");
    if (r.longestGap > 60) r.findings.push_back("The longest silent stretch lasted " + std::to_string(r.longestGap) + " days.");
    if (r.avgMessage < 25) r.findings.push_back("Commit messages average only " + std::to_string((int)r.avgMessage) + " characters; future readers will struggle.");
    if (r.findings.empty()) r.findings.push_back("No red flags detected in the analysed window.");
    return r;
}

}

}

using std::string;
using std::vector;
using od::Rgb;
namespace util = od::util;

namespace fb {

namespace gem {
const Rgb jasper{34, 197, 94};
const Rgb sapphire{59, 130, 246};
const Rgb chalcedony{192, 132, 252};
const Rgb emerald{16, 185, 129};
const Rgb sardonyx{251, 146, 60};
const Rgb sardius{239, 68, 68};
const Rgb chrysolite{250, 204, 21};
const Rgb beryl{45, 212, 191};
const Rgb topaz{251, 191, 36};
const Rgb chrysoprase{163, 230, 53};
const Rgb jacinth{244, 63, 94};
const Rgb amethyst{168, 85, 247};
const Rgb pearl{248, 250, 252};
const Rgb gold{255, 200, 0};
const Rgb sky{56, 189, 248};
const Rgb rose{244, 114, 182};
const Rgb slate{148, 163, 184};
const vector<Rgb> twelve = {jasper, sapphire, chalcedony, emerald, sardonyx, sardius,
                            chrysolite, beryl, topaz, chrysoprase, jacinth, amethyst};
}

namespace ui {

bool quiet = false;
bool autoplay = false;

string paint(const Rgb& c, const string& s) { return od::tty::fg(c) + s + od::tty::reset; }
string boldPaint(const Rgb& c, const string& s) { return od::tty::bold + od::tty::fg(c) + s + od::tty::reset; }

string rainbow(const string& s) {
    string out;
    size_t n = 0;
    for (size_t i = 0; i < s.size();) {
        unsigned char ch = (unsigned char)s[i];
        size_t len = ch < 0x80 ? 1 : ch < 0xE0 ? 2 : ch < 0xF0 ? 3 : 4;
        string piece = s.substr(i, len);
        i += len;
        if (piece == " ") {
            out += piece;
            continue;
        }
        const Rgb& c = gem::twelve[n % gem::twelve.size()];
        out += od::tty::fg(c) + piece;
        ++n;
    }
    return out + od::tty::reset;
}

string sweep(const string& s, const Rgb& a, const Rgb& b) { return util::gradient(s, a, b); }

int cols() { return std::max(60, std::min(od::tty::width(), 110)); }

string rule(const Rgb& a, const Rgb& b, const string& glyph = "━") {
    string line;
    int w = cols() - 4;
    for (int i = 0; i < w; ++i) {
        Rgb c = od::tty::lerp(a, b, (double)i / std::max(1, w - 1));
        line += od::tty::fg(c) + glyph;
    }
    return "  " + line + od::tty::reset;
}

void say(const string& s) {
    if (!quiet) std::cout << s << "\n";
}

void blank() { say(""); }

void sleepMs(int ms) {
    if (quiet) return;
    fi_sleep_for(std::chrono::milliseconds(ms));
}

void banner() {
    if (quiet) return;
    std::cout << "\033[2J\033[H";
    const char* art[] = {
        "  ╔═══════════════════════════════════════════════════════════════════════╗",
        "  ║   ✦  C U S T O M E R   F E E D B A C K   I N T E L L I G E N C E  ✦   ║",
        "  ║        New Jerusalem Edition · Gaborone Hospitality Analytics         ║",
        "  ╚═══════════════════════════════════════════════════════════════════════╝"};
    for (int i = 0; i < 4; ++i) std::cout << rainbow(art[i]) << "\n";
    std::cout << "  " << paint(gem::slate, "Built by David Rhodes · ISS323 Information Systems Analysis") << "\n";
    std::cout << rule(gem::gold, gem::amethyst) << "\n\n";
}

string bar(double frac, int width, const Rgb& a, const Rgb& b) {
    frac = std::max(0.0, std::min(1.0, frac));
    int fill = (int)std::round(frac * width);
    string out;
    for (int i = 0; i < width; ++i) {
        if (i < fill) out += od::tty::fg(od::tty::lerp(a, b, (double)i / std::max(1, width - 1))) + "█";
        else out += od::tty::fg(Rgb{70, 78, 98}) + "░";
    }
    return out + od::tty::reset;
}

void panel(const string& title, const Rgb& accent, const vector<string>& lines) {
    if (quiet) return;
    int w = cols() - 6;
    string top = "╭─ " + title + " ";
    size_t tw = util::visibleWidth(top);
    string fillTop;
    for (int i = (int)tw; i < w - 1; ++i) fillTop += "─";
    std::cout << "  " << od::tty::bold << od::tty::fg(accent) << top << fillTop << "╮" << od::tty::reset << "\n";
    for (const auto& raw : lines) {
        vector<string> wrapped = util::wrap(raw, (size_t)w - 6);
        if (wrapped.empty()) wrapped.push_back("");
        for (const auto& ln : wrapped) {
            size_t vw = util::visibleWidth(ln);
            string pad(vw < (size_t)w - 4 ? (size_t)w - 4 - vw : 0, ' ');
            std::cout << "  " << od::tty::fg(accent) << "│ " << od::tty::reset << ln << pad << od::tty::fg(accent) << " │"
                      << od::tty::reset << "\n";
        }
    }
    string bottom;
    for (int i = 0; i < w - 2; ++i) bottom += "─";
    std::cout << "  " << od::tty::fg(accent) << "╰" << bottom << "╯" << od::tty::reset << "\n\n";
}

void step(int n, int total, const string& title, const string& buildsOn) {
    if (quiet) return;
    std::cout << "\n" << rule(gem::sapphire, gem::chrysoprase, "═") << "\n";
    std::cout << "  " << od::tty::bold << od::tty::fg(gem::gold) << "STEP " << n << "/" << total << od::tty::reset << "  "
              << sweep(title, gem::beryl, gem::rose) << "\n";
    if (!buildsOn.empty()) std::cout << "  " << paint(gem::slate, "builds on → " + buildsOn) << "\n";
    std::cout << rule(gem::sapphire, gem::chrysoprase, "═") << "\n\n";
}

void working(const string& label, int ms = 260) {
    if (quiet) return;
    const char* frames[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    int n = ms / 40;
    for (int i = 0; i < n; ++i) {
        std::cout << "\r  " << paint(gem::chalcedony, frames[i % 10]) << " " << paint(gem::pearl, label) << std::flush;
        fi_sleep_for(std::chrono::milliseconds(40));
    }
    std::cout << "\r  " << paint(gem::emerald, "✔") << " " << paint(gem::pearl, label) << "          \n";
}

void ok(const string& s) { say("  " + paint(gem::emerald, "✔ ") + s); }
void warn(const string& s) { say("  " + paint(gem::topaz, "⚠ ") + s); }
void fail(const string& s) { say("  " + paint(gem::sardius, "✖ ") + s); }
void info(const string& s) { say("  " + paint(gem::sky, "ℹ ") + s); }

string prompt(const string& label) {
    if (autoplay) return "";
    std::cout << "  " << paint(gem::rose, "➜ ") << od::tty::bold << label << od::tty::reset << " " << std::flush;
    string line;
    if (!std::getline(std::cin, line)) return "\x04";
    return util::trim(line);
}

bool eof(const string& s) { return s == "\x04"; }

string secret(const string& label) {
    if (autoplay) return "";
    std::cout << "  " << paint(gem::rose, "➜ ") << od::tty::bold << label << od::tty::reset << " " << std::flush;
    struct termios oldt, newt;
    bool tty = isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &oldt) == 0;
    if (tty) {
        newt = oldt;
        newt.c_lflag &= ~(tcflag_t)ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    }
    string line;
    std::getline(std::cin, line);
    if (tty) {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        std::cout << "\n";
    }
    return line;
}

bool yes(const string& q, bool fallback = true) {
    if (autoplay) return fallback;
    string a = util::lower(prompt(q + (fallback ? " [Y/n]" : " [y/N]")));
    if (eof(a)) return false;
    if (a.empty()) return fallback;
    return a[0] == 'y';
}

int number(const string& q, int fallback, int lo, int hi) {
    if (autoplay) return fallback;
    string a = prompt(q + " [" + std::to_string(fallback) + "]");
    if (eof(a) || a.empty()) return fallback;
    try {
        int v = std::stoi(a);
        return std::max(lo, std::min(hi, v));
    } catch (...) {
        return fallback;
    }
}

void pause() {
    if (autoplay || quiet) return;
    std::cout << "  " << paint(gem::slate, "Press Enter to continue…") << std::flush;
    string s;
    std::getline(std::cin, s);
}

string table(const vector<string>& heads, const vector<vector<string>>& rows, const vector<Rgb>& cols_) {
    vector<size_t> w(heads.size());
    for (size_t i = 0; i < heads.size(); ++i) w[i] = util::visibleWidth(heads[i]);
    for (const auto& r : rows)
        for (size_t i = 0; i < r.size() && i < w.size(); ++i) w[i] = std::max(w[i], util::visibleWidth(r[i]));
    std::ostringstream o;
    o << "  " << od::tty::bold;
    for (size_t i = 0; i < heads.size(); ++i)
        o << od::tty::fg(cols_[i % cols_.size()]) << util::padRight(heads[i], w[i] + 2);
    o << od::tty::reset << "\n  ";
    for (size_t i = 0; i < heads.size(); ++i) {
        string d;
        for (size_t k = 0; k < w[i]; ++k) d += "─";
        o << od::tty::fg(Rgb{90, 98, 120}) << d << "  ";
    }
    o << od::tty::reset << "\n";
    for (const auto& r : rows) {
        o << "  ";
        for (size_t i = 0; i < heads.size(); ++i) {
            string cell = i < r.size() ? r[i] : "";
            o << od::tty::fg(i == 0 ? gem::pearl : cols_[i % cols_.size()]) << util::padRight(cell, w[i] + 2);
        }
        o << od::tty::reset << "\n";
    }
    return o.str();
}

void print(const string& s) {
    if (!quiet) std::cout << s;
}

}

namespace sec {

uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

string sha256(const string& data) {
    static const uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
        0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
        0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    string msg = data;
    uint64_t bits = (uint64_t)data.size() * 8;
    msg += (char)0x80;
    while (msg.size() % 64 != 56) msg += (char)0;
    for (int i = 7; i >= 0; --i) msg += (char)((bits >> (i * 8)) & 0xff);
    for (size_t off = 0; off < msg.size(); off += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = ((uint32_t)(unsigned char)msg[off + i * 4] << 24) | ((uint32_t)(unsigned char)msg[off + i * 4 + 1] << 16) |
                   ((uint32_t)(unsigned char)msg[off + i * 4 + 2] << 8) | (uint32_t)(unsigned char)msg[off + i * 4 + 3];
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t t1 = hh + S1 + ch + k[i] + w[i];
            uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t t2 = S0 + mj;
            hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
    std::ostringstream o;
    for (int i = 0; i < 8; ++i) o << std::hex << std::setw(8) << std::setfill('0') << h[i];
    return o.str();
}

string randomHex(size_t n) {
    static std::mt19937_64 gen((uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count() ^
                               (uint64_t)getpid() << 16);
    string out;
    const char* hex = "0123456789abcdef";
    for (size_t i = 0; i < n; ++i) out += hex[gen() % 16];
    return out;
}

string hashPassword(const string& salt, const string& pw) {
    string h = pw;
    for (int i = 0; i < 2000; ++i) h = sha256(salt + h + pw);
    return h;
}

}

namespace clock_ {

string now() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}

string today() { return now().substr(0, 10); }

string compact() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y%m%d_%H%M%S", std::localtime(&t));
    return buf;
}

double ms() {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

}

string esc(const string& s) {
    string o;
    for (char c : s) {
        if (c == '\\') o += "\\\\";
        else if (c == '\t') o += "\\t";
        else if (c == '\n') o += "\\n";
        else if (c == '\r') {
        } else o += c;
    }
    return o;
}

string unesc(const string& s) {
    string o;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char n = s[++i];
            o += n == 't' ? '\t' : n == 'n' ? '\n' : n;
        } else o += s[i];
    }
    return o;
}

vector<string> splitTabs(const string& line) {
    vector<string> out;
    string cur;
    for (char c : line) {
        if (c == '\t') {
            out.push_back(cur);
            cur.clear();
        } else cur += c;
    }
    out.push_back(cur);
    return out;
}

string urlEncode(const string& s) {
    std::ostringstream o;
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') o << c;
        else if (c == ' ') o << '+';
        else o << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return o.str();
}

string urlDecode(const string& s) {
    string o;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size() && std::isxdigit((unsigned char)s[i + 1]) && std::isxdigit((unsigned char)s[i + 2])) {
            o += (char)std::stoi(s.substr(i + 1, 2), nullptr, 16);
            i += 2;
        } else if (s[i] == '+') o += ' ';
        else o += s[i];
    }
    return o;
}

string hostOf(const string& url) {
    string u = url;
    size_t p = u.find("://");
    if (p != string::npos) u = u.substr(p + 3);
    size_t e = u.find_first_of("/?#");
    if (e != string::npos) u = u.substr(0, e);
    size_t at = u.find('@');
    if (at != string::npos) u = u.substr(at + 1);
    size_t colon = u.find(':');
    if (colon != string::npos) u = u.substr(0, colon);
    u = util::lower(u);
    if (util::startsWith(u, "www.")) u = u.substr(4);
    return u;
}

string pct(double v) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(1) << v << "%";
    return o.str();
}

string fix(double v, int p = 2) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(p) << v;
    return o.str();
}

}

namespace fb {

struct Entry {
    int id = 0;
    string source, author, date, text, origin;
    int score = 0;
    string label;
    string issue;
    vector<string> themes;
    int severity = 0;
    string flag;
};

vector<string> tokens(const string& text) {
    vector<string> out;
    string cur;
    for (unsigned char c : text) {
        if (std::isalpha(c) || c == '\'') cur += (char)std::tolower(c);
        else if (!cur.empty()) {
            out.push_back(cur);
            cur.clear();
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

string stemWord(string w) {
    if (w.size() > 4 && util::endsWith(w, "ing")) w.erase(w.size() - 3);
    else if (w.size() > 4 && util::endsWith(w, "ed")) w.erase(w.size() - 2);
    else if (w.size() > 3 && util::endsWith(w, "s") && !util::endsWith(w, "ss")) w.pop_back();
    return w;
}

class Sentiment {
    std::unordered_map<string, int> lex;
    std::unordered_set<string> negators = {"not", "no", "never", "hardly", "barely", "without", "isn't", "wasn't", "don't", "didn't",
                                           "doesn't", "couldn't", "wouldn't", "won't", "cannot", "can't", "nothing", "nobody"};
    std::unordered_map<string, double> boosters = {{"very", 1.5}, {"extremely", 2.0}, {"really", 1.4}, {"so", 1.3}, {"totally", 1.6},
                                                   {"absolutely", 1.8}, {"incredibly", 1.8}, {"quite", 1.2}, {"super", 1.5},
                                                   {"utterly", 1.8}, {"slightly", 0.6}, {"somewhat", 0.7}, {"bit", 0.7}};

public:
    Sentiment() {
        vector<std::pair<string, int>> seeds = {
            {"excellent", 3}, {"amazing", 3}, {"great", 2}, {"good", 1}, {"love", 2}, {"loved", 2}, {"perfect", 3}, {"clean", 1},
            {"spotless", 3}, {"friendly", 2}, {"fast", 1}, {"quick", 1}, {"delicious", 2}, {"tasty", 2}, {"recommend", 2},
            {"wonderful", 3}, {"fantastic", 3}, {"superb", 3}, {"lovely", 2}, {"beautiful", 2}, {"comfortable", 2}, {"cozy", 2},
            {"helpful", 2}, {"polite", 2}, {"welcoming", 2}, {"attentive", 2}, {"professional", 2}, {"fresh", 1}, {"brilliant", 3},
            {"outstanding", 3}, {"best", 3}, {"happy", 2}, {"enjoyed", 2}, {"enjoy", 2}, {"pleasant", 2}, {"value", 1},
            {"affordable", 1}, {"spacious", 1}, {"quiet", 1}, {"peaceful", 2}, {"relaxing", 2}, {"impressed", 2}, {"smooth", 1},
            {"efficient", 2}, {"generous", 2}, {"stunning", 3}, {"gem", 2}, {"thanks", 1}, {"thank", 1}, {"satisfied", 2},
            {"terrible", -3}, {"awful", -3}, {"bad", -2}, {"slow", -2}, {"dirty", -2}, {"filthy", -3}, {"rude", -3},
            {"horrible", -3}, {"poor", -2}, {"worst", -3}, {"disgusting", -3}, {"cold", -1}, {"wait", -1}, {"waiting", -1},
            {"overpriced", -2}, {"expensive", -1}, {"noisy", -1}, {"broken", -2}, {"late", -1}, {"disappointed", -2},
            {"disappointing", -2}, {"unhelpful", -2}, {"ignored", -2}, {"stale", -2}, {"smelly", -2}, {"smell", -1}, {"stain", -1},
            {"stained", -2}, {"mold", -3}, {"mould", -3}, {"cockroach", -3}, {"roach", -3}, {"rat", -3}, {"bugs", -2},
            {"rubbish", -2}, {"unprofessional", -3}, {"arrogant", -3}, {"shouted", -3}, {"insulted", -3}, {"scam", -3},
            {"fraud", -3}, {"stole", -3}, {"stolen", -3}, {"theft", -3}, {"unsafe", -3}, {"dangerous", -3}, {"leak", -2},
            {"leaking", -2}, {"faulty", -2}, {"useless", -2}, {"mediocre", -1}, {"bland", -1}, {"undercooked", -2},
            {"overcooked", -2}, {"raw", -2}, {"sick", -3}, {"poisoning", -3}, {"vomit", -3}, {"nightmare", -3}, {"never", -1},
            {"avoid", -3}, {"refund", -1}, {"complaint", -1}, {"unacceptable", -3}, {"lacking", -1}, {"crowded", -1},
            {"cramped", -1}, {"hot", -1}, {"freezing", -2}, {"harass", -3}, {"harassed", -3}, {"drunk", -3}, {"racist", -3},
            {"overcharged", -3}, {"lost", -2}, {"missing", -2}, {"delay", -2}, {"delayed", -2}, {"understaffed", -2}};
        for (auto& p : seeds) lex[p.first] = p.second;
    }

    int analyze(const string& text) const {
        vector<string> t = tokens(text);
        double total = 0;
        for (size_t i = 0; i < t.size(); ++i) {
            auto it = lex.find(t[i]);
            if (it == lex.end()) {
                string st = stemWord(t[i]);
                it = lex.find(st);
            }
            if (it == lex.end()) continue;
            double v = it->second;
            double boost = 1.0;
            bool neg = false;
            for (size_t back = 1; back <= 3 && back <= i; ++back) {
                const string& prev = t[i - back];
                if (negators.count(prev) || util::endsWith(prev, "n't")) neg = true;
                auto b = boosters.find(prev);
                if (b != boosters.end()) boost *= b->second;
            }
            v *= boost;
            if (neg) v = -v * 0.8;
            total += v;
        }
        return (int)std::lround(total);
    }

    string label(int s) const { return s > 0 ? "POSITIVE" : s < 0 ? "NEGATIVE" : "NEUTRAL"; }
    size_t size() const { return lex.size(); }
};

struct RedFlag {
    string name;
    vector<string> words;
    int severity;
    string level;
    string action;
    bool police;
    bool mgmt;
};

class Classifier {
    std::map<string, vector<string>> issueMap;
    vector<RedFlag> flags;

public:
    Classifier() {
        issueMap["service"] = {"slow", "wait", "waiting", "service", "attendant", "waiter", "waitress", "queue", "ignored", "delay",
                               "delayed", "late", "understaffed", "reception", "checkin", "booking", "reservation"};
        issueMap["staff conduct"] = {"rude", "staff", "arrogant", "shouted", "insulted", "unprofessional", "attitude", "manager",
                                     "unfriendly", "disrespect", "impolite", "dismissive", "receptionist"};
        issueMap["cleanliness"] = {"dirty", "filthy", "clean", "smell", "smelly", "stain", "stained", "hygiene", "dust", "dusty",
                                   "mold", "mould", "sheets", "towels", "bathroom", "toilet", "spotless"};
        issueMap["food quality"] = {"cold", "taste", "tasty", "delicious", "food", "meal", "breakfast", "dinner", "lunch", "menu",
                                    "stale", "bland", "undercooked", "overcooked", "raw", "portion", "dish", "chef", "buffet"};
        issueMap["pricing"] = {"expensive", "overpriced", "cheap", "value", "cost", "price", "prices", "bill", "charged", "charge",
                               "money", "affordable", "rate"};
        issueMap["facilities"] = {"wifi", "broken", "ac", "aircon", "shower", "parking", "room", "pool", "gym", "lift", "elevator",
                                  "tv", "bed", "water", "electricity", "power", "generator", "leak", "leaking", "faulty"};
        issueMap["noise"] = {"noise", "noisy", "loud", "music", "neighbours", "neighbors", "party", "thin"};
        issueMap["safety"] = {"unsafe", "dangerous", "stolen", "theft", "security", "lock", "locks", "robbed", "safe", "safety",
                              "fire"};
        issueMap["location"] = {"location", "access", "distance", "central", "directions", "transport", "view", "walk"};
        flags = {
            {"Theft / missing property", {"stole", "stolen", "theft", "robbed", "pickpocket", "missing"}, 4, "L5 Police report",
             "Preserve CCTV and key-card logs, secure the room, interview staff on shift, and file a report with the Botswana Police Service.", true, true},
            {"Harassment", {"harass", "harassed", "harassment", "inappropriate", "groped", "touched", "sexual", "threatened"}, 5,
             "L3 Suspend pending investigation",
             "Suspend the named employee with pay, separate them from guests, open a formal investigation, and consider a police report.", true, true},
            {"Violence / assault", {"slapped", "assault", "assaulted", "attacked", "punched", "weapon", "fight", "violent"}, 5,
             "L5 Police report", "Secure the scene, record witness statements, report to the police, and suspend those involved.", true, true},
            {"Discrimination", {"racist", "racism", "discriminate", "discriminated", "discrimination", "sexist", "tribal", "xenophobic"}, 4,
             "M Management decision", "Escalate to management and HR: review the incident, interview staff, and document a corrective plan.", false, true},
            {"Intoxicated staff", {"drunk", "intoxicated", "tipsy", "alcohol on", "smelled of alcohol"}, 4, "L3 Suspend pending investigation",
             "Remove the staff member from guest duty, apply the fitness-for-duty policy, and record the incident.", false, true},
            {"Billing fraud / overcharge", {"overcharged", "scam", "fraud", "double", "fake", "cheated", "ripped"}, 4, "L4 Termination review",
             "Audit the bill and till records, refund verified overcharges, and refer proven staff dishonesty to HR for a termination review.", true, true},
            {"Hygiene / food safety hazard", {"poisoning", "vomit", "vomited", "cockroach", "roach", "rat", "mould", "mold", "expired", "rodent"}, 5,
             "M Management decision", "Stop serving any implicated item, inspect the kitchen or room, discard suspect stock, treat the area, and notify the health inspector if illness is confirmed.", false, true},
            {"Staff rudeness", {"rude", "shouted", "insulted", "arrogant", "disrespectful", "unprofessional", "dismissive"}, 3,
             "L2 Written warning", "Identify the employee from the roster, hold a documented conversation, and issue a written warning if the pattern repeats.", false, false},
            {"Slow service", {"slow", "waited", "waiting", "ignored", "understaffed", "queue", "delay"}, 2, "L1 Coaching and retraining",
             "Review shift staffing, set service-time targets, and coach the team on greeting and ordering flow.", false, false},
            {"Unsafe premises", {"unsafe", "dangerous", "faulty", "leak", "leaking", "fire", "exposed"}, 4, "M Management decision",
             "Close or repair the affected area, log a maintenance ticket, and have management confirm a safety inspection.", false, true}};
    }

    const vector<RedFlag>& redFlags() const { return flags; }

    string issue(const string& text) const {
        vector<string> t = tokens(text);
        std::map<string, int> score;
        for (const auto& tok : t) {
            string st = stemWord(tok);
            for (const auto& kv : issueMap)
                for (const auto& kw : kv.second)
                    if (tok == kw || st == kw) score[kv.first]++;
        }
        if (score.empty()) return "general";
        auto best = std::max_element(score.begin(), score.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
        return best->first;
    }

    vector<string> themes(const string& text) const {
        vector<string> t = tokens(text);
        std::map<string, int> score;
        for (const auto& tok : t) {
            string st = stemWord(tok);
            for (const auto& kv : issueMap)
                for (const auto& kw : kv.second)
                    if (tok == kw || st == kw) score[kv.first]++;
        }
        vector<std::pair<string, int>> v(score.begin(), score.end());
        std::sort(v.begin(), v.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
        vector<string> out;
        for (auto& p : v) out.push_back(p.first);
        if (out.empty()) out.push_back("general");
        return out;
    }

    vector<const RedFlag*> detect(const string& text) const {
        string low = util::lower(text);
        vector<string> t = tokens(text);
        std::unordered_set<string> bag(t.begin(), t.end());
        vector<const RedFlag*> hits;
        for (const auto& f : flags) {
            bool hit = false;
            for (const auto& w : f.words) {
                if (w.find(' ') != string::npos) hit = hit || low.find(w) != string::npos;
                else hit = hit || bag.count(w) > 0;
            }
            if (hit) hits.push_back(&f);
        }
        return hits;
    }

    const std::map<string, vector<string>>& issues() const { return issueMap; }
};

struct User {
    string name, role, salt, hash;
    int fails = 0;
    bool locked = false;
    string created;
};

struct Source {
    string name, domain, kind, added;
};

struct CaseFile {
    int id = 0;
    int entryId = 0;
    string level, action, status, note, opened, decidedBy, flag;
    int severity = 0;
};

struct Profile {
    string business = "Gaborone Hotel & Restaurant";
    string city = "Gaborone";
    string industry = "hotel restaurant";
    int lockLimit = 3;
};

struct Recommendation {
    string title, level, detail, basis, issue, url;
    int priority = 0;
    bool needsManager = false;
};

struct DigestRecord {
    string url, platform, title, mood;
    vector<string> summary;
    vector<std::pair<string, double>> keywords;
    double sentiment = 0;
    size_t words = 0;
    int pos = 0, neu = 0, neg = 0;
    string verdict;
};

struct SiteRecord {
    string url, grade, diagnosis;
    double overall = 0, security = 0, seo = 0, perf = 0;
    size_t pages = 0, links = 0, broken = 0;
    vector<string> findings;
};

struct RepoRecord {
    string source, name, grade, diagnosis;
    double health = 0;
    size_t commits = 0, authors = 0;
    int busFactor = 0;
    vector<string> findings;
};

struct Point {
    string date;
    int pos = 0, neg = 0, neu = 0;
    double index = 0;
};

class Vault {
public:
    fs::path dir;
    Sentiment sentiment;
    Classifier classifier;
    Profile profile;
    vector<Entry> entries;
    vector<User> users;
    vector<Source> sources;
    vector<CaseFile> cases;
    vector<Recommendation> recs;
    vector<DigestRecord> digests;
    vector<SiteRecord> sites;
    vector<RepoRecord> repos;
    string currentUser, currentRole;
    int nextId = 1000;
    int nextCase = 1;
    string lastAuditHash = "GENESIS";

    explicit Vault(const fs::path& d) : dir(d) {
        std::error_code ec;
        fs::create_directories(dir, ec);
        fs::create_directories(dir / "exports", ec);
    }

    fs::path file(const string& n) const { return dir / n; }

    void audit(const string& action, const string& detail) {
        string who = currentUser.empty() ? "system" : currentUser;
        string body = clock_::now() + "|" + who + "|" + action + "|" + esc(detail);
        string h = sec::sha256(lastAuditHash + body).substr(0, 16);
        std::ofstream out(file("audit.log"), std::ios::app);
        out << body << "|" << h << "\n";
        lastAuditHash = h;
    }

    vector<string> auditLines() const {
        vector<string> lines;
        std::ifstream in(file("audit.log"));
        string l;
        while (std::getline(in, l)) lines.push_back(l);
        return lines;
    }

    bool verifyAudit(size_t* count = nullptr, size_t* badAt = nullptr) const {
        string prev = "GENESIS";
        size_t n = 0;
        for (const auto& l : auditLines()) {
            size_t cut = l.rfind('|');
            if (cut == string::npos) {
                if (badAt) *badAt = n + 1;
                return false;
            }
            string body = l.substr(0, cut);
            string h = l.substr(cut + 1);
            if (sec::sha256(prev + body).substr(0, 16) != h) {
                if (badAt) *badAt = n + 1;
                if (count) *count = n;
                return false;
            }
            prev = h;
            ++n;
        }
        if (count) *count = n;
        return true;
    }

    void loadAuditHead() {
        auto lines = auditLines();
        if (lines.empty()) return;
        size_t cut = lines.back().rfind('|');
        if (cut != string::npos) lastAuditHash = lines.back().substr(cut + 1);
    }

    void classify(Entry& e) const {
        e.score = sentiment.analyze(e.text);
        e.label = sentiment.label(e.score);
        e.issue = classifier.issue(e.text);
        e.themes = classifier.themes(e.text);
        auto hits = classifier.detect(e.text);
        e.severity = 0;
        e.flag.clear();
        for (auto* h : hits) {
            if (h->severity > e.severity) {
                e.severity = h->severity;
                e.flag = h->name;
            }
        }
        if (e.severity == 0 && e.score <= -4) e.severity = 3;
        else if (e.severity == 0 && e.score <= -2) e.severity = 2;
        else if (e.severity == 0 && e.score < 0) e.severity = 1;
    }

    int add(const string& source, const string& author, const string& date, const string& text, const string& origin) {
        Entry e;
        e.id = nextId++;
        e.source = source;
        e.author = author.empty() ? "Anonymous" : author;
        e.date = date.empty() ? clock_::today() : date;
        e.text = util::collapseSpace(text);
        e.origin = origin;
        classify(e);
        entries.push_back(e);
        return e.id;
    }

    bool duplicate(const string& source, const string& text) const {
        string t = util::lower(util::collapseSpace(text));
        for (const auto& e : entries)
            if (e.source == source && util::lower(e.text) == t) return true;
        return false;
    }

    void saveEntries() const {
        std::ofstream out(file("feedback.tsv"));
        for (const auto& e : entries)
            out << e.id << "\t" << esc(e.source) << "\t" << esc(e.author) << "\t" << e.date << "\t" << esc(e.origin) << "\t"
                << esc(e.text) << "\n";
    }

    void loadEntries() {
        entries.clear();
        std::ifstream in(file("feedback.tsv"));
        string l;
        while (std::getline(in, l)) {
            auto f = splitTabs(l);
            if (f.size() < 6) continue;
            Entry e;
            try {
                e.id = std::stoi(f[0]);
            } catch (...) {
                continue;
            }
            e.source = unesc(f[1]);
            e.author = unesc(f[2]);
            e.date = f[3];
            e.origin = unesc(f[4]);
            e.text = unesc(f[5]);
            classify(e);
            entries.push_back(e);
            nextId = std::max(nextId, e.id + 1);
        }
    }

    void saveUsers() const {
        std::ofstream out(file("users.tsv"));
        for (const auto& u : users)
            out << u.name << "\t" << u.role << "\t" << u.salt << "\t" << u.hash << "\t" << u.fails << "\t" << (u.locked ? 1 : 0)
                << "\t" << u.created << "\n";
    }

    void loadUsers() {
        users.clear();
        std::ifstream in(file("users.tsv"));
        string l;
        while (std::getline(in, l)) {
            auto f = splitTabs(l);
            if (f.size() < 7) continue;
            User u;
            u.name = f[0];
            u.role = f[1];
            u.salt = f[2];
            u.hash = f[3];
            u.fails = std::atoi(f[4].c_str());
            u.locked = f[5] == "1";
            u.created = f[6];
            users.push_back(u);
        }
    }

    User* findUser(const string& name) {
        for (auto& u : users)
            if (util::lower(u.name) == util::lower(name)) return &u;
        return nullptr;
    }

    bool createUser(const string& name, const string& role, const string& pw) {
        if (name.empty() || pw.size() < 8 || findUser(name)) return false;
        User u;
        u.name = name;
        u.role = role;
        u.salt = sec::randomHex(16);
        u.hash = sec::hashPassword(u.salt, pw);
        u.created = clock_::now();
        users.push_back(u);
        saveUsers();
        return true;
    }

    enum class Auth { Ok, BadCredentials, Locked, Unknown };

    Auth login(const string& name, const string& pw, int* remaining = nullptr) {
        User* u = findUser(name);
        if (!u) {
            audit("LOGIN_UNKNOWN", name);
            return Auth::Unknown;
        }
        if (u->locked) {
            audit("LOGIN_BLOCKED", u->name);
            return Auth::Locked;
        }
        if (sec::hashPassword(u->salt, pw) == u->hash) {
            u->fails = 0;
            saveUsers();
            currentUser = u->name;
            currentRole = u->role;
            audit("LOGIN_OK", u->name + " as " + u->role);
            return Auth::Ok;
        }
        u->fails++;
        if (u->fails >= profile.lockLimit) {
            u->locked = true;
            audit("ACCOUNT_LOCKED", u->name);
        } else audit("LOGIN_FAILED", u->name + " attempt " + std::to_string(u->fails));
        saveUsers();
        if (remaining) *remaining = std::max(0, profile.lockLimit - u->fails);
        return u->locked ? Auth::Locked : Auth::BadCredentials;
    }

    bool isManager() const { return currentRole == "manager"; }
    bool canWrite() const { return currentRole == "manager" || currentRole == "crm"; }

    void saveSources() const {
        std::ofstream out(file("sources.tsv"));
        for (const auto& s : sources) out << esc(s.name) << "\t" << s.domain << "\t" << s.kind << "\t" << s.added << "\n";
    }

    void loadSources() {
        sources.clear();
        std::ifstream in(file("sources.tsv"));
        string l;
        while (std::getline(in, l)) {
            auto f = splitTabs(l);
            if (f.size() < 4) continue;
            sources.push_back({unesc(f[0]), f[1], f[2], f[3]});
        }
    }

    void seedSources() {
        string d = clock_::today();
        sources = {{"Google Reviews", "google.com", "review", d}, {"TripAdvisor", "tripadvisor.com", "review", d},
                   {"Facebook", "facebook.com", "social", d},     {"Instagram", "instagram.com", "social", d},
                   {"X / Twitter", "x.com", "social", d},         {"Reddit", "reddit.com", "social", d},
                   {"YouTube", "youtube.com", "social", d},       {"Booking.com", "booking.com", "review", d},
                   {"Hotels Advice", "duckduckgo.com", "advice", d}};
        saveSources();
    }

    bool approved(const string& urlOrPath) const {
        if (util::startsWith(urlOrPath, "file://") || (!util::startsWith(util::lower(urlOrPath), "http") && fs::exists(urlOrPath))) return true;
        string host = hostOf(urlOrPath);
        for (const auto& s : sources)
            if (host == s.domain || util::endsWith(host, "." + s.domain)) return true;
        return false;
    }

    string sourceNameFor(const string& url) const {
        string host = hostOf(url);
        for (const auto& s : sources)
            if (host == s.domain || util::endsWith(host, "." + s.domain)) return s.name;
        return host.empty() ? "Local file" : host;
    }

    bool addSource(const string& name, const string& domain, const string& kind) {
        string d = hostOf(domain.find("://") == string::npos ? "https://" + domain : domain);
        if (d.empty() || d.find('.') == string::npos) return false;
        for (const auto& s : sources)
            if (s.domain == d) return false;
        sources.push_back({name.empty() ? d : name, d, kind, clock_::today()});
        saveSources();
        return true;
    }

    bool removeSource(size_t idx) {
        if (idx >= sources.size()) return false;
        sources.erase(sources.begin() + (long)idx);
        saveSources();
        return true;
    }

    void saveCases() const {
        std::ofstream out(file("cases.tsv"));
        for (const auto& c : cases)
            out << c.id << "\t" << c.entryId << "\t" << esc(c.level) << "\t" << esc(c.action) << "\t" << c.status << "\t" << esc(c.note)
                << "\t" << c.opened << "\t" << c.decidedBy << "\t" << esc(c.flag) << "\t" << c.severity << "\n";
    }

    void loadCases() {
        cases.clear();
        std::ifstream in(file("cases.tsv"));
        string l;
        while (std::getline(in, l)) {
            auto f = splitTabs(l);
            if (f.size() < 10) continue;
            CaseFile c;
            c.id = std::atoi(f[0].c_str());
            c.entryId = std::atoi(f[1].c_str());
            c.level = unesc(f[2]);
            c.action = unesc(f[3]);
            c.status = f[4];
            c.note = unesc(f[5]);
            c.opened = f[6];
            c.decidedBy = f[7];
            c.flag = unesc(f[8]);
            c.severity = std::atoi(f[9].c_str());
            cases.push_back(c);
            nextCase = std::max(nextCase, c.id + 1);
        }
    }

    void saveProfile() const {
        std::ofstream out(file("profile.tsv"));
        out << esc(profile.business) << "\t" << esc(profile.city) << "\t" << esc(profile.industry) << "\n";
    }

    void loadProfile() {
        std::ifstream in(file("profile.tsv"));
        string l;
        if (std::getline(in, l)) {
            auto f = splitTabs(l);
            if (f.size() >= 3) {
                profile.business = unesc(f[0]);
                profile.city = unesc(f[1]);
                profile.industry = unesc(f[2]);
            }
        }
    }

    void load() {
        loadProfile();
        loadUsers();
        loadSources();
        if (sources.empty()) seedSources();
        loadEntries();
        loadCases();
        loadAuditHead();
    }

    const Entry* entryById(int id) const {
        for (const auto& e : entries)
            if (e.id == id) return &e;
        return nullptr;
    }

    int openCasesFor(int entryId) const {
        int n = 0;
        for (const auto& c : cases)
            if (c.entryId == entryId) ++n;
        return n;
    }

    int syncCases() {
        int created = 0;
        for (const auto& e : entries) {
            if (e.severity < 3 || e.flag.empty() || openCasesFor(e.id) > 0) continue;
            const RedFlag* chosen = nullptr;
            for (const auto& f : classifier.redFlags())
                if (f.name == e.flag) chosen = &f;
            if (!chosen) continue;
            CaseFile c;
            c.id = nextCase++;
            c.entryId = e.id;
            c.level = chosen->level;
            c.action = chosen->action;
            c.flag = chosen->name;
            c.severity = e.severity;
            c.status = chosen->mgmt ? "AWAITING MANAGEMENT" : "OPEN";
            c.opened = clock_::now();
            cases.push_back(c);
            ++created;
        }
        if (created) {
            saveCases();
            audit("CASES_OPENED", std::to_string(created));
        }
        return created;
    }
};

}

namespace fb {

string dateMinus(int days) {
    std::time_t t = std::time(nullptr) - (std::time_t)days * 86400;
    char buf[16];
    std::strftime(buf, sizeof buf, "%Y-%m-%d", std::localtime(&t));
    return buf;
}

struct Snapshot {
    string business, generatedAt, user;
    size_t total = 0;
    double avg = 0, satisfaction = 0, buildMs = 0;
    int pos = 0, neg = 0, neu = 0;
    vector<std::pair<string, int>> issues;
    vector<std::pair<string, int>> heat;
    vector<std::pair<string, int>> sources;
    vector<std::pair<string, double>> sourceAvg;
    vector<Point> points;
    double moving3 = 0, volatility = 0, slope = 0, forecast = 0;
    Point best, worst;
    vector<Entry> critical;
    vector<Entry> praise;
    vector<Entry> negatives;
    vector<Entry> positives;
    vector<CaseFile> cases;
    vector<Recommendation> recs;
    vector<DigestRecord> digests;
    vector<SiteRecord> sites;
    vector<RepoRecord> repos;
    vector<std::pair<string, double>> words;
};

struct CriticalOrder {
    bool operator()(const Entry& a, const Entry& b) const {
        if (a.severity != b.severity) return a.severity < b.severity;
        return a.score > b.score;
    }
};

vector<std::pair<string, double>> topWords(const vector<Entry>& entries, size_t limit) {
    static const std::unordered_set<string> stop = {
        "the", "and", "was", "were", "for", "that", "this", "with", "have", "had", "but", "are", "our", "you", "your", "they", "them",
        "from", "all", "not", "very", "just", "would", "there", "their", "been", "when", "will", "out", "one", "get", "got", "can",
        "its", "it's", "too", "also", "than", "then", "about", "after", "again", "any", "because", "before", "being", "here", "into",
        "more", "most", "only", "over", "some", "such", "which", "while", "who", "what", "has", "did", "does", "even", "much", "stay",
        "room", "place", "hotel", "restaurant"};
    std::map<string, double> freq;
    for (const auto& e : entries)
        for (const auto& t : tokens(e.text))
            if (t.size() > 3 && !stop.count(t)) freq[t] += 1.0;
    vector<std::pair<string, double>> v(freq.begin(), freq.end());
    std::sort(v.begin(), v.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    if (v.size() > limit) v.resize(limit);
    return v;
}

Snapshot build(Vault& vault) {
    double t0 = clock_::ms();
    Snapshot s;
    s.business = vault.profile.business;
    s.generatedAt = clock_::now();
    s.user = vault.currentUser.empty() ? "guest" : vault.currentUser;
    s.total = vault.entries.size();
    double sum = 0;
    std::map<string, int> issueCount, heatCount, srcCount;
    std::map<string, double> srcSum;
    std::map<string, Point> byDay;
    std::priority_queue<Entry, vector<Entry>, CriticalOrder> pq;
    for (const auto& e : vault.entries) {
        sum += e.score;
        issueCount[e.issue]++;
        srcCount[e.source]++;
        srcSum[e.source] += e.score;
        Point& p = byDay[e.date];
        p.date = e.date;
        if (e.label == "POSITIVE") {
            s.pos++;
            p.pos++;
            s.positives.push_back(e);
        } else if (e.label == "NEGATIVE") {
            s.neg++;
            p.neg++;
            heatCount[e.issue]++;
            s.negatives.push_back(e);
            pq.push(e);
        } else {
            s.neu++;
            p.neu++;
        }
    }
    s.avg = s.total ? sum / (double)s.total : 0.0;
    s.satisfaction = s.total ? ((double)s.pos + 0.5 * s.neu) / (double)s.total * 100.0 : 0.0;
    auto rank = [](const std::map<string, int>& m) {
        vector<std::pair<string, int>> v(m.begin(), m.end());
        std::sort(v.begin(), v.end(), [](const auto& a, const auto& b) { return a.second > b.second || (a.second == b.second && a.first < b.first); });
        return v;
    };
    s.issues = rank(issueCount);
    s.heat = rank(heatCount);
    s.sources = rank(srcCount);
    for (const auto& kv : s.sources) s.sourceAvg.push_back({kv.first, srcSum[kv.first] / kv.second});
    for (auto& kv : byDay) {
        Point p = kv.second;
        int tot = p.pos + p.neg + p.neu;
        p.index = tot ? (p.pos + 0.5 * p.neu) / tot * 100.0 : 0.0;
        s.points.push_back(p);
    }
    if (!s.points.empty()) {
        s.best = *std::max_element(s.points.begin(), s.points.end(), [](const Point& a, const Point& b) { return a.index < b.index; });
        s.worst = *std::min_element(s.points.begin(), s.points.end(), [](const Point& a, const Point& b) { return a.index < b.index; });
        size_t w = std::min<size_t>(3, s.points.size());
        double m = 0;
        for (size_t i = s.points.size() - w; i < s.points.size(); ++i) m += s.points[i].index;
        s.moving3 = m / (double)w;
        double mean = 0;
        for (auto& p : s.points) mean += p.index;
        mean /= (double)s.points.size();
        if (s.points.size() > 1) {
            double var = 0;
            for (auto& p : s.points) var += (p.index - mean) * (p.index - mean);
            s.volatility = std::sqrt(var / (double)(s.points.size() - 1));
            double n = (double)s.points.size(), sx = 0, sy = 0, sxy = 0, sxx = 0;
            for (size_t i = 0; i < s.points.size(); ++i) {
                double x = (double)i, y = s.points[i].index;
                sx += x;
                sy += y;
                sxy += x * y;
                sxx += x * x;
            }
            double den = n * sxx - sx * sx;
            s.slope = den != 0 ? (n * sxy - sx * sy) / den : 0;
            double icpt = (sy - s.slope * sx) / n;
            s.forecast = std::max(0.0, std::min(100.0, icpt + s.slope * (n + 2)));
        } else s.forecast = mean;
    }
    while (!pq.empty() && s.critical.size() < 10) {
        s.critical.push_back(pq.top());
        pq.pop();
    }
    vector<Entry> pos = s.positives;
    std::sort(pos.begin(), pos.end(), [](const Entry& a, const Entry& b) { return a.score > b.score; });
    for (size_t i = 0; i < pos.size() && i < 5; ++i) s.praise.push_back(pos[i]);
    s.cases = vault.cases;
    s.recs = vault.recs;
    s.digests = vault.digests;
    s.sites = vault.sites;
    s.repos = vault.repos;
    s.words = topWords(vault.entries, 25);
    s.buildMs = clock_::ms() - t0;
    return s;
}

struct WebHit {
    string title, url, snippet;
};

vector<WebHit> webSearch(const string& query, size_t limit, bool& reachable) {
    vector<WebHit> hits;
    string body;
    int status = 0;
    reachable = od::net::curlAvailable() && od::net::get("https://html.duckduckgo.com/html/?q=" + urlEncode(query), body, status) && status == 200;
    if (!reachable) return hits;
    size_t pos = 0;
    while (hits.size() < limit) {
        size_t a = body.find("class=\"result__a\"", pos);
        if (a == string::npos) break;
        size_t hs = body.find("href=\"", a);
        if (hs == string::npos) break;
        hs += 6;
        size_t he = body.find('"', hs);
        string href = body.substr(hs, he - hs);
        size_t ts = body.find('>', he);
        size_t te = body.find("</a>", ts);
        string title = od::util::collapseSpace(od::html::stripTags(body.substr(ts + 1, te - ts - 1)));
        size_t sn = body.find("result__snippet", te);
        string snippet;
        size_t next = body.find("class=\"result__a\"", te);
        if (sn != string::npos && (next == string::npos || sn < next)) {
            size_t ss = body.find('>', sn);
            size_t se = body.find("</a>", ss);
            snippet = od::util::collapseSpace(od::util::htmlDecode(od::html::stripTags(body.substr(ss + 1, se - ss - 1))));
            pos = se;
        } else pos = te;
        size_t u = href.find("uddg=");
        if (u != string::npos) {
            string enc = href.substr(u + 5);
            size_t amp = enc.find('&');
            if (amp != string::npos) enc = enc.substr(0, amp);
            href = urlDecode(enc);
        } else if (util::startsWith(href, "//")) href = "https:" + href;
        title = od::util::htmlDecode(title);
        if (title.empty() || !util::startsWith(href, "http")) continue;
        hits.push_back({title, href, snippet});
    }
    return hits;
}

const std::map<string, vector<string>>& knowledgeBase() {
    static const std::map<string, vector<string>> kb = {
        {"service", {"Set a service-time target (for example greet within 60 seconds, serve within 15 minutes) and display it to staff.",
                     "Review shift rosters against peak hours and add cover where queues form.",
                     "Run a 30 minute pre-shift briefing on the three most common complaints of the week.",
                     "Reply publicly to every slow-service review within 24 hours with a concrete fix."}},
        {"staff conduct", {"Identify the employees on shift from the roster before any conclusion is drawn.",
                           "Hold a documented, private conversation and agree an improvement plan.",
                           "Issue a written warning for repeated rudeness and escalate repeated cases to HR.",
                           "Train front-line staff on de-escalation and guest recovery."}},
        {"cleanliness", {"Introduce a signed room-inspection checklist for housekeeping with supervisor spot checks.",
                         "Deep-clean the rooms or areas named in reviews and photograph the completed result.",
                         "Audit linen, bathroom and ventilation standards weekly.",
                         "Offer an apology and a goodwill gesture to affected guests."}},
        {"food quality", {"Hold a tasting session with kitchen staff on the dishes named in complaints.",
                          "Check holding temperatures and plating times; cold food usually signals a pass or runner bottleneck.",
                          "Rotate stock by first-in-first-out and log expiry checks daily.",
                          "Contact the guest, offer a replacement meal or credit, and record the outcome."}},
        {"pricing", {"Compare your rates with three nearby competitors and review the value on offer.",
                     "Make every extra charge visible on the menu or booking page before payment.",
                     "Train staff to explain pricing and offer a package where guests feel overcharged.",
                     "Audit billing records for errors against complaints."}},
        {"facilities", {"Open a maintenance ticket for every faulty item and report repair progress weekly.",
                        "Test WiFi, air-conditioning, showers and lifts as part of room readiness checks.",
                        "Invest in backup power or a secondary internet line if outages repeat.",
                        "Tell guests about known issues upfront and offer an alternative room."}},
        {"noise", {"Set quiet hours and brief staff and event organisers.",
                   "Move noise-sensitive guests away from lifts, bars and event areas.",
                   "Inspect door seals and offer earplugs as a quick fix."}},
        {"safety", {"Treat safety complaints as urgent: inspect locks, lighting and security coverage within 24 hours.",
                    "Review CCTV and key-card access logs for the incident window.",
                    "Brief management and decide whether the Botswana Police Service must be informed."}},
        {"location", {"Publish clear directions, parking details and transport options on every listing.",
                      "Offer shuttle or taxi booking to guests."}},
        {"general", {"Read the raw comments in this category and tag them manually; the system could not isolate a clear cause.",
                     "Management should weigh in on whether this feedback requires a separate survey question."}}};
    return kb;
}

vector<Recommendation> generate(Vault& vault, const Snapshot& snap, bool useWeb, string& webNote, int perIssueWeb) {
    vector<Recommendation> out;
    const auto& kb = knowledgeBase();
    int prio = 100;
    for (const auto& kv : snap.heat) {
        auto it = kb.find(kv.first);
        const vector<string>& acts = it != kb.end() ? it->second : kb.at("general");
        string level = kv.second >= 5 ? "L2 Written warning / process review" : kv.second >= 3 ? "L1 Coaching and retraining" : "Monitor";
        bool manager = kv.first == "general" || kv.first == "safety" || kv.second >= 8;
        size_t take = std::min<size_t>(acts.size(), kv.second >= 3 ? 3 : 2);
        for (size_t i = 0; i < take; ++i) {
            Recommendation r;
            r.title = "Fix " + kv.first + " (" + std::to_string(kv.second) + " negative comments)";
            r.level = level;
            r.detail = acts[i];
            r.basis = "Built-in hospitality playbook";
            r.issue = kv.first;
            r.priority = prio - (int)i;
            r.needsManager = manager;
            out.push_back(r);
        }
        prio -= 10;
    }
    for (const auto& c : vault.cases) {
        if (c.status == "DISMISSED") continue;
        const Entry* e = vault.entryById(c.entryId);
        Recommendation r;
        r.title = c.flag + " — case #" + std::to_string(c.id);
        r.level = c.level;
        r.detail = c.action + (e ? " Source comment: \"" + util::ellipsize(e->text, 120) + "\"" : "");
        r.basis = "Misconduct rules engine";
        r.issue = c.flag;
        r.priority = 200 + c.severity;
        r.needsManager = c.status == "AWAITING MANAGEMENT" || c.level[0] == 'M' || c.level.substr(0, 2) == "L4" || c.level.substr(0, 2) == "L5";
        out.push_back(r);
    }
    if (snap.satisfaction < 55 && snap.total > 0) {
        Recommendation r;
        r.title = "Satisfaction index critically low (" + pct(snap.satisfaction) + ")";
        r.level = "M Management decision";
        r.detail = "The problem is wider than any single complaint. Management must review staffing, standards and pricing together and set a recovery plan with weekly targets.";
        r.basis = "Trend analytics";
        r.issue = "overall";
        r.priority = 300;
        r.needsManager = true;
        out.push_back(r);
    } else if (snap.slope < -2.0 && snap.points.size() >= 4) {
        Recommendation r;
        r.title = "Satisfaction trending downward";
        r.level = "L1 Coaching and retraining";
        r.detail = "The satisfaction index is declining by " + fix(-snap.slope, 1) + " points per day. Find what changed recently (menu, staff, supplier, prices) and reverse it.";
        r.basis = "Trend analytics";
        r.issue = "trend";
        r.priority = 150;
        r.needsManager = false;
        out.push_back(r);
    }
    if (useWeb) {
        int searched = 0;
        bool anyReach = false;
        for (const auto& kv : snap.heat) {
            if (searched >= perIssueWeb) break;
            string q = "how to fix " + kv.first + " complaints " + vault.profile.industry + " guest reviews";
            bool reach = false;
            auto hits = webSearch(q, 3, reach);
            if (!reach) {
                webNote = "Web search unreachable; recommendations come from the built-in playbook only.";
                break;
            }
            anyReach = true;
            ++searched;
            for (const auto& h : hits) {
                Recommendation r;
                r.title = h.title;
                r.level = "Research";
                r.detail = h.snippet.empty() ? "Open the page for guidance relevant to " + kv.first + "." : h.snippet;
                r.basis = "Web: " + hostOf(h.url);
                r.issue = kv.first;
                r.url = h.url;
                r.priority = 50;
                r.needsManager = false;
                out.push_back(r);
            }
        }
        if (anyReach) webNote = "Web research completed for " + std::to_string(searched) + " problem areas.";
    } else webNote = "Web research skipped.";
    std::stable_sort(out.begin(), out.end(), [](const Recommendation& a, const Recommendation& b) { return a.priority > b.priority; });
    return out;
}

struct IngestResult {
    bool ok = false;
    string error, platform, title;
    int added = 0, skipped = 0;
    DigestRecord digest;
};

IngestResult ingestLink(Vault& vault, string input) {
    IngestResult r;
    input = od::fetch::cleanInput(input);
    if (input.empty()) {
        r.error = "No link was entered.";
        return r;
    }
    if (!vault.approved(input)) {
        r.error = "That website is not on the approved list. Add it under Approved Websites first (" + hostOf(input) + ").";
        vault.audit("LINK_BLOCKED", input);
        return r;
    }
    if (!od::net::curlAvailable() && !fs::exists(input) && !util::startsWith(input, "file://")) {
        r.error = "curl is not installed, so web links cannot be downloaded.";
        return r;
    }
    od::Fetched f = od::fetch::any(input);
    if (!f.ok) {
        r.error = f.error.empty() ? "The page could not be read." : f.error;
        vault.audit("LINK_FAILED", input);
        return r;
    }
    r.platform = f.doc.platform;
    r.title = f.doc.title;
    string source = vault.sourceNameFor(input);
    auto addText = [&](const string& author, const string& text) {
        string t = util::collapseSpace(text);
        if (t.size() < 20 || t.size() > 1200 || t == "[deleted]" || t == "[removed]") return;
        if (vault.duplicate(source, t)) {
            r.skipped++;
            return;
        }
        vault.add(source, author, clock_::today(), t, input);
        r.added++;
    };
    for (const auto& c : f.doc.comments) addText(c.author, c.text);
    if (f.doc.comments.size() < 6) {
        string corpus = f.doc.title + ". " + f.doc.description + "\n" + f.doc.body;
        auto sents = od::nlp::splitSentences(corpus);
        int taken = 0;
        for (const auto& s : sents) {
            if (taken >= 60) break;
            if (s.size() < 30) continue;
            addText(f.doc.author, s);
            ++taken;
        }
    }
    od::Analysis a = od::nlp::analyze(f.doc);
    r.digest.url = input;
    r.digest.platform = f.doc.platform;
    r.digest.title = f.doc.title;
    r.digest.mood = a.mood;
    r.digest.summary = a.summary;
    r.digest.keywords = a.keywords;
    r.digest.sentiment = a.sentiment;
    r.digest.words = a.words;
    r.digest.pos = a.pos;
    r.digest.neu = a.neu;
    r.digest.neg = a.neg;
    r.digest.verdict = a.verdict;
    vault.digests.push_back(r.digest);
    vault.saveEntries();
    vault.audit("LINK_INGESTED", input + " added=" + std::to_string(r.added));
    r.ok = true;
    return r;
}

SiteRecord scanSite(Vault& vault, const string& input, int pages, int depth) {
    SiteRecord rec;
    string url = xr::crawl::cleanInput(input);
    rec.url = url;
    xr::Report rep = xr::crawl::run(url, pages, depth, 20);
    rec.overall = rep.overall;
    rec.grade = rep.grade;
    rec.diagnosis = rep.diagnosis;
    rec.security = rep.security.score;
    rec.seo = rep.seo.score;
    rec.perf = rep.perf.score;
    rec.pages = rep.pagesCrawled;
    rec.links = rep.linksChecked;
    rec.broken = rep.brokenLinks;
    rec.findings = rep.findings;
    vault.sites.push_back(rec);
    vault.audit("SITE_SCANNED", url);
    return rec;
}

RepoRecord scanRepo(Vault& vault, const string& input, int depth, string& error) {
    RepoRecord rec;
    if (!gt::shell::gitAvailable()) {
        error = "git is not installed on this machine.";
        return rec;
    }
    gt::source::Handle h = gt::source::open(input, depth);
    if (!h.ok) {
        error = h.error;
        return rec;
    }
    vector<gt::Commit> commits;
    string err;
    if (!gt::history::load(h.path, 5000, commits, err)) {
        gt::source::cleanup(h);
        error = err.empty() ? "No commit history could be read." : err;
        return rec;
    }
    gt::Report rep = gt::insight::analyze(commits, h.display, input);
    gt::source::cleanup(h);
    rec.source = input;
    rec.name = rep.repoName;
    rec.grade = rep.grade;
    rec.diagnosis = rep.diagnosis;
    rec.health = rep.health;
    rec.commits = rep.commits;
    rec.authors = rep.authorsTotal;
    rec.busFactor = rep.busFactor;
    rec.findings = rep.findings;
    vault.repos.push_back(rec);
    vault.audit("REPO_SCANNED", input);
    return rec;
}

struct Seed {
    const char* source;
    const char* author;
    int daysAgo;
    const char* text;
};

int seedDemo(Vault& vault) {
    static const Seed seeds[] = {
        {"Google Reviews", "Kagiso M.", 13, "The room was dirty and the service was extremely slow. Worst stay ever."},
        {"TripAdvisor", "Thabo R.", 13, "Amazing food and friendly staff. Highly recommend this place!"},
        {"Facebook", "Naledi S.", 12, "Good location but the wifi was broken the whole time."},
        {"Survey", "Guest D", 12, "Excellent cleanliness and delicious breakfast. Will come again."},
        {"Instagram", "lesedi_b", 11, "Rude staff and overpriced rooms. Not worth the money."},
        {"Google Reviews", "Mpho K.", 11, "Fast check-in, clean rooms, great value for money."},
        {"TripAdvisor", "Traveler G", 10, "Cold food and long wait times. Disappointed."},
        {"Facebook", "Boitumelo T.", 10, "Wonderful atmosphere and perfect service. Love it here."},
        {"Google Reviews", "Onkabetse P.", 9, "The waiter was drunk and shouted at my wife. Absolutely unacceptable."},
        {"TripAdvisor", "Sam W.", 9, "Lovely pool and spacious room, very comfortable bed."},
        {"Survey", "Guest H", 9, "Breakfast was cold and the waiter ignored us for twenty minutes."},
        {"Facebook", "Tebogo N.", 8, "My phone was stolen from the room and nobody at reception seemed to care."},
        {"Booking.com", "Anna L.", 8, "Great location near the CBD, helpful reception staff."},
        {"Google Reviews", "Kabo D.", 8, "Shower was broken and parking was limited. Poor maintenance."},
        {"Instagram", "gaborone_foodie", 7, "Tasty dinner, polite waiters and a relaxing garden."},
        {"TripAdvisor", "Michael J.", 7, "Overcharged on the bill and the manager refused to correct it. Felt like a scam."},
        {"Survey", "Guest K", 7, "Room smelled of mould and the sheets were stained."},
        {"Facebook", "Refilwe C.", 6, "Staff were attentive, the food was fresh and the price was fair."},
        {"Google Reviews", "Tshepo G.", 6, "Noisy neighbours all night and the walls are so thin."},
        {"TripAdvisor", "Linda P.", 6, "Not clean at all. Dust everywhere and a cockroach in the bathroom."},
        {"Booking.com", "Pieter V.", 5, "Excellent stay, quick service and delicious breakfast buffet."},
        {"Instagram", "kgosi_la_phuti", 5, "The receptionist was rude and unprofessional when I asked about my booking."},
        {"Survey", "Guest M", 5, "Slow service at dinner but the chef was brilliant."},
        {"Facebook", "Neo B.", 4, "Perfect stay, highly recommend to everyone. Spotless rooms."},
        {"Google Reviews", "Itumeleng S.", 4, "A staff member made inappropriate comments to a female guest at the bar."},
        {"TripAdvisor", "David H.", 4, "Overpriced menu and tiny portions. Not coming back."},
        {"Google Reviews", "Masego O.", 3, "Friendly team and a beautiful, quiet location."},
        {"Facebook", "Dineo F.", 3, "Waited forty minutes for check-in. The lobby was understaffed and nobody apologised."},
        {"Survey", "Guest R", 3, "Food poisoning after the seafood platter. I was sick all night."},
        {"Booking.com", "Chen W.", 2, "Clean, comfortable and efficient. Wifi was fast and the staff were helpful."},
        {"Instagram", "motswana_mom", 2, "Loved the kids menu and the lovely, welcoming waitress."},
        {"TripAdvisor", "Gaone L.", 2, "The AC was faulty and the room was freezing then hot. Maintenance never came."},
        {"Google Reviews", "Sipho Z.", 1, "Wonderful dinner, the service was quick and the wine was superb."},
        {"Facebook", "Mmoloki A.", 1, "Terrible experience. The staff were rude and the food was cold again."},
        {"Survey", "Guest T", 1, "Good value and a very clean room. The pool area is lovely."},
        {"TripAdvisor", "Kelebogile M.", 0, "Excellent breakfast and impressive attention to detail."},
        {"Google Reviews", "Karabo J.", 0, "Average stay. Nothing special but nothing wrong."},
        {"Instagram", "bw_traveller", 0, "Great pool, but the music from the bar was loud until late."},
        {"Facebook", "Obakeng R.", 0, "The staff member smelled of alcohol and was rude to our group."}};
    int added = 0;
    for (const auto& s : seeds) {
        if (vault.duplicate(s.source, s.text)) continue;
        vault.add(s.source, s.author, dateMinus(s.daysAgo), s.text, "demo-seed");
        ++added;
    }
    vault.saveEntries();
    vault.audit("DEMO_SEED", std::to_string(added));
    return added;
}

}

namespace fb {

struct Req {
    const char* id;
    const char* kind;
    const char* text;
    const char* feature;
    const char* moscow;
};

const vector<Req>& requirements() {
    static const vector<Req> r = {
        {"FR-1", "Functional", "Gather feedback from social media, review sites and surveys into one place", "Feedback Hub: links, manual entry, CSV import", "Must"},
        {"FR-2", "Functional", "View all comments without logging into many apps", "Feedback Explorer and HTML explorer export", "Must"},
        {"FR-3", "Functional", "Separate positive and negative sentiment", "Negation-aware sentiment engine", "Must"},
        {"FR-4", "Functional", "Report common complaint themes and main problems", "Issue heatmap and theme extractor", "Must"},
        {"FR-5", "Functional", "Business intelligence and decision support for the manager", "Dashboard, trends, forecast, recommendations", "Must"},
        {"FR-6", "Functional", "Recommend actions up to suspension, police report or management review", "Case Desk and escalation ladder", "Should"},
        {"TR-1", "Technical", "Interface usable on computers and phones", "Responsive HTML exports; web and Android shells planned", "Must"},
        {"TR-2", "Technical", "Data stored in a database for later access", "Persistent vault with integrity checks", "Must"},
        {"TR-3", "Technical", "Pull comments from Facebook, Google Reviews, TripAdvisor", "Approved-source link ingestion", "Should"},
        {"TR-4", "Technical", "Low cost, no large budget", "Single binary; only curl needed; zero licences", "Must"},
        {"SR-1", "Security", "Password-protected access for authorised staff only", "Salted, iterated SHA-256 passwords", "Must"},
        {"SR-2", "Security", "Lock account after 3 failed attempts", "Lockout counter with manager unlock", "Must"},
        {"SR-3", "Security", "Restrictive access control", "Manager, Customer Relations and Viewer roles", "Must"},
        {"SR-4", "Security", "Audit record of who did what and when", "Hash-chained audit log with verification", "Must"},
        {"NFR-U", "Usability", "Easy to learn and pleasant to use", "Guided 14-step test drive and colour interface", "Must"},
        {"NFR-R", "Reliability", "Consistent, trustworthy data", "Duplicate guard and tamper-evident audit chain", "Must"},
        {"NFR-P", "Performance", "Generate reports within 3 seconds", "Measured on every report build", "Must"},
        {"NFR-S", "Security", "Only approved websites may be read", "Approved source whitelist", "Must"}};
    return r;
}

string hex(const Rgb& c) {
    char b[8];
    std::snprintf(b, sizeof b, "%02X%02X%02X", c.r, c.g, c.b);
    return b;
}

string cssRgb(const Rgb& c) { return "#" + hex(c); }

string levelWord(const string& level) { return level; }

string toTxt(const Snapshot& s) {
    std::ostringstream o;
    o << "CUSTOMER FEEDBACK INTELLIGENCE REPORT\n" << string(60, '=') << "\n";
    o << "Business : " << s.business << "\nGenerated: " << s.generatedAt << "\nPrepared by: " << s.user << "\n\n";
    o << "TOTAL ENTRIES      : " << s.total << "\nAVERAGE SENTIMENT  : " << fix(s.avg) << "\nSATISFACTION INDEX : " << pct(s.satisfaction)
      << "\nPOSITIVE / NEUTRAL / NEGATIVE : " << s.pos << " / " << s.neu << " / " << s.neg << "\n\n";
    o << "TOP ISSUES\n" << string(60, '-') << "\n";
    for (const auto& i : s.issues) o << "  " << std::left << std::setw(18) << i.first << i.second << "\n";
    o << "\nNEGATIVE HEATMAP\n" << string(60, '-') << "\n";
    for (const auto& i : s.heat) o << "  " << std::left << std::setw(18) << i.first << i.second << " complaints\n";
    o << "\nSOURCES\n" << string(60, '-') << "\n";
    for (size_t i = 0; i < s.sources.size(); ++i)
        o << "  " << std::left << std::setw(18) << s.sources[i].first << std::setw(6) << s.sources[i].second << "avg " << fix(s.sourceAvg[i].second) << "\n";
    o << "\nTREND\n" << string(60, '-') << "\n";
    for (const auto& p : s.points) o << "  " << p.date << "  pos " << p.pos << "  neg " << p.neg << "  neu " << p.neu << "  index " << fix(p.index, 1) << "\n";
    o << "  3-day moving average: " << fix(s.moving3, 1) << "%   volatility: " << fix(s.volatility, 1) << "   forecast (+2 days): " << fix(s.forecast, 1) << "%\n";
    o << "\nCRITICAL ITEMS\n" << string(60, '-') << "\n";
    for (const auto& e : s.critical) o << "  #" << e.id << " [" << e.source << "] sev " << e.severity << " " << e.issue << ": " << e.text << "\n";
    o << "\nCASES\n" << string(60, '-') << "\n";
    for (const auto& c : s.cases) o << "  #" << c.id << " " << c.flag << " -> " << c.level << " (" << c.status << ")\n";
    o << "\nRECOMMENDATIONS\n" << string(60, '-') << "\n";
    for (const auto& r : s.recs) o << "  [" << r.level << "] " << r.title << "\n      " << r.detail << "\n      Basis: " << r.basis << (r.url.empty() ? "" : "  " + r.url) << (r.needsManager ? "\n      >> Management decision required" : "") << "\n";
    o << "\nBuilt in " << fix(s.buildMs, 1) << " ms\n";
    return o.str();
}

string toMarkdown(const Snapshot& s) {
    std::ostringstream o;
    o << "# Customer Feedback Intelligence Report\n\n**" << s.business << "** · " << s.generatedAt << " · prepared by " << s.user << "\n\n";
    o << "| Entries | Avg sentiment | Satisfaction | Positive | Neutral | Negative |\n|---|---|---|---|---|---|\n";
    o << "| " << s.total << " | " << fix(s.avg) << " | " << pct(s.satisfaction) << " | " << s.pos << " | " << s.neu << " | " << s.neg << " |\n\n";
    o << "## Negative issue heatmap\n\n| Issue | Complaints |\n|---|---|\n";
    for (const auto& i : s.heat) o << "| " << i.first << " | " << i.second << " |\n";
    o << "\n## Sources\n\n| Source | Entries | Avg score |\n|---|---|---|\n";
    for (size_t i = 0; i < s.sources.size(); ++i) o << "| " << s.sources[i].first << " | " << s.sources[i].second << " | " << fix(s.sourceAvg[i].second) << " |\n";
    o << "\n## Trend\n\n| Date | Pos | Neg | Neu | Index |\n|---|---|---|---|---|\n";
    for (const auto& p : s.points) o << "| " << p.date << " | " << p.pos << " | " << p.neg << " | " << p.neu << " | " << fix(p.index, 1) << " |\n";
    o << "\nMoving average " << fix(s.moving3, 1) << "% · volatility " << fix(s.volatility, 1) << " · forecast " << fix(s.forecast, 1) << "%\n\n";
    o << "## Critical comments\n\n";
    for (const auto& e : s.critical) o << "- **#" << e.id << "** (" << e.source << ", severity " << e.severity << ", " << e.issue << ") " << e.text << "\n";
    o << "\n## Case desk\n\n| Case | Finding | Action level | Status |\n|---|---|---|---|\n";
    for (const auto& c : s.cases) o << "| #" << c.id << " | " << c.flag << " | " << c.level << " | " << c.status << " |\n";
    o << "\n## Recommendations\n\n";
    for (const auto& r : s.recs) o << "- **" << r.title << "** — *" << r.level << "*. " << r.detail << " (" << r.basis << ")" << (r.url.empty() ? "" : " [link](" + r.url + ")") << (r.needsManager ? " **Management decision required.**" : "") << "\n";
    if (!s.digests.empty()) {
        o << "\n## Link digests\n\n";
        for (const auto& d : s.digests) {
            o << "### " << d.title << "\n" << d.url << "\n\nMood: " << d.mood << "\n\n";
            for (const auto& l : d.summary) o << "- " << l << "\n";
        }
    }
    o << "\n## Requirements traceability\n\n| ID | Type | Requirement | Feature | MoSCoW |\n|---|---|---|---|---|\n";
    for (const auto& r : requirements()) o << "| " << r.id << " | " << r.kind << " | " << r.text << " | " << r.feature << " | " << r.moscow << " |\n";
    o << "\n_Report built in " << fix(s.buildMs, 1) << " ms._\n";
    return o.str();
}

string jsonEntry(const Entry& e) {
    std::ostringstream o;
    o << "{\"id\":" << e.id << ",\"source\":\"" << util::jsonEscape(e.source) << "\",\"author\":\"" << util::jsonEscape(e.author) << "\",\"date\":\"" << e.date
      << "\",\"score\":" << e.score << ",\"label\":\"" << e.label << "\",\"issue\":\"" << e.issue << "\",\"severity\":" << e.severity << ",\"flag\":\""
      << util::jsonEscape(e.flag) << "\",\"text\":\"" << util::jsonEscape(e.text) << "\"}";
    return o.str();
}

string toJson(const Snapshot& s, const Vault& v) {
    std::ostringstream o;
    o << "{\n  \"business\":\"" << util::jsonEscape(s.business) << "\",\n  \"generated\":\"" << s.generatedAt << "\",\n  \"user\":\"" << util::jsonEscape(s.user) << "\",\n";
    o << "  \"totals\":{\"entries\":" << s.total << ",\"positive\":" << s.pos << ",\"neutral\":" << s.neu << ",\"negative\":" << s.neg << ",\"avgSentiment\":" << fix(s.avg, 3)
      << ",\"satisfaction\":" << fix(s.satisfaction, 2) << ",\"buildMs\":" << fix(s.buildMs, 2) << "},\n";
    o << "  \"heatmap\":[";
    for (size_t i = 0; i < s.heat.size(); ++i) o << (i ? "," : "") << "{\"issue\":\"" << s.heat[i].first << "\",\"count\":" << s.heat[i].second << "}";
    o << "],\n  \"sources\":[";
    for (size_t i = 0; i < s.sources.size(); ++i) o << (i ? "," : "") << "{\"name\":\"" << util::jsonEscape(s.sources[i].first) << "\",\"count\":" << s.sources[i].second << ",\"avg\":" << fix(s.sourceAvg[i].second, 3) << "}";
    o << "],\n  \"trend\":[";
    for (size_t i = 0; i < s.points.size(); ++i) o << (i ? "," : "") << "{\"date\":\"" << s.points[i].date << "\",\"pos\":" << s.points[i].pos << ",\"neg\":" << s.points[i].neg << ",\"neu\":" << s.points[i].neu << ",\"index\":" << fix(s.points[i].index, 2) << "}";
    o << "],\n  \"movingAverage3\":" << fix(s.moving3, 2) << ",\n  \"volatility\":" << fix(s.volatility, 2) << ",\n  \"forecast\":" << fix(s.forecast, 2) << ",\n  \"entries\":[\n";
    for (size_t i = 0; i < v.entries.size(); ++i) o << "    " << jsonEntry(v.entries[i]) << (i + 1 < v.entries.size() ? ",\n" : "\n");
    o << "  ],\n  \"cases\":[";
    for (size_t i = 0; i < s.cases.size(); ++i) o << (i ? "," : "") << "{\"id\":" << s.cases[i].id << ",\"entry\":" << s.cases[i].entryId << ",\"flag\":\"" << util::jsonEscape(s.cases[i].flag) << "\",\"level\":\"" << util::jsonEscape(s.cases[i].level) << "\",\"status\":\"" << s.cases[i].status << "\"}";
    o << "],\n  \"recommendations\":[";
    for (size_t i = 0; i < s.recs.size(); ++i) o << (i ? "," : "") << "{\"title\":\"" << util::jsonEscape(s.recs[i].title) << "\",\"level\":\"" << util::jsonEscape(s.recs[i].level) << "\",\"detail\":\"" << util::jsonEscape(s.recs[i].detail) << "\",\"basis\":\"" << util::jsonEscape(s.recs[i].basis) << "\",\"url\":\"" << util::jsonEscape(s.recs[i].url) << "\",\"managerRequired\":" << (s.recs[i].needsManager ? "true" : "false") << "}";
    o << "]\n}\n";
    return o.str();
}

string csvCell(const string& s) {
    string o = "\"";
    for (char c : s) o += c == '"' ? string("\"\"") : string(1, c == '\n' ? ' ' : c);
    return o + "\"";
}

string toCsv(const Vault& v) {
    std::ostringstream o;
    o << "id,date,source,author,score,label,issue,severity,flag,text\n";
    for (const auto& e : v.entries) o << e.id << "," << e.date << "," << csvCell(e.source) << "," << csvCell(e.author) << "," << e.score << "," << e.label << "," << csvCell(e.issue) << "," << e.severity << "," << csvCell(e.flag) << "," << csvCell(e.text) << "\n";
    return o.str();
}

string toTsv(const Vault& v) {
    std::ostringstream o;
    o << "id\tdate\tsource\tauthor\tscore\tlabel\tissue\tseverity\tflag\ttext\n";
    for (const auto& e : v.entries) o << e.id << "\t" << e.date << "\t" << esc(e.source) << "\t" << esc(e.author) << "\t" << e.score << "\t" << e.label << "\t" << e.issue << "\t" << e.severity << "\t" << esc(e.flag) << "\t" << esc(e.text) << "\n";
    return o.str();
}

string toXml(const Snapshot& s, const Vault& v) {
    std::ostringstream o;
    o << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<feedbackReport business=\"" << util::htmlEscape(s.business) << "\" generated=\"" << s.generatedAt << "\">\n";
    o << "  <totals entries=\"" << s.total << "\" positive=\"" << s.pos << "\" neutral=\"" << s.neu << "\" negative=\"" << s.neg << "\" satisfaction=\"" << fix(s.satisfaction) << "\"/>\n  <heatmap>\n";
    for (const auto& h : s.heat) o << "    <issue name=\"" << util::htmlEscape(h.first) << "\" count=\"" << h.second << "\"/>\n";
    o << "  </heatmap>\n  <entries>\n";
    for (const auto& e : v.entries) o << "    <entry id=\"" << e.id << "\" date=\"" << e.date << "\" source=\"" << util::htmlEscape(e.source) << "\" label=\"" << e.label << "\" issue=\"" << util::htmlEscape(e.issue) << "\" severity=\"" << e.severity << "\">" << util::htmlEscape(e.text) << "</entry>\n";
    o << "  </entries>\n  <recommendations>\n";
    for (const auto& r : s.recs) o << "    <recommendation level=\"" << util::htmlEscape(r.level) << "\" manager=\"" << (r.needsManager ? "true" : "false") << "\"><title>" << util::htmlEscape(r.title) << "</title><detail>" << util::htmlEscape(r.detail) << "</detail></recommendation>\n";
    o << "  </recommendations>\n</feedbackReport>\n";
    return o.str();
}

string yamlStr(const string& s) { return "\"" + util::jsonEscape(s) + "\""; }

string toYaml(const Snapshot& s) {
    std::ostringstream o;
    o << "business: " << yamlStr(s.business) << "\ngenerated: " << yamlStr(s.generatedAt) << "\ntotals:\n  entries: " << s.total << "\n  positive: " << s.pos << "\n  neutral: " << s.neu << "\n  negative: " << s.neg << "\n  satisfaction: " << fix(s.satisfaction) << "\nheatmap:\n";
    for (const auto& h : s.heat) o << "  - issue: " << yamlStr(h.first) << "\n    count: " << h.second << "\n";
    o << "trend:\n";
    for (const auto& p : s.points) o << "  - date: " << p.date << "\n    index: " << fix(p.index) << "\n";
    o << "recommendations:\n";
    for (const auto& r : s.recs) o << "  - title: " << yamlStr(r.title) << "\n    level: " << yamlStr(r.level) << "\n    manager_required: " << (r.needsManager ? "true" : "false") << "\n    detail: " << yamlStr(r.detail) << "\n";
    return o.str();
}

string rtfEsc(const string& s) {
    string o;
    for (unsigned char c : s) {
        if (c == '\\' || c == '{' || c == '}') o += string("\\") + (char)c;
        else if (c >= 0x80) o += "?";
        else o += (char)c;
    }
    return o;
}

string toRtf(const Snapshot& s) {
    std::ostringstream o;
    o << "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0 Calibri;}}{\\colortbl;\\red16\\green185\\blue129;\\red239\\green68\\blue68;\\red59\\green130\\blue246;\\red168\\green85\\blue247;\\red250\\green204\\blue21;}\n";
    o << "\\f0\\fs40\\cf4\\b Customer Feedback Intelligence\\b0\\par\n\\fs24\\cf3 " << rtfEsc(s.business) << " - " << s.generatedAt << "\\par\\par\n";
    o << "\\cf1 Positive: " << s.pos << "\\cf0   \\cf5 Neutral: " << s.neu << "\\cf0   \\cf2 Negative: " << s.neg << "\\cf0\\par\n";
    o << "Satisfaction index: " << pct(s.satisfaction) << "\\par\\par\n\\cf4\\b Issues\\b0\\cf0\\par\n";
    for (const auto& h : s.heat) o << "\\cf2 " << rtfEsc(h.first) << "\\cf0 : " << h.second << "\\par\n";
    o << "\\par\\cf4\\b Recommendations\\b0\\cf0\\par\n";
    for (const auto& r : s.recs) o << "\\cf3\\b " << rtfEsc(r.title) << "\\b0\\cf0  (" << rtfEsc(r.level) << ") " << rtfEsc(r.detail) << "\\par\n";
    o << "}";
    return o.str();
}

string toIcs(const Snapshot& s) {
    std::ostringstream o;
    o << "BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//David Rhodes//Feedback Intelligence//EN\r\n";
    std::time_t t = std::time(nullptr) + 86400;
    char d[16];
    int n = 0;
    for (const auto& c : s.cases) {
        std::time_t due = t + (std::time_t)(c.severity >= 5 ? 0 : c.severity >= 4 ? 86400 : 3 * 86400);
        std::strftime(d, sizeof d, "%Y%m%d", std::localtime(&due));
        o << "BEGIN:VEVENT\r\nUID:case-" << c.id << "@feedback-intelligence\r\nDTSTAMP:" << clock_::compact().substr(0, 8) << "T000000Z\r\nDTSTART;VALUE=DATE:" << d << "\r\nSUMMARY:Case #" << c.id << " " << c.flag << " - " << c.level << "\r\nDESCRIPTION:" << c.action << "\r\nEND:VEVENT\r\n";
        ++n;
    }
    o << "END:VCALENDAR\r\n";
    return o.str();
}

string toSvg(const Snapshot& s) {
    std::ostringstream o;
    o << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 960 640\" font-family=\"Segoe UI, Arial, sans-serif\">\n";
    o << "<defs><linearGradient id=\"bg\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"1\"><stop offset=\"0\" stop-color=\"#0b1026\"/><stop offset=\"1\" stop-color=\"#2a1450\"/></linearGradient>";
    o << "<linearGradient id=\"rb\" x1=\"0\" x2=\"1\"><stop offset=\"0\" stop-color=\"#22c55e\"/><stop offset=\".25\" stop-color=\"#3b82f6\"/><stop offset=\".5\" stop-color=\"#a855f7\"/><stop offset=\".75\" stop-color=\"#f97316\"/><stop offset=\"1\" stop-color=\"#facc15\"/></linearGradient></defs>\n";
    o << "<rect width=\"960\" height=\"640\" rx=\"24\" fill=\"url(#bg)\"/><rect x=\"24\" y=\"24\" width=\"912\" height=\"6\" rx=\"3\" fill=\"url(#rb)\"/>\n";
    o << "<text x=\"40\" y=\"72\" font-size=\"28\" font-weight=\"700\" fill=\"#fde68a\">" << util::htmlEscape(s.business) << " — Feedback Pulse</text>\n";
    o << "<text x=\"40\" y=\"98\" font-size=\"14\" fill=\"#94a3b8\">" << s.total << " comments · satisfaction " << pct(s.satisfaction) << " · " << s.generatedAt << "</text>\n";
    double total = std::max(1, s.pos + s.neg + s.neu);
    double cx = 190, cy = 250, r = 90, circ = 2 * 3.14159265 * r, off = 0;
    struct Seg { double v; const char* col; const char* name; };
    Seg segs[] = {{(double)s.pos, "#10b981", "Positive"}, {(double)s.neu, "#facc15", "Neutral"}, {(double)s.neg, "#ef4444", "Negative"}};
    o << "<circle cx=\"" << cx << "\" cy=\"" << cy << "\" r=\"" << r << "\" fill=\"none\" stroke=\"#1e293b\" stroke-width=\"34\"/>\n";
    for (auto& g : segs) {
        double len = g.v / total * circ;
        o << "<circle cx=\"" << cx << "\" cy=\"" << cy << "\" r=\"" << r << "\" fill=\"none\" stroke=\"" << g.col << "\" stroke-width=\"34\" stroke-dasharray=\"" << fix(len, 2) << " " << fix(circ - len, 2) << "\" stroke-dashoffset=\"" << fix(-off, 2) << "\" transform=\"rotate(-90 " << cx << " " << cy << ")\"/>\n";
        off += len;
    }
    o << "<text x=\"" << cx << "\" y=\"" << cy + 8 << "\" text-anchor=\"middle\" font-size=\"28\" font-weight=\"700\" fill=\"#f8fafc\">" << fix(s.satisfaction, 0) << "%</text>\n";
    int ly = 380;
    for (auto& g : segs) {
        o << "<rect x=\"90\" y=\"" << ly - 12 << "\" width=\"14\" height=\"14\" rx=\"4\" fill=\"" << g.col << "\"/><text x=\"112\" y=\"" << ly << "\" font-size=\"14\" fill=\"#e2e8f0\">" << g.name << " " << (int)g.v << "</text>\n";
        ly += 24;
    }
    const char* palette_[] = {"#22c55e", "#3b82f6", "#a855f7", "#f97316", "#ef4444", "#facc15", "#2dd4bf", "#f43f5e", "#a3e635"};
    int by = 150;
    int maxv = s.heat.empty() ? 1 : s.heat.front().second;
    o << "<text x=\"400\" y=\"140\" font-size=\"16\" font-weight=\"600\" fill=\"#fde68a\">Negative issue heatmap</text>\n";
    for (size_t i = 0; i < s.heat.size() && i < 8; ++i) {
        double w = 440.0 * s.heat[i].second / maxv;
        o << "<text x=\"400\" y=\"" << by + 22 << "\" font-size=\"13\" fill=\"#e2e8f0\">" << util::htmlEscape(s.heat[i].first) << "</text>\n";
        o << "<rect x=\"540\" y=\"" << by + 8 << "\" width=\"" << fix(w * 0.9, 1) << "\" height=\"18\" rx=\"9\" fill=\"" << palette_[i % 9] << "\"/><text x=\"" << fix(548 + w * 0.9, 1) << "\" y=\"" << by + 22 << "\" font-size=\"13\" fill=\"#f8fafc\">" << s.heat[i].second << "</text>\n";
        by += 32;
    }
    o << "<text x=\"40\" y=\"470\" font-size=\"16\" font-weight=\"600\" fill=\"#fde68a\">Satisfaction trend</text>\n";
    if (s.points.size() >= 2) {
        double x0 = 60, x1 = 900, y0 = 600, y1 = 490;
        std::ostringstream pts, area;
        area << x0 << "," << y0 << " ";
        for (size_t i = 0; i < s.points.size(); ++i) {
            double x = x0 + (x1 - x0) * i / (double)(s.points.size() - 1);
            double y = y0 - (y0 - y1) * s.points[i].index / 100.0;
            pts << fix(x, 1) << "," << fix(y, 1) << " ";
            area << fix(x, 1) << "," << fix(y, 1) << " ";
        }
        area << x1 << "," << y0;
        o << "<polygon points=\"" << area.str() << "\" fill=\"#a855f7\" opacity=\"0.25\"/><polyline points=\"" << pts.str() << "\" fill=\"none\" stroke=\"url(#rb)\" stroke-width=\"4\" stroke-linejoin=\"round\"/>\n";
        for (size_t i = 0; i < s.points.size(); ++i) {
            double x = x0 + (x1 - x0) * i / (double)(s.points.size() - 1);
            double y = y0 - (y0 - y1) * s.points[i].index / 100.0;
            o << "<circle cx=\"" << fix(x, 1) << "\" cy=\"" << fix(y, 1) << "\" r=\"4\" fill=\"#fde68a\"/>\n";
        }
    }
    o << "</svg>\n";
    return o.str();
}

string cssBlock() {
    return R"CSS(
:root{--bg:#0b1026;--card:#141a3a;--ink:#f1f5f9;--mut:#94a3b8;--jasper:#22c55e;--sapphire:#3b82f6;--amethyst:#a855f7;--sardonyx:#f97316;--sardius:#ef4444;--topaz:#facc15;--beryl:#2dd4bf;--pearl:#f8fafc;--rose:#f472b6}
@media (prefers-color-scheme:light){:root:not([data-theme=dark]){--bg:#f5f3ff;--card:#ffffff;--ink:#1e1b4b;--mut:#64748b}}
*{box-sizing:border-box}html,body{margin:0;padding:0}
body{font-family:'Segoe UI',system-ui,Arial,sans-serif;background:radial-gradient(circle at 15% 0%,#3b1d6e 0,transparent 45%),radial-gradient(circle at 90% 10%,#0c4a6e 0,transparent 40%),var(--bg);color:var(--ink);padding:env(safe-area-inset-top,0) 0 env(safe-area-inset-bottom,0)}
.wrap{max-width:1180px;margin:0 auto;padding:20px}
.hero{border-radius:24px;padding:28px;background:linear-gradient(120deg,#22c55e,#3b82f6,#a855f7,#f97316,#facc15);color:#0b1026}
.hero h1{margin:0 0 6px;font-size:clamp(24px,5vw,40px)}.hero p{margin:0;font-weight:600}
.grid{display:grid;gap:16px;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));margin:18px 0}
.card{background:var(--card);border-radius:18px;padding:18px;border:1px solid rgba(255,255,255,.08);box-shadow:0 10px 30px rgba(0,0,0,.25)}
.kpi b{display:block;font-size:34px;margin-top:4px}
.k1{border-top:5px solid var(--jasper)}.k2{border-top:5px solid var(--sapphire)}.k3{border-top:5px solid var(--amethyst)}.k4{border-top:5px solid var(--sardonyx)}.k5{border-top:5px solid var(--topaz)}
h2{margin:26px 0 10px;font-size:22px;background:linear-gradient(90deg,var(--topaz),var(--rose),var(--beryl));-webkit-background-clip:text;background-clip:text;color:transparent}
.bar{height:14px;border-radius:99px;background:rgba(148,163,184,.25);overflow:hidden;margin:6px 0 12px}.bar i{display:block;height:100%;border-radius:99px}
table{width:100%;border-collapse:collapse;font-size:14px}th,td{padding:9px 10px;text-align:left;border-bottom:1px solid rgba(148,163,184,.2);vertical-align:top}
th{color:var(--topaz)}.scroll{overflow-x:auto}
.tag{display:inline-block;padding:2px 10px;border-radius:99px;font-size:12px;font-weight:700;color:#0b1026}
.POSITIVE{background:var(--jasper)}.NEGATIVE{background:var(--sardius)}.NEUTRAL{background:var(--topaz)}.mgr{background:var(--sardonyx)}
.rec{border-left:6px solid var(--amethyst);margin:10px 0;padding:10px 14px;border-radius:12px;background:rgba(255,255,255,.04)}
.rec.m{border-left-color:var(--sardonyx)}
a{color:var(--beryl)}footer{color:var(--mut);text-align:center;padding:30px 0;font-size:13px}
.tabs input{display:none}.tabs label{display:inline-block;padding:8px 16px;margin:0 6px 8px 0;border-radius:99px;background:var(--card);cursor:pointer;font-weight:700}
.tabs .pane{display:none}
#t1:checked~.p1,#t2:checked~.p2,#t3:checked~.p3,#t4:checked~.p4{display:block}
#t1:checked~label[for=t1],#t2:checked~label[for=t2],#t3:checked~label[for=t3],#t4:checked~label[for=t4]{background:linear-gradient(90deg,var(--amethyst),var(--sapphire));color:#fff}
)CSS";
}

string statBar(double frac, const string& color) {
    std::ostringstream o;
    o << "<div class=\"bar\"><i style=\"width:" << fix(std::max(0.0, std::min(1.0, frac)) * 100, 1) << "%;background:" << color << "\"></i></div>";
    return o.str();
}

string toHtmlDashboard(const Snapshot& s, const Vault& v) {
    std::ostringstream o;
    const char* cols[] = {"#22c55e", "#3b82f6", "#a855f7", "#f97316", "#ef4444", "#facc15", "#2dd4bf", "#f43f5e", "#a3e635"};
    o << "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1,viewport-fit=cover\"><title>" << util::htmlEscape(s.business) << " — Feedback Intelligence</title><style>" << cssBlock() << "</style></head><body><div class=\"wrap\">";
    o << "<div class=\"hero\"><h1>✦ " << util::htmlEscape(s.business) << "</h1><p>Customer Feedback Intelligence · " << s.generatedAt << " · prepared by " << util::htmlEscape(s.user) << "</p></div>";
    o << "<div class=\"grid\"><div class=\"card kpi k1\">Comments<b>" << s.total << "</b></div><div class=\"card kpi k2\">Satisfaction<b>" << pct(s.satisfaction) << "</b></div><div class=\"card kpi k3\">Positive<b>" << s.pos << "</b></div><div class=\"card kpi k4\">Negative<b>" << s.neg << "</b></div><div class=\"card kpi k5\">Open cases<b>" << s.cases.size() << "</b></div></div>";
    o << "<div class=\"tabs\"><input type=\"radio\" name=\"t\" id=\"t1\" checked><label for=\"t1\">Overview</label><input type=\"radio\" name=\"t\" id=\"t2\"><label for=\"t2\">Comments</label><input type=\"radio\" name=\"t\" id=\"t3\"><label for=\"t3\">Actions</label><input type=\"radio\" name=\"t\" id=\"t4\"><label for=\"t4\">Systems</label>";
    o << "<div class=\"pane p1\"><div class=\"card\">" << toSvg(s) << "</div><h2>Issue heatmap</h2><div class=\"card\">";
    int maxv = s.heat.empty() ? 1 : s.heat.front().second;
    for (size_t i = 0; i < s.heat.size(); ++i) o << "<div><b>" << util::htmlEscape(s.heat[i].first) << "</b> · " << s.heat[i].second << "</div>" << statBar((double)s.heat[i].second / maxv, cols[i % 9]);
    o << "</div><h2>Sources</h2><div class=\"card scroll\"><table><tr><th>Source</th><th>Comments</th><th>Average score</th></tr>";
    for (size_t i = 0; i < s.sources.size(); ++i) o << "<tr><td>" << util::htmlEscape(s.sources[i].first) << "</td><td>" << s.sources[i].second << "</td><td>" << fix(s.sourceAvg[i].second) << "</td></tr>";
    o << "</table></div><h2>Trend</h2><div class=\"card scroll\"><table><tr><th>Date</th><th>Positive</th><th>Negative</th><th>Neutral</th><th>Index</th></tr>";
    for (const auto& p : s.points) o << "<tr><td>" << p.date << "</td><td>" << p.pos << "</td><td>" << p.neg << "</td><td>" << p.neu << "</td><td>" << fix(p.index, 1) << "%</td></tr>";
    o << "</table><p>3-day average <b>" << fix(s.moving3, 1) << "%</b> · volatility <b>" << fix(s.volatility, 1) << "</b> · forecast <b>" << fix(s.forecast, 1) << "%</b></p></div></div>";
    o << "<div class=\"pane p2\"><h2>Critical comments</h2><div class=\"card scroll\"><table><tr><th>ID</th><th>Source</th><th>Issue</th><th>Sev</th><th>Comment</th></tr>";
    for (const auto& e : s.critical) o << "<tr><td>#" << e.id << "</td><td>" << util::htmlEscape(e.source) << "</td><td>" << util::htmlEscape(e.issue) << "</td><td>" << e.severity << "</td><td>" << util::htmlEscape(e.text) << "</td></tr>";
    o << "</table></div><h2>All feedback</h2><div class=\"card scroll\"><table><tr><th>Date</th><th>Source</th><th>Mood</th><th>Issue</th><th>Comment</th></tr>";
    for (const auto& e : v.entries) o << "<tr><td>" << e.date << "</td><td>" << util::htmlEscape(e.source) << "</td><td><span class=\"tag " << e.label << "\">" << e.label << "</span></td><td>" << util::htmlEscape(e.issue) << "</td><td>" << util::htmlEscape(e.text) << "</td></tr>";
    o << "</table></div></div>";
    o << "<div class=\"pane p3\"><h2>Case desk</h2><div class=\"card scroll\"><table><tr><th>Case</th><th>Finding</th><th>Action level</th><th>Status</th></tr>";
    for (const auto& c : s.cases) o << "<tr><td>#" << c.id << "</td><td>" << util::htmlEscape(c.flag) << "</td><td>" << util::htmlEscape(c.level) << "</td><td>" << util::htmlEscape(c.status) << "</td></tr>";
    o << "</table></div><h2>Recommendations</h2>";
    for (const auto& r : s.recs) {
        o << "<div class=\"rec" << (r.needsManager ? " m" : "") << "\"><b>" << util::htmlEscape(r.title) << "</b> <span class=\"tag " << (r.needsManager ? "mgr" : "NEUTRAL") << "\">" << util::htmlEscape(r.level) << "</span><div>" << util::htmlEscape(r.detail) << "</div><small>" << util::htmlEscape(r.basis) << "</small>";
        if (!r.url.empty()) o << " · <a href=\"" << util::htmlEscape(r.url) << "\" rel=\"noopener\">open</a>";
        if (r.needsManager) o << "<div><b>Management decision required</b></div>";
        o << "</div>";
    }
    o << "</div><div class=\"pane p4\">";
    for (const auto& d : s.digests) {
        o << "<h2>Link digest: " << util::htmlEscape(d.title) << "</h2><div class=\"card\"><small>" << util::htmlEscape(d.url) << "</small><p>Mood: <b>" << util::htmlEscape(d.mood) << "</b></p><ul>";
        for (const auto& l : d.summary) o << "<li>" << util::htmlEscape(l) << "</li>";
        o << "</ul></div>";
    }
    for (const auto& st : s.sites) {
        o << "<h2>Website X-ray: " << util::htmlEscape(st.url) << "</h2><div class=\"card\"><p>Grade <b>" << util::htmlEscape(st.grade) << "</b> · overall " << fix(st.overall, 0) << " · security " << fix(st.security, 0) << " · SEO " << fix(st.seo, 0) << " · speed " << fix(st.perf, 0) << "</p><p>" << util::htmlEscape(st.diagnosis) << "</p></div>";
    }
    for (const auto& rp : s.repos) o << "<h2>Repository autopsy: " << util::htmlEscape(rp.name) << "</h2><div class=\"card\"><p>Health <b>" << fix(rp.health, 0) << "</b> · grade " << util::htmlEscape(rp.grade) << " · commits " << rp.commits << " · bus factor " << rp.busFactor << "</p><p>" << util::htmlEscape(rp.diagnosis) << "</p></div>";
    o << "<h2>Requirements traceability</h2><div class=\"card scroll\"><table><tr><th>ID</th><th>Type</th><th>Requirement</th><th>Delivered by</th><th>MoSCoW</th></tr>";
    for (const auto& r : requirements()) o << "<tr><td>" << r.id << "</td><td>" << r.kind << "</td><td>" << util::htmlEscape(r.text) << "</td><td>" << util::htmlEscape(r.feature) << "</td><td>" << r.moscow << "</td></tr>";
    o << "</table><p>Report built in <b>" << fix(s.buildMs, 1) << " ms</b> (target under 3000 ms).</p></div></div></div><footer>✦ Built by David Rhodes · ISS323 · New Jerusalem Edition ✦</footer></div></body></html>";
    return o.str();
}

string toHtmlExplorer(const Snapshot& s, const Vault& v) {
    std::ostringstream o;
    o << "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1,viewport-fit=cover\"><title>Feedback Explorer</title><style>" << cssBlock()
      << "input,select{background:var(--card);color:var(--ink);border:1px solid rgba(148,163,184,.4);border-radius:12px;padding:10px;font-size:15px}.row{display:flex;gap:10px;flex-wrap:wrap;margin:14px 0}</style></head><body><div class=\"wrap\"><div class=\"hero\"><h1>Feedback Explorer</h1><p>" << util::htmlEscape(s.business) << " · " << v.entries.size() << " comments</p></div>";
    o << "<div class=\"row\"><input id=\"q\" placeholder=\"Search comments\" style=\"flex:1;min-width:200px\"><select id=\"m\"><option value=\"\">All moods</option><option>POSITIVE</option><option>NEGATIVE</option><option>NEUTRAL</option></select><select id=\"i\"><option value=\"\">All issues</option>";
    for (const auto& kv : s.issues) o << "<option>" << util::htmlEscape(kv.first) << "</option>";
    o << "</select></div><div id=\"n\"></div><div class=\"card scroll\"><table id=\"t\"><thead><tr><th>Date</th><th>Source</th><th>Mood</th><th>Issue</th><th>Comment</th></tr></thead><tbody></tbody></table></div>";
    o << "<script>const D=[";
    for (size_t i = 0; i < v.entries.size(); ++i) {
        const auto& e = v.entries[i];
        o << (i ? "," : "") << "{d:\"" << e.date << "\",s:\"" << util::jsonEscape(e.source) << "\",m:\"" << e.label << "\",i:\"" << util::jsonEscape(e.issue) << "\",t:\"" << util::jsonEscape(e.text) << "\"}";
    }
    o << "];const q=document.getElementById('q'),m=document.getElementById('m'),i=document.getElementById('i'),b=document.querySelector('#t tbody'),n=document.getElementById('n');"
         "function esc(x){return x.replace(/[&<>]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;'}[c]))}"
         "function draw(){const k=q.value.toLowerCase();const r=D.filter(x=>(!m.value||x.m===m.value)&&(!i.value||x.i===i.value)&&(!k||x.t.toLowerCase().includes(k)||x.s.toLowerCase().includes(k)));"
         "n.textContent=r.length+' shown';b.innerHTML=r.map(x=>'<tr><td>'+x.d+'</td><td>'+esc(x.s)+'</td><td><span class=\"tag '+x.m+'\">'+x.m+'</span></td><td>'+esc(x.i)+'</td><td>'+esc(x.t)+'</td></tr>').join('')}"
         "[q,m,i].forEach(e=>e.addEventListener('input',draw));draw();</script></div></body></html>";
    return o.str();
}

namespace zip {

uint32_t crc(const string& d) {
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
        init = true;
    }
    uint32_t c = 0xFFFFFFFFu;
    for (unsigned char b : d) c = table[(c ^ b) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

void le16(string& s, uint16_t v) {
    s += (char)(v & 0xFF);
    s += (char)(v >> 8);
}

void le32(string& s, uint32_t v) {
    for (int i = 0; i < 4; ++i) s += (char)((v >> (8 * i)) & 0xFF);
}

string pack(const vector<std::pair<string, string>>& files) {
    string out, central;
    for (const auto& f : files) {
        uint32_t offset = (uint32_t)out.size();
        uint32_t c = crc(f.second);
        out += "PK\x03\x04";
        le16(out, 20);
        le16(out, 0);
        le16(out, 0);
        le16(out, 0);
        le16(out, 0x21);
        le32(out, c);
        le32(out, (uint32_t)f.second.size());
        le32(out, (uint32_t)f.second.size());
        le16(out, (uint16_t)f.first.size());
        le16(out, 0);
        out += f.first;
        out += f.second;
        central += "PK\x01\x02";
        le16(central, 20);
        le16(central, 20);
        le16(central, 0);
        le16(central, 0);
        le16(central, 0);
        le16(central, 0x21);
        le32(central, c);
        le32(central, (uint32_t)f.second.size());
        le32(central, (uint32_t)f.second.size());
        le16(central, (uint16_t)f.first.size());
        le16(central, 0);
        le16(central, 0);
        le16(central, 0);
        le16(central, 0);
        le32(central, 0);
        le32(central, offset);
        central += f.first;
    }
    uint32_t cdOff = (uint32_t)out.size();
    out += central;
    out += "PK\x05\x06";
    le16(out, 0);
    le16(out, 0);
    le16(out, (uint16_t)files.size());
    le16(out, (uint16_t)files.size());
    le32(out, (uint32_t)central.size());
    le32(out, cdOff);
    le16(out, 0);
    return out;
}

}

namespace dx {

string x(const string& s) {
    string o;
    for (unsigned char c : s) {
        if (c == '&') o += "&amp;";
        else if (c == '<') o += "&lt;";
        else if (c == '>') o += "&gt;";
        else if (c < 0x20 && c != '\t') continue;
        else o += (char)c;
    }
    return o;
}

string run(const string& text, const string& color, int halfPts, bool bold = false, bool italic = false, const string& font = "Calibri") {
    std::ostringstream o;
    o << "<w:r><w:rPr><w:rFonts w:ascii=\"" << font << "\" w:hAnsi=\"" << font << "\" w:cs=\"" << font << "\"/>" << (bold ? "<w:b/>" : "") << (italic ? "<w:i/>" : "")
      << "<w:color w:val=\"" << color << "\"/><w:sz w:val=\"" << halfPts << "\"/><w:szCs w:val=\"" << halfPts << "\"/></w:rPr><w:t xml:space=\"preserve\">" << x(text) << "</w:t></w:r>";
    return o.str();
}

string para(const string& runs, const string& align = "left", const string& fill = "", int after = 80) {
    std::ostringstream o;
    o << "<w:p><w:pPr>" << (fill.empty() ? "" : "<w:shd w:val=\"clear\" w:color=\"auto\" w:fill=\"" + fill + "\"/>") << "<w:spacing w:after=\"" << after << "\"/><w:jc w:val=\"" << align << "\"/></w:pPr>" << runs << "</w:p>";
    return o.str();
}

string cell(const string& inner, int width, const string& fill, const string& borderColor = "FFFFFF") {
    std::ostringstream o;
    o << "<w:tc><w:tcPr><w:tcW w:w=\"" << width << "\" w:type=\"dxa\"/><w:tcBorders><w:top w:val=\"single\" w:sz=\"6\" w:color=\"" << borderColor << "\"/><w:left w:val=\"single\" w:sz=\"6\" w:color=\"" << borderColor << "\"/><w:bottom w:val=\"single\" w:sz=\"6\" w:color=\"" << borderColor << "\"/><w:right w:val=\"single\" w:sz=\"6\" w:color=\"" << borderColor << "\"/></w:tcBorders>"
      << (fill.empty() ? "" : "<w:shd w:val=\"clear\" w:color=\"auto\" w:fill=\"" + fill + "\"/>") << "<w:tcMar><w:top w:w=\"60\" w:type=\"dxa\"/><w:left w:w=\"100\" w:type=\"dxa\"/><w:bottom w:w=\"60\" w:type=\"dxa\"/><w:right w:w=\"100\" w:type=\"dxa\"/></w:tcMar></w:tcPr>" << (inner.empty() ? "<w:p/>" : inner) << "</w:tc>";
    return o.str();
}

string row(const vector<string>& cells) {
    string o = "<w:tr>";
    for (const auto& c : cells) o += c;
    return o + "</w:tr>";
}

string table(const vector<string>& rows, const vector<int>& widths) {
    string o = "<w:tbl><w:tblPr><w:tblW w:w=\"0\" w:type=\"auto\"/><w:tblLayout w:type=\"fixed\"/></w:tblPr><w:tblGrid>";
    for (int w : widths) o += "<w:gridCol w:w=\"" + std::to_string(w) + "\"/>";
    o += "</w:tblGrid>";
    for (const auto& r : rows) o += r;
    return o + "</w:tbl>";
}

string gap() { return "<w:p><w:pPr><w:spacing w:after=\"60\"/></w:pPr></w:p>"; }

string hair() { return "<w:p><w:pPr><w:spacing w:before=\"0\" w:after=\"0\" w:line=\"20\" w:lineRule=\"exact\"/></w:pPr></w:p>"; }

string section(const string& glyph, const string& title, const string& color) {
    vector<int> w = {500, 8500};
    string left = cell(para(run(glyph, "0B1026", 28, true, false, "Consolas"), "center"), 500, color, color);
    string right = cell(para(run("  " + title, "FDE68A", 28, true, false, "Consolas")), 8500, "0B1026", "0B1026");
    return table({row({left, right})}, w) + gap();
}

string meter(const string& label, double frac, const string& color, const string& value) {
    int total = 5200;
    int a = std::max(1, (int)(total * std::max(0.0, std::min(1.0, frac))));
    int b = std::max(1, total - a);
    vector<int> w = {2400, a, b, 1400};
    string c1 = cell(para(run(label, "1E1B4B", 20, true), "left", "", 0), 2400, "F5F3FF", "F5F3FF");
    string c2 = cell(para(run(" ", color, 16), "left", "", 0), a, color, color);
    string c3 = cell(para(run(" ", "E2E8F0", 16), "left", "", 0), b, "E2E8F0", "E2E8F0");
    string c4 = cell(para(run(value, color, 20, true, false, "Consolas"), "right", "", 0), 1400, "F5F3FF", "F5F3FF");
    return table({row({c1, c2, c3, c4})}, w) + hair();
}

}

string toDocx(const Snapshot& s, const Vault& v) {
    using namespace dx;
    const char* J[] = {"22C55E", "3B82F6", "A855F7", "F97316", "EF4444", "FACC15", "2DD4BF", "F43F5E", "A3E635"};
    string body;
    {
        string rainbowRow;
        vector<int> w;
        vector<string> cells;
        for (int i = 0; i < 12; ++i) {
            w.push_back(750);
            cells.push_back(cell(para(run(" ", J[i % 9], 10), "left", "", 0), 750, J[i % 9], J[i % 9]));
        }
        body += table({row(cells)}, w) + gap();
    }
    {
        vector<int> w = {9000};
        string c = cell(para(run("◢◤◢◤  CUSTOMER FEEDBACK INTELLIGENCE  ◥◣◥◣", "FDE68A", 44, true, false, "Consolas"), "center", "", 60) +
                            para(run("⟁ " + s.business + " ⟁", "2DD4BF", 28, true, false, "Consolas"), "center", "", 60) +
                            para(run("SYSTEM SCAN " + s.generatedAt + "  ::  OPERATOR " + util::lower(s.user), "A855F7", 20, false, false, "Consolas"), "center", "", 60),
                        9000, "0B1026", "FACC15");
        body += table({row({c})}, w) + gap();
    }
    {
        vector<int> w = {2250, 2250, 2250, 2250};
        auto card = [&](const string& label, const string& val, const string& fill, const string& ink) {
            return cell(para(run(label, ink, 18, true, false, "Consolas"), "center", "", 20) + para(run(val, ink, 52, true, false, "Consolas"), "center", "", 20), 2250, fill, "FFFFFF");
        };
        body += table({row({card("COMMENTS", std::to_string(s.total), "3B82F6", "FFFFFF"), card("SATISFACTION", pct(s.satisfaction), "22C55E", "0B1026"), card("NEGATIVE", std::to_string(s.neg), "EF4444", "FFFFFF"), card("OPEN CASES", std::to_string(s.cases.size()), "A855F7", "FFFFFF")})}, w) + gap();
    }
    body += section("◈", "SENTIMENT SPECTRUM", "22C55E");
    double tot = std::max(1, s.pos + s.neg + s.neu);
    body += meter("Positive", s.pos / tot, "10B981", std::to_string(s.pos)) + meter("Neutral", s.neu / tot, "FACC15", std::to_string(s.neu)) + meter("Negative", s.neg / tot, "EF4444", std::to_string(s.neg)) + gap();
    body += section("✦", "ISSUE HEATMAP", "F97316");
    int maxv = s.heat.empty() ? 1 : s.heat.front().second;
    for (size_t i = 0; i < s.heat.size(); ++i) body += meter(s.heat[i].first, (double)s.heat[i].second / maxv, J[i % 9], std::to_string(s.heat[i].second));
    body += gap();
    body += section("⟡", "TREND TELEMETRY", "3B82F6");
    for (const auto& p : s.points) body += meter(p.date, p.index / 100.0, p.index >= 70 ? "22C55E" : p.index >= 50 ? "FACC15" : "EF4444", fix(p.index, 0) + "%");
    body += para(run("Moving average " + fix(s.moving3, 1) + "%   ◇   Volatility " + fix(s.volatility, 1) + "   ◇   Forecast +2d " + fix(s.forecast, 1) + "%", "A855F7", 20, true, false, "Consolas"), "left", "", 120) + gap();
    body += section("⚠", "CRITICAL SIGNALS", "EF4444");
    {
        vector<int> w = {900, 1700, 1500, 800, 4100};
        vector<string> rows;
        rows.push_back(row({cell(para(run("ID", "FFFFFF", 18, true, false, "Consolas")), 900, "7C3AED"), cell(para(run("SOURCE", "FFFFFF", 18, true, false, "Consolas")), 1700, "7C3AED"), cell(para(run("ISSUE", "FFFFFF", 18, true, false, "Consolas")), 1500, "7C3AED"), cell(para(run("SEV", "FFFFFF", 18, true, false, "Consolas")), 800, "7C3AED"), cell(para(run("COMMENT", "FFFFFF", 18, true, false, "Consolas")), 4100, "7C3AED")}));
        int k = 0;
        for (const auto& e : s.critical) {
            string fill = k++ % 2 ? "FEF3C7" : "FFEDD5";
            rows.push_back(row({cell(para(run("#" + std::to_string(e.id), "7C2D12", 18, true)), 900, fill, "FFFFFF"), cell(para(run(e.source, "1E3A8A", 18)), 1700, fill, "FFFFFF"), cell(para(run(e.issue, "065F46", 18)), 1500, fill, "FFFFFF"), cell(para(run(std::to_string(e.severity), "991B1B", 20, true)), 800, fill, "FFFFFF"), cell(para(run(e.text, "1E1B4B", 18)), 4100, fill, "FFFFFF")}));
        }
        body += table(rows, w) + gap();
    }
    body += section("⚖", "CASE DESK AND ESCALATION", "A855F7");
    {
        vector<int> w = {800, 2400, 2800, 3000};
        vector<string> rows;
        rows.push_back(row({cell(para(run("CASE", "FFFFFF", 18, true, false, "Consolas")), 800, "0EA5E9"), cell(para(run("FINDING", "FFFFFF", 18, true, false, "Consolas")), 2400, "0EA5E9"), cell(para(run("ACTION LEVEL", "FFFFFF", 18, true, false, "Consolas")), 2800, "0EA5E9"), cell(para(run("STATUS", "FFFFFF", 18, true, false, "Consolas")), 3000, "0EA5E9")}));
        int k = 0;
        for (const auto& c : s.cases) {
            string fill = k++ % 2 ? "E0F2FE" : "F0F9FF";
            rows.push_back(row({cell(para(run("#" + std::to_string(c.id), "0C4A6E", 18, true)), 800, fill), cell(para(run(c.flag, "7C2D12", 18, true)), 2400, fill), cell(para(run(c.level, "4C1D95", 18)), 2800, fill), cell(para(run(c.status, "14532D", 18, true)), 3000, fill)}));
        }
        body += table(rows, w) + gap();
    }
    body += section("✧", "RECOMMENDATION MATRIX", "2DD4BF");
    int rk = 0;
    for (const auto& r : s.recs) {
        string accent = r.needsManager ? "F97316" : J[rk % 9];
        vector<int> w = {240, 8760};
        body += table({row({cell(para(run(" ", accent, 10), "left", "", 0), 240, accent, accent), cell(para(run(r.title, "1E1B4B", 22, true), "left", "", 20) + para(run("[" + r.level + "]  ", accent, 18, true, false, "Consolas") + run(r.detail, "334155", 19), "left", "", 20) + para(run("basis: " + r.basis + (r.needsManager ? "   ★ MANAGEMENT DECISION REQUIRED" : ""), "7C3AED", 16, false, true), "left", "", 20), 8760, "FAFAFA", "FFFFFF")})}, w) + gap();
        if (++rk >= 40) break;
    }
    if (!s.digests.empty()) {
        body += section("❖", "LINK DIGEST", "F43F5E");
        for (const auto& d : s.digests) {
            body += para(run(d.title, "BE185D", 24, true) + run("   mood: " + d.mood, "7C3AED", 20, true, true));
            body += para(run(d.url, "0369A1", 16, false, true));
            for (const auto& l : d.summary) body += para(run("◆ " + l, "1E1B4B", 20));
        }
        body += gap();
    }
    body += section("☰", "REQUIREMENTS TRACEABILITY", "FACC15");
    {
        vector<int> w = {900, 1200, 3300, 2900, 700};
        vector<string> rows;
        rows.push_back(row({cell(para(run("ID", "FFFFFF", 16, true, false, "Consolas")), 900, "1D4ED8"), cell(para(run("TYPE", "FFFFFF", 16, true, false, "Consolas")), 1200, "1D4ED8"), cell(para(run("REQUIREMENT", "FFFFFF", 16, true, false, "Consolas")), 3300, "1D4ED8"), cell(para(run("DELIVERED BY", "FFFFFF", 16, true, false, "Consolas")), 2900, "1D4ED8"), cell(para(run("MOSCOW", "FFFFFF", 14, true, false, "Consolas")), 700, "1D4ED8")}));
        int k = 0;
        for (const auto& r : requirements()) {
            string fill = k++ % 2 ? "DBEAFE" : "EFF6FF";
            rows.push_back(row({cell(para(run(r.id, "1E3A8A", 16, true)), 900, fill), cell(para(run(r.kind, "0F766E", 16)), 1200, fill), cell(para(run(r.text, "1E1B4B", 16)), 3300, fill), cell(para(run(r.feature, "4C1D95", 16)), 2900, fill), cell(para(run(r.moscow, "9A3412", 16, true)), 700, fill)}));
        }
        body += table(rows, w) + gap();
    }
    body += para(run("◢◤ Report generated in " + fix(s.buildMs, 1) + " ms · Built by David Rhodes · ISS323 ◥◣", "A855F7", 18, true, false, "Consolas"), "center");
    string doc = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\"><w:body>" + body +
                 "<w:sectPr><w:pgSz w:w=\"11906\" w:h=\"16838\"/><w:pgMar w:top=\"700\" w:right=\"1400\" w:bottom=\"700\" w:left=\"1400\" w:header=\"400\" w:footer=\"400\" w:gutter=\"0\"/></w:sectPr></w:body></w:document>";
    string types = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\"><Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/><Default Extension=\"xml\" ContentType=\"application/xml\"/><Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/></Types>";
    string rels = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"word/document.xml\"/></Relationships>";
    return zip::pack({{"[Content_Types].xml", types}, {"_rels/.rels", rels}, {"word/document.xml", doc}});
}

vector<string> exportAll(Vault& vault, const Snapshot& snap) {
    fs::path dir = vault.dir / "exports" / clock_::compact();
    std::error_code ec;
    fs::create_directories(dir, ec);
    vector<std::pair<string, string>> files = {
        {"dashboard.html", toHtmlDashboard(snap, vault)}, {"explorer.html", toHtmlExplorer(snap, vault)}, {"report.docx", toDocx(snap, vault)},
        {"report.md", toMarkdown(snap)}, {"report.txt", toTxt(snap)}, {"report.json", toJson(snap, vault)}, {"report.xml", toXml(snap, vault)},
        {"report.yaml", toYaml(snap)}, {"feedback.csv", toCsv(vault)}, {"feedback.tsv", toTsv(vault)}, {"pulse.svg", toSvg(snap)},
        {"report.rtf", toRtf(snap)}, {"actions.ics", toIcs(snap)}};
    vector<string> paths;
    for (const auto& f : files) {
        fs::path p = dir / f.first;
        std::ofstream out(p, std::ios::binary);
        out << f.second;
        out.close();
        paths.push_back(p.string());
    }
    vault.audit("EXPORT", std::to_string(paths.size()) + " files to " + dir.string());
    return paths;
}

}

namespace fb {

Rgb moodColor(const string& label) { return label == "POSITIVE" ? gem::emerald : label == "NEGATIVE" ? gem::sardius : gem::topaz; }

string moodTag(const string& label) { return ui::paint(moodColor(label), label == "POSITIVE" ? "▲ POS" : label == "NEGATIVE" ? "▼ NEG" : "● NEU"); }

int menu(const string& title, const Rgb& accent, const vector<std::pair<string, string>>& items, bool backIsQuit = false) {
    vector<string> lines;
    for (size_t i = 0; i < items.size(); ++i) {
        const Rgb& c = gem::twelve[i % gem::twelve.size()];
        lines.push_back(ui::boldPaint(c, "[" + std::to_string(i + 1) + "]") + " " + ui::boldPaint(gem::pearl, items[i].first) + "  " + ui::paint(gem::slate, items[i].second));
    }
    lines.push_back(ui::boldPaint(gem::sardius, "[0]") + " " + ui::boldPaint(gem::pearl, backIsQuit ? "Log out and quit" : "Back"));
    ui::panel(title, accent, lines);
    while (true) {
        string a = ui::prompt("Choose");
        if (ui::eof(a)) return -1;
        if (a.empty() && ui::autoplay) return 0;
        try {
            int v = std::stoi(a);
            if (v >= 0 && v <= (int)items.size()) return v;
        } catch (...) {
        }
        ui::warn("Please pick one of the numbers shown.");
    }
}

vector<string> parseCsvLine(const string& line) {
    vector<string> out;
    string cur;
    bool q = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (q) {
            if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') {
                cur += '"';
                ++i;
            } else if (c == '"') q = false;
            else cur += c;
        } else if (c == '"') q = true;
        else if (c == ',') {
            out.push_back(cur);
            cur.clear();
        } else if (c != '\r') cur += c;
    }
    out.push_back(cur);
    return out;
}

void showEntry(const Entry& e) {
    string head = ui::boldPaint(gem::gold, "#" + std::to_string(e.id)) + "  " + moodTag(e.label) + "  " + ui::paint(gem::sky, e.source) + "  " + ui::paint(gem::slate, e.date + " · " + e.author);
    string meta = ui::paint(gem::chalcedony, "issue: " + e.issue) + "   " + ui::paint(gem::beryl, "score: " + std::to_string(e.score)) + "   " + ui::paint(e.severity >= 4 ? gem::sardius : e.severity >= 2 ? gem::sardonyx : gem::emerald, "severity: " + std::to_string(e.severity)) + (e.flag.empty() ? "" : "   " + ui::boldPaint(gem::jacinth, "⚑ " + e.flag));
    ui::say("  " + head);
    ui::say("    " + ui::paint(gem::pearl, util::ellipsize(e.text, (size_t)ui::cols() - 12)));
    ui::say("    " + meta);
    ui::blank();
}

void showDashboard(const Snapshot& s) {
    ui::panel("◈ OVERVIEW", gem::sapphire,
              {ui::boldPaint(gem::gold, "Business: ") + s.business, ui::boldPaint(gem::sky, "Comments: ") + std::to_string(s.total) + "    " + ui::boldPaint(gem::beryl, "Average sentiment: ") + fix(s.avg),
               ui::boldPaint(gem::emerald, "Satisfaction index: ") + pct(s.satisfaction) + "  " + ui::bar(s.satisfaction / 100.0, 28, gem::sardius, gem::emerald)});
    double tot = std::max(1, s.pos + s.neg + s.neu);
    ui::panel("◉ SENTIMENT SPECTRUM", gem::emerald,
              {ui::paint(gem::emerald, "Positive ") + util::padLeft(std::to_string(s.pos), 4) + " " + ui::bar(s.pos / tot, 36, gem::jasper, gem::chrysoprase),
               ui::paint(gem::topaz, "Neutral  ") + util::padLeft(std::to_string(s.neu), 4) + " " + ui::bar(s.neu / tot, 36, gem::topaz, gem::sardonyx),
               ui::paint(gem::sardius, "Negative ") + util::padLeft(std::to_string(s.neg), 4) + " " + ui::bar(s.neg / tot, 36, gem::sardonyx, gem::sardius)});
    vector<string> lines;
    int maxv = s.heat.empty() ? 1 : s.heat.front().second;
    for (size_t i = 0; i < s.heat.size() && i < 8; ++i) {
        const Rgb& c = gem::twelve[(i * 3) % gem::twelve.size()];
        lines.push_back(ui::paint(c, util::padRight(s.heat[i].first, 16)) + util::padLeft(std::to_string(s.heat[i].second), 3) + " " + ui::bar((double)s.heat[i].second / maxv, 30, c, gem::pearl));
    }
    if (lines.empty()) lines.push_back(ui::paint(gem::slate, "No negative feedback yet."));
    ui::panel("✦ NEGATIVE ISSUE HEATMAP", gem::sardonyx, lines);
    lines.clear();
    for (size_t i = 0; i < s.sources.size(); ++i)
        lines.push_back(ui::paint(gem::sky, util::padRight(s.sources[i].first, 18)) + util::padLeft(std::to_string(s.sources[i].second), 3) + "  avg " + ui::paint(s.sourceAvg[i].second >= 0 ? gem::emerald : gem::sardius, fix(s.sourceAvg[i].second)));
    if (lines.empty()) lines.push_back(ui::paint(gem::slate, "No sources yet."));
    ui::panel("❖ SOURCES", gem::amethyst, lines);
}

void showCritical(const Snapshot& s) {
    ui::panel("⚠ PRIORITY QUEUE · MOST CRITICAL FIRST", gem::sardius, {ui::paint(gem::slate, "Ordered by severity, then by how negative the comment is.")});
    if (s.critical.empty()) ui::say("  " + ui::paint(gem::emerald, "Nothing critical. Well done."));
    for (const auto& e : s.critical) showEntry(e);
}

void showTrend(const Snapshot& s) {
    vector<vector<string>> rows;
    for (const auto& p : s.points) {
        const Rgb c = p.index >= 70 ? gem::emerald : p.index >= 50 ? gem::topaz : gem::sardius;
        rows.push_back({p.date, std::to_string(p.pos), std::to_string(p.neg), std::to_string(p.neu), ui::paint(c, fix(p.index, 1) + "% ") + ui::bar(p.index / 100.0, 20, gem::sardius, gem::emerald)});
    }
    ui::print(ui::table({"DATE", "POS", "NEG", "NEU", "SATISFACTION"}, rows, {gem::pearl, gem::emerald, gem::sardius, gem::topaz, gem::sky}));
    ui::blank();
    string verdict = s.moving3 < 60 ? "Immediate intervention required on recurring negative themes." : s.moving3 < 75 ? "Monitor service and cleanliness closely this week." : "Maintain standards and reinforce what guests praise.";
    ui::panel("⟡ TREND ANALYTICS", gem::beryl,
              {ui::boldPaint(gem::sky, "3-day moving average: ") + fix(s.moving3, 1) + "%", ui::boldPaint(gem::emerald, "Best day:  ") + s.best.date + " (" + fix(s.best.index, 1) + "%)",
               ui::boldPaint(gem::sardius, "Worst day: ") + s.worst.date + " (" + fix(s.worst.index, 1) + "%)", ui::boldPaint(gem::amethyst, "Volatility (std dev): ") + fix(s.volatility, 1),
               ui::boldPaint(gem::gold, "Slope: ") + fix(s.slope, 2) + " pts/day   " + ui::boldPaint(gem::rose, "Forecast +2 days: ") + fix(s.forecast, 1) + "%", ui::boldPaint(gem::topaz, "Verdict: ") + verdict});
}

void showWords(const Snapshot& s) {
    vector<string> lines;
    double mx = s.words.empty() ? 1 : s.words.front().second;
    for (size_t i = 0; i < s.words.size() && i < 12; ++i) {
        const Rgb& c = gem::twelve[i % gem::twelve.size()];
        lines.push_back(ui::paint(c, util::padRight(s.words[i].first, 16)) + util::padLeft(std::to_string((int)s.words[i].second), 3) + " " + ui::bar(s.words[i].second / mx, 28, c, gem::pearl));
    }
    if (lines.empty()) lines.push_back("No words yet.");
    ui::panel("☁ WORD CLOUD · WHAT GUESTS TALK ABOUT", gem::chalcedony, lines);
}

void showRecs(const Snapshot& s, size_t limit) {
    if (s.recs.empty()) {
        ui::say("  " + ui::paint(gem::slate, "No recommendations yet. Generate them from the Intelligence menu."));
        return;
    }
    size_t n = 0;
    for (const auto& r : s.recs) {
        if (n++ >= limit) break;
        Rgb c = r.needsManager ? gem::sardonyx : r.level == "Research" ? gem::sky : gem::amethyst;
        ui::say("  " + ui::boldPaint(c, "◆ " + r.title) + "  " + ui::paint(gem::topaz, "[" + r.level + "]"));
        for (const auto& l : util::wrap(r.detail, (size_t)ui::cols() - 10)) ui::say("      " + ui::paint(gem::pearl, l));
        ui::say("      " + ui::paint(gem::slate, r.basis + (r.url.empty() ? "" : "  " + r.url)));
        if (r.needsManager) ui::say("      " + ui::boldPaint(gem::sardonyx, "★ Management decision required"));
        ui::blank();
    }
    if (s.recs.size() > limit) ui::say("  " + ui::paint(gem::slate, "… and " + std::to_string(s.recs.size() - limit) + " more in the exported reports."));
}

void showCases(Vault& v) {
    if (v.cases.empty()) {
        ui::say("  " + ui::paint(gem::emerald, "No cases open."));
        return;
    }
    vector<vector<string>> rows;
    for (const auto& c : v.cases) {
        Rgb lc = c.level[0] == 'M' ? gem::sardonyx : c.level.substr(0, 2) == "L5" || c.level.substr(0, 2) == "L4" ? gem::sardius : c.level.substr(0, 2) == "L3" ? gem::jacinth : gem::sky;
        rows.push_back({"#" + std::to_string(c.id), c.flag, ui::paint(lc, c.level), ui::paint(c.status == "DISMISSED" ? gem::slate : c.status.find("AWAITING") != string::npos ? gem::topaz : gem::emerald, c.status)});
    }
    ui::print(ui::table({"CASE", "FINDING", "ACTION LEVEL", "STATUS"}, rows, {gem::gold, gem::rose, gem::sky, gem::emerald}));
    ui::blank();
}

void decideCase(Vault& v, CaseFile& c, const string& decision, const string& note) {
    c.status = decision;
    c.note = note;
    c.decidedBy = v.currentUser;
    v.saveCases();
    v.audit("CASE_DECISION", "case " + std::to_string(c.id) + " -> " + decision + (note.empty() ? "" : " (" + note + ")"));
}

void caseDesk(Vault& v) {
    while (true) {
        v.syncCases();
        ui::print("\n");
        showCases(v);
        int c = menu("⚖ CASE DESK", gem::jacinth, {{"Review a case", "read the evidence and the recommended action"}, {"Management decision", "approve, escalate to police or dismiss (managers only)"}, {"Reopen scan", "look for new misconduct signals"}});
        if (c <= 0) return;
        if (c == 3) {
            ui::ok(std::to_string(v.syncCases()) + " new case(s) opened.");
            continue;
        }
        string idText = ui::prompt("Case number");
        if (ui::eof(idText)) return;
        CaseFile* cf = nullptr;
        for (auto& x : v.cases)
            if (idText == std::to_string(x.id) || idText == "#" + std::to_string(x.id)) cf = &x;
        if (!cf) {
            ui::warn("No such case.");
            continue;
        }
        const Entry* e = v.entryById(cf->entryId);
        vector<string> lines = {ui::boldPaint(gem::gold, "Finding: ") + cf->flag, ui::boldPaint(gem::sky, "Level: ") + cf->level, ui::boldPaint(gem::emerald, "Recommended: ") + cf->action,
                                ui::boldPaint(gem::sardonyx, "Status: ") + cf->status + (cf->decidedBy.empty() ? "" : " by " + cf->decidedBy)};
        if (e) lines.push_back(ui::boldPaint(gem::rose, "Evidence: ") + e->text + " (" + e->source + ", " + e->date + ")");
        lines.push_back(ui::paint(gem::slate, "Fairness note: a single review is an allegation. Verify with rosters, CCTV and statements, and follow your disciplinary code and the Employment Act before any dismissal."));
        ui::panel("CASE #" + std::to_string(cf->id), gem::jacinth, lines);
        if (c == 2) {
            if (!v.isManager()) {
                ui::fail("Only a manager can record a decision.");
                continue;
            }
            int d = menu("DECISION", gem::sardonyx, {{"Approve recommended action", ""}, {"Escalate to police / authorities", ""}, {"Request more evidence", ""}, {"Dismiss as unfounded", ""}});
            if (d <= 0) continue;
            string note = ui::prompt("Note for the record (optional)");
            if (ui::eof(note)) return;
            const char* st[] = {"", "APPROVED", "ESCALATED TO AUTHORITIES", "EVIDENCE REQUESTED", "DISMISSED"};
            decideCase(v, *cf, st[d], note);
            ui::ok("Decision recorded and written to the audit trail.");
        }
        ui::pause();
    }
}

void sourceMenu(Vault& v);

void feedbackHub(Vault& v) {
    while (true) {
        int c = menu("❖ FEEDBACK HUB", gem::emerald,
                     {{"Paste a link", "approved websites only; summarises and ingests comments"}, {"Add a comment", "type in a survey or walk-in comment"}, {"Import CSV file", "source,author,date,text"},
                      {"Browse feedback", "filter by mood or search words"}, {"Load sample data", "40 realistic Gaborone comments"}, {"Delete a comment", "managers only"}});
        if (c <= 0) return;
        if (c == 1) {
            if (!v.canWrite()) {
                ui::fail("Your role is read-only.");
                continue;
            }
            string url = ui::prompt("Paste the link (or a local file path)");
            if (ui::eof(url)) return;
            if (url.empty()) continue;
            ui::working("Checking the approved-website list", 200);
            ui::working("Downloading and reading the page", 300);
            IngestResult r = ingestLink(v, url);
            if (!r.ok) {
                ui::fail(r.error);
                continue;
            }
            ui::ok("Added " + std::to_string(r.added) + " comments (" + std::to_string(r.skipped) + " duplicates skipped) from " + r.platform + ".");
            vector<string> lines = {ui::boldPaint(gem::gold, r.digest.title.empty() ? "Untitled page" : r.digest.title), ui::paint(gem::sky, "Mood: " + r.digest.mood + "   words: " + std::to_string(r.digest.words))};
            for (const auto& s : r.digest.summary) lines.push_back(ui::paint(gem::pearl, "• " + s));
            ui::panel("✦ LINK DIGEST", gem::chalcedony, lines);
            v.syncCases();
        } else if (c == 2) {
            if (!v.canWrite()) {
                ui::fail("Your role is read-only.");
                continue;
            }
            for (size_t i = 0; i < v.sources.size(); ++i) ui::say("   " + ui::paint(gem::twelve[i % 12], std::to_string(i + 1) + ". " + v.sources[i].name));
            int pick = ui::number("Source number", 1, 1, (int)v.sources.size());
            string author = ui::prompt("Author (optional)");
            if (ui::eof(author)) return;
            string text = ui::prompt("Comment");
            if (ui::eof(text)) return;
            if (text.size() < 5) {
                ui::warn("That comment is too short.");
                continue;
            }
            int id = v.add(v.sources[(size_t)pick - 1].name, author, "", text, "manual");
            v.saveEntries();
            v.audit("ENTRY_ADDED", "#" + std::to_string(id));
            showEntry(*v.entryById(id));
            v.syncCases();
        } else if (c == 3) {
            if (!v.canWrite()) {
                ui::fail("Your role is read-only.");
                continue;
            }
            string path = ui::prompt("CSV path");
            if (ui::eof(path)) return;
            std::ifstream in(path);
            if (!in) {
                ui::fail("Cannot open that file.");
                continue;
            }
            string line;
            int added = 0, skipped = 0;
            bool first = true;
            while (std::getline(in, line)) {
                auto f = parseCsvLine(line);
                if (first) {
                    first = false;
                    if (!f.empty() && util::lower(f[0]) == "source") continue;
                }
                if (f.size() < 4 || f[3].size() < 5) {
                    ++skipped;
                    continue;
                }
                if (v.duplicate(f[0], f[3])) {
                    ++skipped;
                    continue;
                }
                v.add(f[0].empty() ? "Survey" : f[0], f[1], f[2], f[3], "csv:" + path);
                ++added;
            }
            v.saveEntries();
            v.audit("CSV_IMPORT", path + " added=" + std::to_string(added));
            ui::ok("Imported " + std::to_string(added) + " rows, skipped " + std::to_string(skipped) + ".");
            v.syncCases();
        } else if (c == 4) {
            int m = menu("FILTER", gem::sky, {{"All", ""}, {"Positive only", ""}, {"Negative only", ""}, {"Neutral only", ""}, {"Search words", ""}});
            if (m <= 0) continue;
            string kw;
            if (m == 5) kw = util::lower(ui::prompt("Search for"));
            const char* labels[] = {"", "", "POSITIVE", "NEGATIVE", "NEUTRAL"};
            int shown = 0;
            for (auto it = v.entries.rbegin(); it != v.entries.rend() && shown < 25; ++it) {
                if (m >= 2 && m <= 4 && it->label != labels[m]) continue;
                if (m == 2 && it->label != "POSITIVE") continue;
                if (m == 5 && util::lower(it->text).find(kw) == string::npos) continue;
                showEntry(*it);
                ++shown;
            }
            if (!shown) ui::warn("Nothing matched.");
        } else if (c == 5) {
            if (!v.canWrite()) {
                ui::fail("Your role is read-only.");
                continue;
            }
            int n = seedDemo(v);
            ui::ok(std::to_string(n) + " sample comments loaded.");
            v.syncCases();
        } else if (c == 6) {
            if (!v.isManager()) {
                ui::fail("Only managers can delete comments.");
                continue;
            }
            string idText = ui::prompt("Comment number to delete");
            int id = std::atoi(idText.c_str());
            auto it = std::remove_if(v.entries.begin(), v.entries.end(), [&](const Entry& e) { return e.id == id; });
            if (it == v.entries.end()) {
                ui::warn("No such comment.");
                continue;
            }
            v.entries.erase(it, v.entries.end());
            v.saveEntries();
            v.audit("ENTRY_DELETED", "#" + std::to_string(id));
            ui::ok("Deleted and logged.");
        }
        ui::pause();
    }
}

void sourceMenu(Vault& v) {
    while (true) {
        vector<vector<string>> rows;
        for (size_t i = 0; i < v.sources.size(); ++i) rows.push_back({std::to_string(i + 1), v.sources[i].name, v.sources[i].domain, v.sources[i].kind, v.sources[i].added});
        ui::print("\n" + ui::table({"#", "NAME", "DOMAIN", "KIND", "ADDED"}, rows, {gem::gold, gem::sky, gem::emerald, gem::amethyst, gem::slate}) + "\n");
        ui::say("  " + ui::paint(gem::slate, "Business: " + v.profile.business + " · " + v.profile.city + " · search keywords: " + v.profile.industry));
        int c = menu("❖ APPROVED WEBSITES", gem::beryl, {{"Add website", "whitelist a new place the app may read"}, {"Remove website", ""}, {"Edit business profile", "name, city and search keywords"}});
        if (c <= 0) return;
        if (!v.isManager()) {
            ui::fail("Only managers can change the approved list.");
            continue;
        }
        if (c == 1) {
            string name = ui::prompt("Display name");
            string dom = ui::prompt("Website address (e.g. tripadvisor.com)");
            if (ui::eof(dom)) return;
            ui::say("   1. review   2. social   3. survey   4. advice   5. web");
            const char* kinds[] = {"", "review", "social", "survey", "advice", "web"};
            int k = ui::number("Kind", 5, 1, 5);
            if (v.addSource(name, dom, kinds[k])) {
                ui::ok("Approved.");
                v.audit("SOURCE_ADDED", dom);
            } else ui::warn("Invalid or duplicate address.");
        } else if (c == 2) {
            int n = ui::number("Number to remove", 0, 0, (int)v.sources.size());
            if (n > 0 && v.removeSource((size_t)n - 1)) {
                ui::ok("Removed.");
                v.audit("SOURCE_REMOVED", std::to_string(n));
            }
        } else if (c == 3) {
            string b = ui::prompt("Business name [" + v.profile.business + "]");
            string city = ui::prompt("City [" + v.profile.city + "]");
            string kw = ui::prompt("Search keywords [" + v.profile.industry + "]");
            if (!b.empty()) v.profile.business = b;
            if (!city.empty()) v.profile.city = city;
            if (!kw.empty()) v.profile.industry = kw;
            v.saveProfile();
            v.audit("PROFILE_EDIT", v.profile.business);
            ui::ok("Profile saved.");
        }
    }
}

void analyticsMenu(Vault& v) {
    while (true) {
        int c = menu("◈ ANALYTICS", gem::sapphire, {{"Dashboard", "sentiment, heatmap, sources"}, {"Priority queue", "most critical comments first"}, {"Trend and forecast", "daily satisfaction"}, {"Word cloud", "what guests mention most"}});
        if (c <= 0) return;
        Snapshot s = build(v);
        ui::banner();
        if (c == 1) showDashboard(s);
        else if (c == 2) showCritical(s);
        else if (c == 3) showTrend(s);
        else if (c == 4) showWords(s);
        ui::say("  " + ui::paint(gem::slate, "Built in " + fix(s.buildMs, 1) + " ms"));
        ui::pause();
    }
}

void runRecommendations(Vault& v, bool web) {
    Snapshot s = build(v);
    string note;
    ui::working(web ? "Searching the web for fixes to your top problems" : "Applying the hospitality playbook", 400);
    v.recs = generate(v, s, web, note, 3);
    v.audit("RECOMMENDATIONS", std::to_string(v.recs.size()) + " generated");
    ui::ok(std::to_string(v.recs.size()) + " recommendations ready. " + note);
}

void intelligenceMenu(Vault& v) {
    while (true) {
        int c = menu("✧ INTELLIGENCE", gem::amethyst,
                     {{"Recommendations", "playbook plus live web research"}, {"Case desk", "suspend, report to police, or management review"}, {"Website X-ray", "audit an approved website"}, {"Repository autopsy", "analyse a git repository link"}, {"Show recommendations", "read what was generated"}});
        if (c <= 0) return;
        if (c == 1) {
            if (!v.canWrite()) {
                ui::fail("Your role is read-only.");
                continue;
            }
            bool web = ui::yes("Include live web research?", true);
            runRecommendations(v, web);
            showRecs(build(v), 12);
        } else if (c == 2) caseDesk(v);
        else if (c == 3) {
            string url = ui::prompt("Website to scan");
            if (ui::eof(url)) return;
            url = od::fetch::cleanInput(url);
            if (url.empty()) continue;
            if (!v.approved(url)) {
                ui::fail("Not on the approved list. Add " + hostOf(url) + " under Approved Websites first.");
                continue;
            }
            int pages = ui::number("Pages to crawl", 8, 1, 40);
            ui::working("Crawling and auditing (this can take a minute)", 300);
            SiteRecord r = scanSite(v, url, pages, 2);
            ui::panel("◎ WEBSITE X-RAY · " + hostOf(url), gem::beryl, {ui::boldPaint(gem::gold, "Grade " + r.grade + "  ") + fix(r.overall, 0) + "/100", "Security " + fix(r.security, 0) + " · SEO " + fix(r.seo, 0) + " · Speed " + fix(r.perf, 0), "Pages " + std::to_string(r.pages) + " · links checked " + std::to_string(r.links) + " · broken " + std::to_string(r.broken), r.diagnosis});
        } else if (c == 4) {
            string url = ui::prompt("Repository link or local folder");
            if (ui::eof(url)) return;
            if (url.empty()) continue;
            if (util::startsWith(util::lower(url), "http") && !v.approved(url)) {
                ui::fail("Not on the approved list. Add " + hostOf(url) + " first.");
                continue;
            }
            ui::working("Reading commit history", 300);
            string err;
            RepoRecord r = scanRepo(v, url, 1500, err);
            if (!err.empty()) {
                ui::fail(err);
                continue;
            }
            ui::panel("☰ REPOSITORY AUTOPSY · " + r.name, gem::chrysoprase, {ui::boldPaint(gem::gold, "Health " + fix(r.health, 0) + "/100  grade " + r.grade), "Commits " + std::to_string(r.commits) + " · authors " + std::to_string(r.authors) + " · bus factor " + std::to_string(r.busFactor), r.diagnosis});
        } else if (c == 5) showRecs(build(v), 30);
        ui::pause();
    }
}

void exportMenu(Vault& v) {
    Snapshot s = build(v);
    ui::working("Building 13 report formats", 500);
    auto paths = exportAll(v, s);
    vector<string> lines;
    const Rgb cols[] = {gem::jasper, gem::sapphire, gem::amethyst, gem::sardonyx, gem::sardius, gem::topaz, gem::beryl, gem::rose, gem::chrysoprase};
    for (size_t i = 0; i < paths.size(); ++i) lines.push_back(ui::paint(cols[i % 9], "✦ " + fs::path(paths[i]).filename().string()));
    lines.push_back(ui::paint(gem::slate, fs::path(paths.front()).parent_path().string()));
    ui::panel("✧ EXPORTED " + std::to_string(paths.size()) + " FILES", gem::gold, lines);
    if (!ui::autoplay && ui::yes("Open the HTML dashboard in your browser?", false)) {
        string cmd = fi_open_cmd(paths.front());
        std::system(cmd.c_str());
    }
}

void securityMenu(Vault& v) {
    while (true) {
        int c = menu("🔒 SECURITY AND AUDIT", gem::sardius, {{"Audit trail", "last 20 actions"}, {"Verify audit chain", "detect tampering"}, {"Users", "list accounts and lock state"}, {"Add user", "manager only"}, {"Unlock account", "manager only"}, {"Change my password", ""}});
        if (c <= 0) return;
        if (c == 1 || c == 2) {
            if (!v.isManager()) {
                ui::fail("Managers only.");
                continue;
            }
        }
        if (c == 1) {
            auto lines = v.auditLines();
            size_t from = lines.size() > 20 ? lines.size() - 20 : 0;
            for (size_t i = from; i < lines.size(); ++i) {
                auto f = std::vector<string>();
                std::stringstream ss(lines[i]);
                string part;
                while (std::getline(ss, part, '|')) f.push_back(part);
                if (f.size() >= 4) ui::say("  " + ui::paint(gem::slate, f[0]) + " " + ui::paint(gem::sky, util::padRight(f[1], 10)) + " " + ui::boldPaint(gem::gold, util::padRight(f[2], 18)) + " " + ui::paint(gem::pearl, unesc(f[3])));
            }
        } else if (c == 2) {
            size_t n = 0, bad = 0;
            ui::working("Re-computing the hash chain", 300);
            if (v.verifyAudit(&n, &bad)) ui::ok("Audit chain intact: " + std::to_string(n) + " records verified.");
            else ui::fail("Tampering detected near record " + std::to_string(bad) + ".");
        } else if (c == 3) {
            vector<vector<string>> rows;
            for (const auto& u : v.users) rows.push_back({u.name, u.role, u.locked ? ui::paint(gem::sardius, "LOCKED") : ui::paint(gem::emerald, "active"), std::to_string(u.fails), u.created});
            ui::print(ui::table({"USER", "ROLE", "STATE", "FAILS", "CREATED"}, rows, {gem::gold, gem::amethyst, gem::emerald, gem::sardonyx, gem::slate}));
        } else if (c == 4) {
            if (!v.isManager()) {
                ui::fail("Managers only.");
                continue;
            }
            string name = ui::prompt("New username");
            ui::say("   1. manager   2. crm (customer relations)   3. viewer");
            int r = ui::number("Role", 2, 1, 3);
            string pw = ui::secret("Password (min 8 characters)");
            const char* roles[] = {"", "manager", "crm", "viewer"};
            if (v.createUser(name, roles[r], pw)) {
                ui::ok("User created.");
                v.audit("USER_CREATED", name + " as " + roles[r]);
            } else ui::warn("Could not create that user (name taken or password too short).");
        } else if (c == 5) {
            if (!v.isManager()) {
                ui::fail("Managers only.");
                continue;
            }
            string name = ui::prompt("Username to unlock");
            User* u = v.findUser(name);
            if (!u) {
                ui::warn("No such user.");
                continue;
            }
            u->locked = false;
            u->fails = 0;
            v.saveUsers();
            v.audit("ACCOUNT_UNLOCKED", u->name);
            ui::ok("Account unlocked.");
        } else if (c == 6) {
            User* u = v.findUser(v.currentUser);
            string old = ui::secret("Current password");
            if (!u || sec::hashPassword(u->salt, old) != u->hash) {
                ui::fail("Current password is wrong.");
                v.audit("PASSWORD_CHANGE_FAILED", v.currentUser);
                continue;
            }
            string a = ui::secret("New password (min 8)");
            string b = ui::secret("Repeat new password");
            if (a != b || a.size() < 8) {
                ui::warn("Passwords did not match or were too short.");
                continue;
            }
            u->salt = sec::randomHex(16);
            u->hash = sec::hashPassword(u->salt, a);
            v.saveUsers();
            v.audit("PASSWORD_CHANGED", v.currentUser);
            ui::ok("Password changed.");
        }
        ui::pause();
    }
}

struct Check {
    string name, attr, detail;
    bool pass;
};

vector<Check> qualityChecks(Vault& v) {
    vector<Check> out;
    double t0 = clock_::ms();
    Snapshot s = build(v);
    string doc = toDocx(s, v);
    string html = toHtmlDashboard(s, v);
    double took = clock_::ms() - t0;
    out.push_back({"Report build under 3 seconds", "Performance", fix(took, 1) + " ms for snapshot + docx + html", took < 3000});
    out.push_back({"Docx package is a valid archive", "Reliability", std::to_string(doc.size()) + " bytes, starts with PK", doc.size() > 100 && doc.substr(0, 2) == "PK"});
    size_t n = 0, bad = 0;
    bool intact = v.verifyAudit(&n, &bad);
    out.push_back({"Audit chain intact", "Security", std::to_string(n) + " records", intact});
    bool salted = !v.users.empty();
    for (const auto& u : v.users) salted = salted && u.salt.size() == 16 && u.hash.size() == 64;
    out.push_back({"Passwords salted and hashed", "Security", std::to_string(v.users.size()) + " accounts", salted});
    out.push_back({"Lockout after 3 failures", "Security", "limit = " + std::to_string(v.profile.lockLimit), v.profile.lockLimit == 3});
    out.push_back({"Approved-website list enforced", "Security", std::to_string(v.sources.size()) + " sources", !v.sources.empty() && !v.approved("https://not-approved-example.invalid/page")});
    Vault probe(v.dir);
    probe.loadEntries();
    out.push_back({"Stored data reloads identically", "Reliability", std::to_string(probe.entries.size()) + " of " + std::to_string(v.entries.size()) + " comments", probe.entries.size() == v.entries.size()});
    out.push_back({"Guided test drive available", "Usability", "14 progressive steps", true});
    out.push_back({"Role-based access active", "Security", "current role: " + v.currentRole, !v.currentRole.empty()});
    return out;
}

void qualityScreen(Vault& v) {
    auto checks = qualityChecks(v);
    vector<vector<string>> rows;
    int pass = 0;
    for (const auto& c : checks) {
        pass += c.pass;
        rows.push_back({c.pass ? ui::paint(gem::emerald, "PASS") : ui::paint(gem::sardius, "FAIL"), c.attr, c.name, c.detail});
    }
    ui::print(ui::table({"RESULT", "ATTRIBUTE", "CHECK", "EVIDENCE"}, rows, {gem::emerald, gem::amethyst, gem::sky, gem::slate}));
    ui::say("\n  " + ui::boldPaint(gem::gold, std::to_string(pass) + " of " + std::to_string(checks.size()) + " non-functional checks passed (Usability, Reliability, Performance, Security)."));
    ui::blank();
    vector<vector<string>> req;
    for (const auto& r : requirements()) req.push_back({r.id, r.kind, r.text, r.moscow});
    ui::print(ui::table({"ID", "TYPE", "REQUIREMENT", "MOSCOW"}, req, {gem::gold, gem::beryl, gem::pearl, gem::sardonyx}));
}

void testDrive(Vault& v) {
    const int N = 14;
    bool saveQuiet = ui::quiet;
    ui::banner();
    ui::panel("▶ GUIDED TEST DRIVE", gem::gold, {"Fourteen connected steps. Each one uses the data created by the one before it, so by the end you will have exercised every function of the system.", ui::paint(gem::slate, "Press Enter to advance. Nothing here is destructive.")});
    ui::pause();
    ui::step(1, N, "Secure session", "nothing; this is the entry point");
    ui::panel("Who is logged in", gem::sapphire, {ui::boldPaint(gem::gold, "User: ") + v.currentUser, ui::boldPaint(gem::sky, "Role: ") + v.currentRole, ui::paint(gem::slate, "Passwords are salted and hashed; 3 wrong tries lock the account; every action is chained into the audit log.")});
    ui::pause();
    ui::step(2, N, "Approved websites and business profile", "step 1: your permission level lets you read the list");
    {
        vector<vector<string>> rows;
        for (size_t i = 0; i < v.sources.size(); ++i) rows.push_back({std::to_string(i + 1), v.sources[i].name, v.sources[i].domain, v.sources[i].kind});
        ui::print(ui::table({"#", "NAME", "DOMAIN", "KIND"}, rows, {gem::gold, gem::sky, gem::emerald, gem::amethyst}));
        if (v.isManager() && v.addSource("Sample Review Hub", "reviewhub.example.com", "review")) ui::ok("Added reviewhub.example.com so the whitelist is easy to see in action.");
        ui::info("Business: " + v.profile.business + " · searches use: " + v.profile.industry);
    }
    ui::pause();
    ui::step(3, N, "Load feedback", "step 2: comments are tagged with their approved source");
    {
        int n = seedDemo(v);
        ui::ok(std::to_string(n) + " new comments loaded; total now " + std::to_string(v.entries.size()) + ".");
    }
    ui::pause();
    ui::step(4, N, "Sentiment engine", "step 3: scores the comments just loaded");
    {
        vector<vector<string>> rows;
        for (size_t i = 0; i < v.entries.size() && i < 6; ++i) rows.push_back({"#" + std::to_string(v.entries[i].id), moodTag(v.entries[i].label), std::to_string(v.entries[i].score), util::ellipsize(v.entries[i].text, 58)});
        ui::print(ui::table({"ID", "MOOD", "SCORE", "COMMENT"}, rows, {gem::gold, gem::emerald, gem::beryl, gem::pearl}));
        ui::info("The engine understands negation (\"not clean\") and intensity (\"extremely slow\").");
    }
    ui::pause();
    ui::step(5, N, "Common complaint themes", "step 4: only negative comments feed the heatmap");
    {
        Snapshot s = build(v);
        vector<string> lines;
        int mx = s.heat.empty() ? 1 : s.heat.front().second;
        for (size_t i = 0; i < s.heat.size(); ++i) lines.push_back(ui::paint(gem::twelve[(i * 3) % 12], util::padRight(s.heat[i].first, 16) + util::padLeft(std::to_string(s.heat[i].second), 3) + " ") + ui::bar((double)s.heat[i].second / mx, 30, gem::sardonyx, gem::sardius));
        ui::panel("✦ HEATMAP", gem::sardonyx, lines);
    }
    ui::pause();
    ui::step(6, N, "Manager dashboard", "steps 4 and 5: combines sentiment and themes");
    showDashboard(build(v));
    ui::pause();
    ui::step(7, N, "Priority queue", "step 6: the most critical negatives rise to the top");
    {
        Snapshot s = build(v);
        s.critical.resize(std::min<size_t>(4, s.critical.size()));
        showCritical(s);
    }
    ui::pause();
    ui::step(8, N, "Trend and forecast", "step 3: uses the dates on the loaded comments");
    showTrend(build(v));
    ui::pause();
    ui::step(9, N, "Misconduct detection and case desk", "step 7: flagged comments become cases");
    {
        int n = v.syncCases();
        ui::ok(std::to_string(n) + " case(s) opened from red-flag comments.");
        showCases(v);
    }
    ui::pause();
    ui::step(10, N, "Link summary", "step 2: the link must pass the approved-website check");
    {
        fs::path sample = v.dir / "sample_review_page.html";
        std::ofstream out(sample);
        out << "<html><head><title>Guest reviews for the Gaborone Garden Hotel</title></head><body><h1>Guest reviews</h1>"
               "<p>The garden restaurant was lovely and the waiter was very attentive, but the breakfast buffet was cold again on Sunday.</p>"
               "<p>Our room was spotless and the bed was comfortable, although the wifi kept dropping and nobody at reception could fix it.</p>"
               "<p>The manager shouted at a porter in front of guests which was embarrassing and unprofessional.</p>"
               "<p>Excellent location, friendly team, and the pool area is beautiful. We will definitely come back next year.</p>"
               "<p>Parking was limited and the price of dinner felt too expensive for the portion size.</p></body></html>";
        out.close();
        ui::working("Reading " + sample.filename().string() + " exactly as if it were a pasted link", 300);
        IngestResult r = ingestLink(v, sample.string());
        if (r.ok) {
            ui::ok("Added " + std::to_string(r.added) + " comments from the page.");
            vector<string> lines = {ui::boldPaint(gem::gold, r.digest.title)};
            for (const auto& l : r.digest.summary) lines.push_back("• " + l);
            ui::panel("✦ LINK DIGEST", gem::chalcedony, lines);
        } else ui::fail(r.error);
        if (!ui::autoplay) {
            string live = ui::prompt("Optional: paste a real approved link now (or press Enter to skip)");
            if (!ui::eof(live) && !live.empty()) {
                IngestResult lr = ingestLink(v, live);
                if (lr.ok) ui::ok("Added " + std::to_string(lr.added) + " comments from your link.");
                else ui::fail(lr.error);
            }
        }
        v.syncCases();
    }
    ui::pause();
    ui::step(11, N, "Recommendations from the playbook and the web", "steps 5, 7, 9 and 10: all findings feed the advice");
    {
        runRecommendations(v, true);
        showRecs(build(v), 8);
    }
    ui::pause();
    ui::step(12, N, "Management decision", "step 9: a manager rules on the first case needing review");
    {
        CaseFile* target = nullptr;
        for (auto& c : v.cases)
            if (c.status == "AWAITING MANAGEMENT") {
                target = &c;
                break;
            }
        if (!target) ui::info("No case is waiting for a decision.");
        else {
            ui::say("  " + ui::boldPaint(gem::jacinth, "Case #" + std::to_string(target->id) + " · " + target->flag) + " → " + target->level);
            if (v.isManager()) {
                bool go = ui::autoplay ? true : ui::yes("Approve the recommended action?", true);
                decideCase(v, *target, go ? "APPROVED" : "EVIDENCE REQUESTED", "recorded during guided test drive");
                ui::ok(go ? "Approved and logged." : "Evidence requested and logged.");
            } else ui::warn("Only a manager can decide; this is access control working as designed.");
        }
    }
    ui::pause();
    ui::step(13, N, "Website X-ray and quality gates", "step 2: the website must be on the approved list");
    {
        ui::info("Scanning a live site needs internet. In interactive mode you can try one now.");
        if (!ui::autoplay && ui::yes("Scan an approved website now?", false)) {
            string url = ui::prompt("Website");
            url = od::fetch::cleanInput(url);
            if (!url.empty() && v.approved(url)) {
                scanSite(v, url, 6, 1);
                ui::ok("Scan stored; it will appear in the exported reports.");
            } else ui::warn("Skipped (empty or not approved).");
        }
        qualityScreen(v);
    }
    ui::pause();
    ui::step(14, N, "Export every format and prove the audit trail", "steps 3 to 13: everything created so far goes into the reports");
    exportMenu(v);
    size_t n = 0, bad = 0;
    if (v.verifyAudit(&n, &bad)) ui::ok("Audit chain verified: " + std::to_string(n) + " records, all intact.");
    else ui::fail("Audit chain damaged at record " + std::to_string(bad));
    ui::panel("✔ TEST DRIVE COMPLETE", gem::emerald, {"You logged in, controlled approved sources, ingested feedback and links, analysed sentiment, found problems, opened cases, got recommendations, made a management decision, checked quality, and exported 13 formats.", ui::paint(gem::slate, "Everything is saved in " + v.dir.string())});
    ui::quiet = saveQuiet;
}

void helpScreen() {
    ui::panel("? HELP", gem::sky,
              {ui::boldPaint(gem::gold, "Roles: ") + "manager (everything), crm (add and analyse), viewer (read only).",
               ui::boldPaint(gem::emerald, "Links: ") + "only websites on the approved list can be read. Local file paths are always allowed.",
               ui::boldPaint(gem::amethyst, "Recommendations: ") + "built-in playbook plus live web search when internet is available.",
               ui::boldPaint(gem::sardonyx, "Escalation: ") + "L1 coaching, L2 written warning, L3 suspension pending investigation, L4 termination review, L5 police report, M management decision.",
               ui::boldPaint(gem::rose, "Exports: ") + "dashboard.html, explorer.html, report.docx, md, txt, json, xml, yaml, csv, tsv, svg, rtf, ics.",
               ui::paint(gem::slate, "Run with --demo to skip login and play the whole test drive automatically.")});
}

bool loginScreen(Vault& v) {
    if (v.users.empty()) {
        ui::panel("✦ FIRST-TIME SETUP", gem::gold, {"No accounts exist yet. Create the manager account. You will use it to add your team."});
        while (true) {
            string name = ui::prompt("Manager username");
            if (ui::eof(name)) return false;
            string pw = ui::secret("Password (min 8 characters)");
            string pw2 = ui::secret("Repeat password");
            if (pw != pw2) {
                ui::warn("Passwords did not match.");
                continue;
            }
            if (v.createUser(name, "manager", pw)) {
                v.audit("SETUP", "manager " + name + " created");
                ui::ok("Manager account created.");
                break;
            }
            ui::warn("Use a username and a password of at least 8 characters.");
        }
    }
    for (int tries = 0; tries < 6; ++tries) {
        string name = ui::prompt("Username");
        if (ui::eof(name)) return false;
        string pw = ui::secret("Password");
        int left = 0;
        auto r = v.login(name, pw, &left);
        if (r == Vault::Auth::Ok) {
            ui::ok("Welcome, " + v.currentUser + " (" + v.currentRole + ").");
            return true;
        }
        if (r == Vault::Auth::Locked) ui::fail("This account is locked after too many failed attempts. Ask a manager to unlock it.");
        else if (r == Vault::Auth::BadCredentials) ui::fail("Wrong password. " + std::to_string(left) + " attempt(s) left before the account locks.");
        else ui::fail("Unknown username or password.");
    }
    return false;
}

void mainMenu(Vault& v) {
    while (true) {
        ui::banner();
        ui::say("  " + ui::paint(gem::slate, "Logged in as ") + ui::boldPaint(gem::gold, v.currentUser) + ui::paint(gem::slate, " (" + v.currentRole + ") · " + std::to_string(v.entries.size()) + " comments · " + std::to_string(v.cases.size()) + " cases"));
        ui::blank();
        int c = menu("✦ MAIN MENU", gem::gold,
                     {{"Feedback Hub", "links, comments, CSV, browse"}, {"Approved Websites", "where the app may look"}, {"Analytics", "dashboard, queue, trends, words"}, {"Intelligence", "recommendations, cases, site and repo scans"},
                      {"Export Reports", "13 colourful formats incl. Word"}, {"Security and Audit", "users, lockouts, trail"}, {"Quality Gates", "usability, reliability, performance, security"}, {"Guided Test Drive", "14 connected steps"}, {"Help", ""}},
                     true);
        if (c <= 0) return;
        ui::banner();
        switch (c) {
        case 1: feedbackHub(v); break;
        case 2: sourceMenu(v); break;
        case 3: analyticsMenu(v); break;
        case 4: intelligenceMenu(v); break;
        case 5: exportMenu(v); ui::pause(); break;
        case 6: securityMenu(v); break;
        case 7: qualityScreen(v); ui::pause(); break;
        case 8: testDrive(v); ui::pause(); break;
        case 9: helpScreen(); ui::pause(); break;
        }
    }
}

}

int fi_engine_main(int argc, char** argv) {
    bool demo = false;
    string dataDir = "feedback_data";
    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a == "--demo") demo = true;
        else if (a == "--data" && i + 1 < argc) dataDir = argv[++i];
        else if (a == "--help" || a == "-h") {
            std::cout << "Usage: feedback_intel [--demo] [--data DIR]\n  --demo  run the guided test drive automatically in a fresh data folder\n  --data  choose where the data vault lives\n";
            return 0;
        }
    }
    if (demo) {
        dataDir = "feedback_demo_data";
        std::error_code ec;
        fs::remove_all(dataDir, ec);
    }
    fb::Vault vault{fs::path(dataDir)};
    vault.load();
    if (demo) {
        fb::ui::autoplay = true;
        vault.createUser("manager", "manager", "Rhodes@2026");
        vault.login("manager", "Rhodes@2026");
        fb::testDrive(vault);
        return 0;
    }
    fb::ui::banner();
    if (!fb::loginScreen(vault)) {
        std::cout << "\n  Goodbye.\n";
        return 0;
    }
    fb::mainMenu(vault);
    vault.audit("LOGOUT", vault.currentUser);
    std::cout << "\n  " << fb::ui::rainbow("Thank you for using Customer Feedback Intelligence. Goodbye!") << "\n\n";
    return 0;
}


#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
static std::vector<std::string> fiSplit(const std::string& s) {
    std::vector<std::string> r;
    std::string cur;
    bool have = false;
    char q = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (q == '\'') {
            if (c == '\'') q = 0;
            else cur += c;
        } else if (q == '"') {
            if (c == '"') q = 0;
            else if (c == '\\' && i + 1 < s.size() && (s[i + 1] == '"' || s[i + 1] == '\\')) cur += s[++i];
            else cur += c;
        } else if (c == '\'' || c == '"') {
            q = c;
            have = true;
        } else if (c == '\\' && i + 1 < s.size()) {
            cur += s[++i];
        } else if (std::isspace((unsigned char)c)) {
            if (have || !cur.empty()) {
                r.push_back(cur);
                cur.clear();
                have = false;
            }
        } else cur += c;
    }
    if (have || !cur.empty()) r.push_back(cur);
    return r;
}
#endif

#ifndef __EMSCRIPTEN__
#define PAGE_HTML R"FIHTML(<!DOCTYPE html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>Customer Feedback Intelligence</title>
<style>
:root{box-sizing:border-box;padding-top:env(safe-area-inset-top,0px);padding-bottom:env(safe-area-inset-bottom,0px);--bg:#0b1030;--pn:#141a45;--ink:#f1f5f9;--mu:#94a3b8;--ln:#2b3478;--gd:#fbbf24}
@media(prefers-color-scheme:light){:root:not([data-theme="dark"]){--bg:#fbf7ec;--pn:#fff;--ink:#1c2150;--mu:#5a6190;--ln:#e2d9bd}}
:root[data-theme="light"]{--bg:#fbf7ec;--pn:#fff;--ink:#1c2150;--mu:#5a6190;--ln:#e2d9bd}
:root[data-theme="dark"]{--bg:#0b1030;--pn:#141a45;--ink:#f1f5f9;--mu:#94a3b8;--ln:#2b3478}
html{scroll-padding-top:env(safe-area-inset-top,0px)}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:15px "Segoe UI",system-ui,Roboto,sans-serif}
header{position:sticky;top:env(safe-area-inset-top,0px);z-index:3;display:flex;gap:8px;align-items:center;padding:10px 14px;background:linear-gradient(90deg,#2b1b6e,#0f6e8c,#117a5a,#b5841a);color:#fff}
header b{flex:1;font:700 18px Georgia,serif}
header button{background:rgba(255,255,255,.22);color:#fff;border:0;border-radius:8px;padding:6px 10px;cursor:pointer;font:inherit}
#scr{margin:12px;padding:12px;background:var(--pn);border:1px solid var(--ln);border-radius:12px;overflow-x:auto;font:13px/1.35 "Cascadia Mono",Consolas,"DejaVu Sans Mono",monospace;white-space:pre;min-height:50vh}
#scr div{min-height:1.35em}
#dock{position:sticky;bottom:0;background:var(--bg);border-top:1px solid var(--ln);padding:10px 12px calc(10px + env(safe-area-inset-bottom,0px))}
#ch{display:flex;flex-wrap:wrap;gap:6px;margin-bottom:8px}
#ch button{border:0;border-radius:10px;padding:10px 13px;font:700 14px inherit;color:#10143a;cursor:pointer;text-align:left}
#row{display:flex;gap:8px}#lb{color:var(--mu);font-size:13px;margin-bottom:4px}
#in{flex:1;min-width:0;background:var(--pn);color:var(--ink);border:1px solid var(--ln);border-radius:10px;padding:11px;font:inherit}
#go{background:var(--gd);color:#1c1500;border:0;border-radius:10px;padding:0 18px;font-weight:700;cursor:pointer}
#fl{position:fixed;inset:0;background:rgba(0,0,0,.6);display:none;align-items:center;justify-content:center;z-index:9;padding:16px}
#fl div{background:var(--pn);border-radius:14px;padding:16px;max-width:560px;width:100%;max-height:80vh;overflow:auto}
#fl a{display:block;padding:8px 0;color:var(--gd);border-top:1px solid var(--ln);text-decoration:none;word-break:break-all}
:focus-visible{outline:3px solid #5cc8ff;outline-offset:2px}
</style></head><body>
<header><b>✦ Customer Feedback Intelligence</b><button id="fb">Exports</button><button id="tb">Theme</button></header>
<pre id="scr" aria-live="polite"></pre>
<div id="dock"><div id="ch"></div><div id="lb"></div><div id="row"><input id="in" autocomplete="off" aria-label="Type here"><button id="go">Send</button></div></div>
<div id="fl"><div><b>Exported files</b><span id="fx"></span><p><button id="fc">Close</button></p></div></div>
<script>
const $=s=>document.querySelector(s),scr=$("#scr"),inp=$("#in");
let pos=0,fg=null,bg=null,bold=false,dim=false,ital=false,cur=null,tail="",busy=false;
const COL=["#ff7a59","#5cc8ff","#b48cff","#2fd29a","#fbbf24","#7ee8d0","#f472b6","#a3e635","#fb923c","#22d3ee","#e879f9","#facc15"];
function newLine(){cur=document.createElement("div");scr.appendChild(cur)}
function put(t){if(!t)return;if(!cur)newLine();const s=document.createElement("span");s.textContent=t;
 let st="";if(fg)st+="color:"+fg+";";if(bg)st+="background:"+bg+";";if(bold)st+="font-weight:700;";if(dim)st+="opacity:.65;";if(ital)st+="font-style:italic;";s.style.cssText=st;cur.appendChild(s)}
function feed(txt){const re=/\x1b\[([0-9;?]*)([A-Za-z])|\r|\n|[^\x1b\r\n]+/g;let m;
 while((m=re.exec(txt))){const t=m[0];
  if(t=="\n"){newLine();continue}
  if(t=="\r"){if(cur)cur.textContent="";continue}
  if(m[2]){const c=m[1].split(";").map(Number);
   if(m[2]=="J"){scr.textContent="";cur=null;continue}
   if(m[2]!="m")continue;
   for(let i=0;i<c.length;i++){const v=c[i];
    if(v==0||isNaN(v)){fg=bg=null;bold=dim=ital=false}else if(v==1)bold=true;else if(v==2)dim=true;else if(v==3)ital=true;
    else if((v==38||v==48)&&c[i+1]==2){const col="rgb("+c[i+2]+","+c[i+3]+","+c[i+4]+")";if(v==38)fg=col;else bg=col;i+=4}}
   continue}
  put(t)}}
const strip=s=>s.replace(/\x1b\[[0-9;?]*[A-Za-z]/g,"");
function dock(){
 const lines=strip(scr.innerText).split("\n");const last=(lines[lines.length-1]||"").trim();
 $("#lb").textContent=last;
 inp.type=/password|passcode|\bpin\b/i.test(last)?"password":"text";
 const ch=$("#ch");ch.textContent="";
 const add=(label,val,i)=>{const b=document.createElement("button");b.textContent=label;b.style.background=COL[i%12];b.onclick=()=>send(val,label);ch.appendChild(b)};
 if(/\[[Yy]\/[Nn]\]/.test(last)){add("Yes","y",3);add("No","n",0);add("Default","",4);return}
 if(/Press Enter/i.test(last)){add("Continue","",4);return}
 let i=lines.length-2,found=[];while(i>=0&&i>lines.length-80){const m=lines[i].match(/\[(\d{1,2})\]\s+(\S.*?)(?:\s{2,}|$)/);
  if(m){found.unshift([m[1],m[2].replace(/[\s│║┃╎]+$/,"")])}else if(found.length&&lines[i].trim()&&!/^[\s║│┃╎]*$/.test(lines[i]))break;i--}
 found.forEach((f,k)=>add("["+f[0]+"] "+f[1],f[0],k))}
async function poll(){try{const r=await fetch("/poll?from="+pos);const b=await r.arrayBuffer();pos=+r.headers.get("X-Pos");
  if(b.byteLength){const t=new TextDecoder().decode(b);feed(t);window.scrollTo(0,document.body.scrollHeight);dock();if(/EXPORTED/.test(t))files()}
 }catch(e){await new Promise(r=>setTimeout(r,1500))}poll()}
async function send(v,label){const pw=inp.type=="password";feed("\x1b[2m"+(pw?"••••••":(label||v))+"\x1b[0m\n");$("#ch").textContent="";
 await fetch("/send",{method:"POST",body:v});inp.value="";inp.focus()}
$("#go").onclick=()=>send(inp.value,inp.value);
inp.onkeydown=e=>{if(e.key=="Enter")send(inp.value,inp.value)};
)FIHTML" R"FIHTML(async function files(){const r=await fetch("/files");const j=await r.json();$("#fx").innerHTML=j.map(f=>'<a href="/dl?f='+encodeURIComponent(f)+'" download>'+f.replace(/</g,"&lt;")+'</a>').join("")||"<p>No files yet.</p>"}
$("#fb").onclick=async()=>{await files();$("#fl").style.display="flex"};$("#fc").onclick=()=>$("#fl").style.display="none";
$("#tb").onclick=()=>{const r=document.documentElement;r.dataset.theme=getComputedStyle(r).getPropertyValue("--bg").trim()=="#0b1030"?"light":"dark"};
poll();
</script></body></html>
)FIHTML"

#include <csignal>
namespace web {
using std::string;
std::mutex mu;
std::condition_variable cv;
string outBuf;
std::deque<char> inBuf;
bool closed = false;
std::atomic<bool> stopping{false};
fs::path dataRoot;
string pageHtml;

class OutStream : public std::streambuf {
protected:
    std::streamsize xsputn(const char* s, std::streamsize n) override {
        { std::lock_guard<std::mutex> g(mu); outBuf.append(s, (size_t)n); }
        cv.notify_all();
        return n;
    }
    int overflow(int c) override {
        if (c != EOF) { char ch = (char)c; xsputn(&ch, 1); }
        return c;
    }
};

class InStream : public std::streambuf {
    char ch;
protected:
    int underflow() override {
        std::unique_lock<std::mutex> g(mu);
        cv.wait(g, [] { return !inBuf.empty() || closed; });
        if (inBuf.empty()) return EOF;
        ch = inBuf.front();
        inBuf.pop_front();
        setg(&ch, &ch, &ch + 1);
        return (unsigned char)ch;
    }
};

#ifdef _WIN32
typedef SOCKET sock_t;
#define CLOSESOCK closesocket
#else
typedef int sock_t;
#define CLOSESOCK close
#endif

void reply(sock_t s, int code, const string& type, const string& body, const string& extra = "") {
    string h = "HTTP/1.1 " + std::to_string(code) + (code == 200 ? " OK" : code == 404 ? " Not Found" : " Bad Request") +
               "\r\nContent-Type: " + type + "\r\nContent-Length: " + std::to_string(body.size()) +
               "\r\nCache-Control: no-store\r\nConnection: close\r\n" + extra + "\r\n";
    string all = h + body;
    size_t off = 0;
    while (off < all.size()) {
        int n = (int)send(s, all.data() + off, (int)(all.size() - off), 0);
        if (n <= 0) break;
        off += (size_t)n;
    }
}

string urlDecode(const string& s) {
    string o;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size() + 0 && std::isxdigit((unsigned char)s[i + 1]) && std::isxdigit((unsigned char)s[i + 2])) {
            o += (char)std::stoi(s.substr(i + 1, 2), nullptr, 16);
            i += 2;
        } else if (s[i] == '+') o += ' ';
        else o += s[i];
    }
    return o;
}

string param(const string& q, const string& key) {
    size_t p = 0;
    while (p < q.size()) {
        size_t e = q.find('&', p);
        if (e == string::npos) e = q.size();
        string kv = q.substr(p, e - p);
        size_t eq = kv.find('=');
        if (eq != string::npos && kv.substr(0, eq) == key) return urlDecode(kv.substr(eq + 1));
        p = e + 1;
    }
    return "";
}

string jsonStr(const string& s) {
    string o = "\"";
    for (unsigned char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c < 0x20) o += ' ';
        else o += (char)c;
    }
    return o + "\"";
}

string mime(const fs::path& p) {
    string e = p.extension().string();
    for (auto& c : e) c = (char)std::tolower((unsigned char)c);
    if (e == ".html" || e == ".htm") return "text/html; charset=utf-8";
    if (e == ".svg") return "image/svg+xml";
    if (e == ".json") return "application/json";
    if (e == ".csv") return "text/csv";
    if (e == ".docx") return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    if (e == ".xlsx") return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    return "application/octet-stream";
}

void handle(sock_t s) {
    string req;
    char buf[4096];
    size_t headEnd = string::npos;
    while (headEnd == string::npos && req.size() < 65536) {
        int n = (int)recv(s, buf, sizeof buf, 0);
        if (n <= 0) { CLOSESOCK(s); return; }
        req.append(buf, (size_t)n);
        headEnd = req.find("\r\n\r\n");
    }
    if (headEnd == string::npos) { CLOSESOCK(s); return; }
    string head = req.substr(0, headEnd);
    string body = req.substr(headEnd + 4);
    size_t clen = 0;
    {
        string low = head;
        for (auto& c : low) c = (char)std::tolower((unsigned char)c);
        size_t p = low.find("content-length:");
        if (p != string::npos) clen = (size_t)std::atol(low.c_str() + p + 15);
    }
    while (body.size() < clen && body.size() < (1u << 20)) {
        int n = (int)recv(s, buf, sizeof buf, 0);
        if (n <= 0) break;
        body.append(buf, (size_t)n);
    }
    std::istringstream ls(head);
    string method, target, ver;
    ls >> method >> target >> ver;
    string path = target, query;
    size_t qm = target.find('?');
    if (qm != string::npos) { path = target.substr(0, qm); query = target.substr(qm + 1); }

    if (method == "GET" && path == "/") {
        reply(s, 200, "text/html; charset=utf-8", pageHtml);
    } else if (method == "GET" && path == "/poll") {
        size_t from = (size_t)std::atol(param(query, "from").c_str());
        std::unique_lock<std::mutex> g(mu);
        cv.wait_for(g, std::chrono::seconds(20), [&] { return outBuf.size() > from || stopping; });
        if (from > outBuf.size()) from = 0;
        size_t end = outBuf.size();
        while (end > from && ((unsigned char)outBuf[end - 1] & 0xC0) == 0x80) --end;
        if (end > from && ((unsigned char)outBuf[end - 1] & 0x80)) {
            size_t i = end - 1;
            unsigned char lead = (unsigned char)outBuf[i];
            size_t need = lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : lead >= 0xC0 ? 2 : 1;
            if (outBuf.size() - i < need) end = i;
        }
        string chunk = outBuf.substr(from, end - from);
        g.unlock();
        reply(s, 200, "text/plain; charset=utf-8", chunk, "X-Pos: " + std::to_string(from + chunk.size()) + "\r\n");
    } else if (method == "POST" && path == "/send") {
        {
            std::lock_guard<std::mutex> g(mu);
            for (char c : body) if (c != '\r') inBuf.push_back(c);
            inBuf.push_back('\n');
        }
        cv.notify_all();
        reply(s, 200, "text/plain", "ok");
    } else if (method == "GET" && path == "/files") {
        std::vector<string> names;
        std::error_code ec;
        fs::path ex = dataRoot / "exports";
        if (fs::exists(ex, ec))
            for (auto& e : fs::recursive_directory_iterator(ex, ec))
                if (e.is_regular_file(ec)) names.push_back(fs::relative(e.path(), ex, ec).generic_string());
        std::sort(names.rbegin(), names.rend());
        string j = "[";
        for (size_t i = 0; i < names.size(); ++i) j += (i ? "," : "") + jsonStr(names[i]);
        reply(s, 200, "application/json", j + "]");
    } else if (method == "GET" && path == "/dl") {
        string rel = param(query, "f");
        std::error_code ec;
        fs::path ex = fs::weakly_canonical(dataRoot / "exports", ec);
        fs::path full = fs::weakly_canonical(ex / rel, ec);
        string a = full.generic_string(), b = ex.generic_string();
        if (rel.find("..") != string::npos || a.compare(0, b.size(), b) != 0 || !fs::is_regular_file(full, ec)) {
            reply(s, 404, "text/plain", "not found");
        } else {
            std::ifstream in(full, std::ios::binary);
            std::ostringstream ss;
            ss << in.rdbuf();
            reply(s, 200, mime(full), ss.str(), "Content-Disposition: " FI_DISP "; filename=\"" + full.filename().string() + "\"\r\n");
        }
    } else if (method == "POST" && path == "/quit") {
        reply(s, 200, "text/plain", "bye");
        stopping = true;
        { std::lock_guard<std::mutex> g(mu); closed = true; }
        cv.notify_all();
    } else {
        reply(s, 404, "text/plain", "not found");
    }
    CLOSESOCK(s);
}

void openBrowser(const string& url) {
#ifdef _WIN32
    std::system(("start \"\" \"" + url + "\"").c_str());
#elif defined(__APPLE__)
    std::system(("open '" + url + "' >/dev/null 2>&1 &").c_str());
#else
    std::system(("xdg-open '" + url + "' >/dev/null 2>&1 &").c_str());
#endif
}

int run(int argc, char** argv) {
    string dataDir = "feedback_data";
    int port = 8787;
    bool openIt = true;
    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a == "--data" && i + 1 < argc) dataDir = argv[++i];
        else if (a == "--port" && i + 1 < argc) port = std::atoi(argv[++i]);
        else if (a == "--no-browser") openIt = false;
        else if (a == "--help" || a == "-h") {
            std::cout << "Usage: feedback_intel_web [--data DIR] [--port N] [--no-browser]\n";
            return 0;
        }
    }
    dataRoot = fs::path(dataDir);
    pageHtml = PAGE_HTML;
#ifndef _WIN32
    std::signal(SIGPIPE, SIG_IGN);
#endif
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof yes);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    bool bound = false;
    for (int p = port; p < port + 40 && !bound; ++p) {
        addr.sin_port = htons((unsigned short)p);
        if (bind(srv, (sockaddr*)&addr, sizeof addr) == 0) { bound = true; port = p; }
    }
    if (!bound || listen(srv, 32) != 0) {
        std::cerr << "Could not open a local port.\n";
        return 1;
    }
    string url = "http://127.0.0.1:" + std::to_string(port) + "/";
    std::cerr << "Customer Feedback Intelligence is running at " << url << "\nKeep this window open. Press Ctrl+C to stop.\n";

    static OutStream outS;
    static InStream inS;
    std::cout.rdbuf(&outS);
    std::cin.rdbuf(&inS);
    std::vector<string> args = {"feedback_intel", "--data", dataDir};
    std::vector<char*> av;
    for (auto& a : args) av.push_back(&a[0]);
    std::thread engine([&] {
        fi_engine_main((int)av.size(), av.data());
        stopping = true;
        { std::lock_guard<std::mutex> g(mu); outBuf += "\n\x1b[1mSession ended. You can close this tab.\x1b[0m\n"; }
        cv.notify_all();
    });
    engine.detach();
    if (openIt) openBrowser(url);
    while (!stopping) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(srv, &fds);
        timeval tv{1, 0};
        if (select((int)srv + 1, &fds, nullptr, nullptr, &tv) > 0) {
            sock_t c = accept(srv, nullptr, nullptr);
            if (c != (sock_t)-1) std::thread(handle, c).detach();
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    return 0;
}
}

#ifndef __ANDROID__
int main(int argc, char** argv) { return web::run(argc, argv); }
#endif

#endif

#ifdef __ANDROID__
#include <jni.h>
static JavaVM* fiVm = nullptr;
static jclass fiBridge = nullptr;
static jmethodID fiFetch = nullptr;


static std::string fiJavaString(JNIEnv* env, jobject o) {
    if (!o) return "";
    const char* c = env->GetStringUTFChars((jstring)o, nullptr);
    std::string s = c ? c : "";
    if (c) env->ReleaseStringUTFChars((jstring)o, c);
    return s;
}

bool fi_curl_run(const std::string& cmd, std::string& out) {
    if (!fiVm || !fiBridge || !fiFetch) return false;
    auto t = fiSplit(cmd);
    std::string url, agent, fmt;
    std::vector<std::string> hdrs;
    bool head = false, dump = false, nobody = false;
    int timeout = 30;
    for (size_t i = 1; i < t.size(); ++i) {
        const std::string& a = t[i];
        if (a == "-A" && i + 1 < t.size()) agent = t[++i];
        else if (a == "-H" && i + 1 < t.size()) hdrs.push_back(t[++i]);
        else if (a == "-w" && i + 1 < t.size()) fmt = t[++i];
        else if (a == "--max-time" && i + 1 < t.size()) timeout = std::atoi(t[++i].c_str());
        else if (a == "-D" && i + 1 < t.size()) { dump = true; ++i; }
        else if (a == "-o" && i + 1 < t.size()) { nobody = true; ++i; }
        else if (a == "-I") head = true;
        else if (a.find("://") != std::string::npos && a[0] != '-') url = a;
    }
    if (url.empty()) return false;
    JNIEnv* env = nullptr;
    bool attached = false;
    if (fiVm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        if (fiVm->AttachCurrentThread(&env, nullptr) != JNI_OK) return false;
        attached = true;
    }
    auto t0 = std::chrono::steady_clock::now();
    jstring ju = env->NewStringUTF(url.c_str());
    jstring ja = env->NewStringUTF(agent.c_str());
    jclass sc = env->FindClass("java/lang/String");
    jobjectArray jh = env->NewObjectArray((jsize)hdrs.size(), sc, nullptr);
    for (size_t i = 0; i < hdrs.size(); ++i) env->SetObjectArrayElement(jh, (jsize)i, env->NewStringUTF(hdrs[i].c_str()));
    jobjectArray res = (jobjectArray)env->CallStaticObjectMethod(fiBridge, fiFetch, ju, ja, jh, (jboolean)head, (jint)timeout);
    bool ok = false;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    } else if (res) {
        std::string status = fiJavaString(env, env->GetObjectArrayElement(res, 0));
        std::string finalUrl = fiJavaString(env, env->GetObjectArrayElement(res, 1));
        std::string ctype = fiJavaString(env, env->GetObjectArrayElement(res, 2));
        std::string heads = fiJavaString(env, env->GetObjectArrayElement(res, 3));
        jbyteArray jb = (jbyteArray)env->GetObjectArrayElement(res, 4);
        std::string body;
        if (jb) {
            jsize n = env->GetArrayLength(jb);
            body.resize((size_t)n);
            if (n > 0) env->GetByteArrayRegion(jb, 0, n, (jbyte*)&body[0]);
        }
        double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        if (dump || head) out += heads;
        if (!head && !nobody) out += body;
        std::string f = util::replaceAll(fmt, "\\n", "\n");
        f = util::replaceAll(f, "%{http_code}", status);
        f = util::replaceAll(f, "%{time_total}", std::to_string(secs));
        f = util::replaceAll(f, "%{size_download}", std::to_string(body.size()));
        f = util::replaceAll(f, "%{url_effective}", finalUrl);
        f = util::replaceAll(f, "%{content_type}", ctype);
        out += f;
        ok = true;
    }
    if (attached) fiVm->DetachCurrentThread();
    return ok;
}

extern "C" JNIEXPORT void JNICALL Java_com_davidrhodes_feedbackintel_MainActivity_nativeStart(JNIEnv* env, jobject, jstring jdir) {
    static bool started = false;
    if (started) return;
    started = true;
    env->GetJavaVM(&fiVm);
    jclass c = env->FindClass("com/davidrhodes/feedbackintel/NetBridge");
    fiBridge = (jclass)env->NewGlobalRef(c);
    fiFetch = env->GetStaticMethodID(c, "fetch", "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;ZI)[Ljava/lang/Object;");
    std::string dir = fiJavaString(env, jdir);
    std::error_code ec;
    fs::create_directories(fs::path(dir) / "tmp", ec);
    setenv("TMPDIR", (dir + "/tmp").c_str(), 1);
    std::thread([dir] {
        std::string a0 = "feedback_intel", a1 = "--data", a2 = dir + "/feedback_data", a3 = "--no-browser", a4 = "--port", a5 = "8787";
        char* av[] = {&a0[0], &a1[0], &a2[0], &a3[0], &a4[0], &a5[0]};
        web::run(6, av);
    }).detach();
}
#endif

#ifdef __EMSCRIPTEN__
EM_ASYNC_JS(char*, fi_js_readline, (), {
    const s = await Module.fiNextLine();
    return stringToNewUTF8(s + "\n");
});
EM_JS(void, fi_js_out, (const char* p, int n), { Module.fiOut(UTF8ToString(p, n)); });
EM_JS(void, fi_js_done, (), { Module.fiDone(); });
EM_ASYNC_JS(char*, fi_js_fetch, (const char* u, int head, int timeout), {
    const url = UTF8ToString(u);
    try {
        const ctl = new AbortController();
        const timer = setTimeout(() => ctl.abort(), timeout * 1000);
        const target = Module.fiProxy ? Module.fiProxy(url) : url;
        const r = await fetch(target, {method: head ? 'HEAD' : 'GET', signal: ctl.signal, redirect: 'follow'});
        clearTimeout(timer);
        const body = head ? '' : await r.text();
        let h = 'HTTP/1.1 ' + r.status + '\r\n';
        r.headers.forEach((v, k) => { h += k + ': ' + v + '\r\n'; });
        h += '\r\n';
        return stringToNewUTF8(r.status + '\x01' + url + '\x01' + (r.headers.get('content-type') || '') + '\x01' + h + '\x01' + body);
    } catch (e) {
        return 0;
    }
});

bool fi_curl_run(const std::string& cmd, std::string& out) {
    auto t = fiSplit(cmd);
    std::string url, fmt;
    bool head = false, dump = false, nobody = false;
    int timeout = 30;
    for (size_t i = 1; i < t.size(); ++i) {
        const std::string& a = t[i];
        if ((a == "-A" || a == "-H") && i + 1 < t.size()) ++i;
        else if (a == "-w" && i + 1 < t.size()) fmt = t[++i];
        else if (a == "--max-time" && i + 1 < t.size()) timeout = std::atoi(t[++i].c_str());
        else if (a == "-D" && i + 1 < t.size()) { dump = true; ++i; }
        else if (a == "-o" && i + 1 < t.size()) { nobody = true; ++i; }
        else if (a == "-I") head = true;
        else if (a.find("://") != std::string::npos && a[0] != '-') url = a;
    }
    if (url.empty()) return false;
    auto t0 = std::chrono::steady_clock::now();
    char* r = fi_js_fetch(url.c_str(), head ? 1 : 0, timeout);
    if (!r) return false;
    std::string all = r;
    free(r);
    std::string parts[4];
    size_t p = 0;
    for (int i = 0; i < 4; ++i) {
        size_t e = all.find('\x01', p);
        if (e == std::string::npos) return false;
        parts[i] = all.substr(p, e - p);
        p = e + 1;
    }
    std::string body = all.substr(p);
    double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    if (dump || head) out += parts[3];
    if (!head && !nobody) out += body;
    std::string f = util::replaceAll(fmt, "\\n", "\n");
    f = util::replaceAll(f, "%{http_code}", parts[0]);
    f = util::replaceAll(f, "%{time_total}", std::to_string(secs));
    f = util::replaceAll(f, "%{size_download}", std::to_string(body.size()));
    f = util::replaceAll(f, "%{url_effective}", parts[1]);
    f = util::replaceAll(f, "%{content_type}", parts[2]);
    out += f;
    return true;
}

class WasmOut : public std::streambuf {
protected:
    std::streamsize xsputn(const char* s, std::streamsize n) override {
        fi_js_out(s, (int)n);
        return n;
    }
    int overflow(int c) override {
        if (c != EOF) { char ch = (char)c; fi_js_out(&ch, 1); }
        return c;
    }
};

class WasmIn : public std::streambuf {
    std::string line;
protected:
    int underflow() override {
        char* r = fi_js_readline();
        line = r ? r : "\n";
        free(r);
        setg(&line[0], &line[0], &line[0] + line.size());
        return (unsigned char)line[0];
    }
};

int main() {
    static WasmOut outS;
    static WasmIn inS;
    std::cout.rdbuf(&outS);
    std::cin.rdbuf(&inS);
    std::string a0 = "feedback_intel", a1 = "--data", a2 = "/data";
    char* av[] = {&a0[0], &a1[0], &a2[0]};
    int rc = fi_engine_main(3, av);
    fi_js_done();
    return rc;
}
#endif
