#include "Assets.h"
#include "Data.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace UG {

namespace {
std::string Trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
    return s.substr(a, b - a);
}

std::vector<std::string> Tokens(const std::string& s) {
    std::istringstream in(s);
    std::vector<std::string> out;
    for (std::string t; in >> t;) out.push_back(t);
    return out;
}

bool Number(const std::string& s, float& out) {
    char* end = nullptr;
    out = std::strtof(s.c_str(), &end);
    return !s.empty() && end && *end == '\0' && std::isfinite(out);
}

bool Integer(const std::string& s, int& out) {
    float f = 0;
    if (!Number(s, f) || f != std::floor(f) || std::fabs(f) > 1e8f) return false;
    out = (int)f;
    return true;
}

bool Point(const std::string& s, GridPoint& out) {
    size_t comma = s.find(',');
    return comma != std::string::npos && Integer(s.substr(0, comma), out.col) && Integer(s.substr(comma + 1), out.row);
}

bool EnemyByKey(const std::string& key, EnemyId& out) {
    for (int i = 0; i < kEnemyCount; ++i)
        if (key == EnemyInfo((EnemyId)i).key) {
            out = (EnemyId)i;
            return true;
        }
    return false;
}

std::string Stem(const std::string& file) {
    size_t slash = file.find_last_of("/\\");
    std::string name = slash == std::string::npos ? file : file.substr(slash + 1);
    size_t dot = name.find_last_of('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

struct Loaded {
    std::vector<LevelDef> levels;
    std::vector<std::string> errors;
};

Loaded Load() {
    Loaded state;
    std::string index;
    if (!FileSystem::ReadAssetText(AssetPath("levels/levels.txt"), index)) {
        state.errors.push_back("levels/levels.txt: 无法读取关卡索引");
        return state;
    }
    std::istringstream lines(index);
    for (std::string line; std::getline(lines, line);) {
        line = Trim(line);
        if (line.rfind("\xEF\xBB\xBF", 0) == 0) line = line.substr(3);
        if (line.empty() || line[0] == '#') continue;
        std::string file = "levels/" + line, text;
        if (!FileSystem::ReadAssetText(AssetPath(file), text)) {
            state.errors.push_back(file + ": 无法读取文件");
            continue;
        }
        LevelDef def;
        if (!ParseLevel(text, file, def, state.errors)) continue;
        bool duplicate = std::any_of(state.levels.begin(), state.levels.end(), [&](const LevelDef& other) { return other.id == def.id; });
        if (duplicate) {
            state.errors.push_back(file + ": 关卡 id 重复：" + def.id);
            continue;
        }
        state.levels.push_back(std::move(def));
    }
    if (state.levels.empty() && state.errors.empty()) state.errors.push_back("levels/levels.txt: 没有列出任何关卡");
    for (const auto& e : state.errors) Diagnostics::Write(LogLevel::Warning, "Level: " + e);
    Diagnostics::Write(LogLevel::Info, "Loaded " + std::to_string(state.levels.size()) + " levels");
    return state;
}

Loaded& State() {
    static Loaded state = Load();
    return state;
}
} // namespace

const std::vector<LevelDef>& Levels() { return State().levels; }
const std::vector<std::string>& LevelErrors() { return State().errors; }

bool ParseLevel(const std::string& text, const std::string& file, LevelDef& out, std::vector<std::string>& errors) {
    LevelDef def;
    def.file = file;
    def.id = Stem(file);
    def.theme = Theme::City;
    def.gold = 300;
    def.lives = 20;
    def.hpScale = 1;
    def.seed = 0;
    def.base = {-100, -100};
    bool ok = true;
    int number = 0;
    auto fail = [&](int line, const std::string& message) {
        errors.push_back(file + ":" + std::to_string(line) + ": " + message);
        ok = false;
    };
    struct PathCheck {
        int line, path;
    };
    std::vector<PathCheck> groupPaths;
    std::istringstream in(text);
    for (std::string raw; std::getline(in, raw);) {
        ++number;
        if (number == 1 && raw.rfind("\xEF\xBB\xBF", 0) == 0) raw = raw.substr(3);
        std::string line = Trim(raw);
        if (line.empty() || line[0] == '#') continue;
        size_t split = line.find_first_of(" \t");
        std::string key = line.substr(0, split);
        std::string value = split == std::string::npos ? "" : Trim(line.substr(split));
        auto tokens = Tokens(value);
        if (key == "id" || key == "name" || key == "subtitle" || key == "desc") {
            if (value.empty()) fail(number, key + " 不能为空");
            else (key == "id" ? def.id : key == "name" ? def.name : key == "subtitle" ? def.subtitle : def.desc) = value;
        } else if (key == "theme") {
            if (value == "city") def.theme = Theme::City;
            else if (value == "mountain") def.theme = Theme::Mountain;
            else if (value == "nebula") def.theme = Theme::Nebula;
            else if (value == "park") def.theme = Theme::Park;
            else if (value == "plaza") def.theme = Theme::Plaza;
            else if (value == "studio") def.theme = Theme::Studio;
            else fail(number, "未知主题“" + value + "”，可用：city / mountain / nebula / park / plaza / studio");
        } else if (key == "gold" || key == "lives") {
            int v = 0;
            if (tokens.size() != 1 || !Integer(tokens[0], v) || v <= 0) fail(number, key + " 需要一个正整数");
            else (key == "gold" ? def.gold : def.lives) = v;
        } else if (key == "hp") {
            float v = 0;
            if (tokens.size() != 1 || !Number(tokens[0], v) || v <= 0) fail(number, "hp 需要一个正数");
            else def.hpScale = v;
        } else if (key == "seed") {
            int v = 0;
            if (tokens.size() != 1 || !Integer(tokens[0], v) || v < 0) fail(number, "seed 需要一个非负整数");
            else def.seed = (std::uint32_t)v;
        } else if (key == "base") {
            GridPoint b{};
            if (tokens.size() != 2 || !Integer(tokens[0], b.col) || !Integer(tokens[1], b.row) || b.col < 0 || b.row < 0 ||
                b.col >= kCols - 1 || b.row >= kRows)
                fail(number, "base 需要两个整数“列 行”，列 0-" + std::to_string(kCols - 2) + "，行 0-" + std::to_string(kRows - 1));
            else def.base = b;
        } else if (key == "heroes") {
            std::vector<HeroId> roster;
            for (const auto& t : tokens) {
                HeroId id{};
                if (!HeroByKey(t, id)) {
                    std::string keys;
                    for (int i = 0; i < kHeroCount; ++i) keys += std::string(i ? " " : "") + HeroInfo((HeroId)i).key;
                    fail(number, "未知英雄“" + t + "”，可用：" + keys);
                } else if (std::find(roster.begin(), roster.end(), id) != roster.end()) {
                    fail(number, "英雄“" + t + "”重复");
                } else {
                    roster.push_back(id);
                }
            }
            if (tokens.empty() || (int)tokens.size() > kRosterMax)
                fail(number, "heroes 需要 1 到 " + std::to_string(kRosterMax) + " 位英雄");
            else def.heroes = roster;
        } else if (key == "path") {
            std::vector<GridPoint> path;
            bool valid = tokens.size() >= 2;
            for (const auto& t : tokens) {
                GridPoint p{};
                if (!Point(t, p) || p.col < -1 || p.row < -1 || p.col > kCols || p.row > kRows) {
                    fail(number, "路径点“" + t + "”格式应为 列,行，且不超出地图一格");
                    valid = false;
                    break;
                }
                if (!path.empty() && path.back().col != p.col && path.back().row != p.row) {
                    fail(number, "路径段 " + tokens[path.size() - 1] + " → " + t + " 必须水平或垂直");
                    valid = false;
                    break;
                }
                path.push_back(p);
            }
            if (tokens.size() < 2) fail(number, "path 至少需要两个点");
            if (valid) def.paths.push_back(path);
        } else if (key == "wave") {
            WaveDef wave;
            std::string rest = value;
            while (true) {
                size_t bar = rest.find('|');
                auto words = Tokens(rest.substr(0, bar));
                SpawnGroup g{EnemyId::Gomora, 1, 1, 0, -1};
                if (words.empty() || !EnemyByKey(words[0], g.enemy)) {
                    fail(number, "未知怪兽“" + (words.empty() ? std::string("") : words[0]) +
                                     "”，可用：gomora baltan baltan_clone eleking redking zetton belial");
                } else {
                    for (size_t i = 1; i < words.size(); ++i) {
                        const std::string& w = words[i];
                        bool hasValue = i + 1 < words.size();
                        if (w.size() > 1 && w[0] == 'x') {
                            if (!Integer(w.substr(1), g.count) || g.count <= 0 || g.count > 500)
                                fail(number, "数量“" + w + "”应为 x1 到 x500");
                        } else if (w == "every" && hasValue) {
                            if (!Number(words[++i], g.interval) || g.interval <= 0) fail(number, "every 需要正数秒");
                        } else if (w == "after" && hasValue) {
                            if (!Number(words[++i], g.delay) || g.delay < 0) fail(number, "after 需要非负秒数");
                        } else if (w == "path" && hasValue) {
                            const std::string& p = words[++i];
                            int index = 0;
                            if (p == "all") g.path = -1;
                            else if (Integer(p, index) && index >= 1) {
                                g.path = index - 1;
                                groupPaths.push_back({number, g.path});
                            } else fail(number, "path 应为 all 或从 1 开始的路径序号");
                        } else {
                            fail(number, "无法识别“" + w + "”，每组写法：怪兽 x数量 [every 秒] [after 秒] [path 序号|all]");
                        }
                    }
                    wave.boss |= EnemyInfo(g.enemy).boss;
                    wave.groups.push_back(g);
                }
                if (bar == std::string::npos) break;
                rest = rest.substr(bar + 1);
            }
            def.waves.push_back(wave);
        } else {
            fail(number, "未知设置“" + key + "”");
        }
    }
    if (def.name.empty()) fail(number, "缺少 name");
    if (def.base.col < 0) fail(number, "缺少 base");
    if (def.paths.empty()) fail(number, "至少需要一条 path");
    if (def.waves.empty()) fail(number, "至少需要一个 wave");
    for (size_t i = 0; i < def.paths.size(); ++i) {
        const auto& end = def.paths[i].back();
        if (def.base.col >= 0 && (end.col != def.base.col || end.row != def.base.row))
            fail(number, "第 " + std::to_string(i + 1) + " 条路径的终点必须是 base 格子");
    }
    for (const auto& check : groupPaths)
        if (check.path >= (int)def.paths.size())
            fail(check.line, "path " + std::to_string(check.path + 1) + " 不存在（共 " + std::to_string(def.paths.size()) + " 条路径）");
    if (def.seed == 0)
        for (char c : def.id) def.seed = def.seed * 131 + (unsigned char)c;
    if (def.heroes.empty())
        for (int i = 0; i < kRosterMax; ++i) def.heroes.push_back((HeroId)i);
    if (ok) out = std::move(def);
    return ok;
}

} // namespace UG
