#ifndef SRC_HPP
#define SRC_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <unordered_map>
#include <cstring>

// Basic exception hierarchy
class BasicException {
protected:
    std::string message;
public:
    explicit BasicException(const std::string &_message) : message(_message) {}
    virtual const char *what() const { return message.c_str(); }
    virtual ~BasicException() = default;
};

class ArgumentException : public BasicException {
public:
    explicit ArgumentException(const std::string &_msg) : BasicException(_msg) {}
};

class IteratorException : public BasicException {
public:
    explicit IteratorException(const char *_msg) : BasicException(_msg) {}
};

// Pokemon structure – name must be a char array of size 12 as required by the judge
struct Pokemon {
    char name[12];
    int id;
    std::vector<std::string> types; // internal representation
    Pokemon() : id(0) { name[0] = '\0'; }
    Pokemon(const std::string &n, int i, const std::vector<std::string> &t) : id(i), types(t) {
        std::fill(std::begin(name), std::end(name), '\0');
        std::strncpy(name, n.c_str(), sizeof(name) - 1);
    }
};

class Pokedex {
private:
    std::vector<Pokemon> data; // always kept sorted by id (ascending)
    std::string fileName;
    const std::unordered_set<std::string> validTypes = {
        "water", "fire", "grass", "electric", "ground", "rock", "flying"
    };
    // type effectiveness table: attack -> defender -> multiplier
    const std::unordered_map<std::string, std::unordered_map<std::string, double>> effect = {
        // attacker -> defender -> multiplier
        {"water", {{"fire",2.0},{"ground",2.0},{"rock",2.0},{"water",0.5},{"grass",0.5},{"electric",0.5},{"flying",1.0}}},
        {"fire", {{"grass",2.0},{"rock",0.5},{"water",0.5},{"fire",0.5},{"electric",0.5},{"ground",1.0},{"flying",1.0}}},
        {"grass", {{"water",2.0},{"ground",2.0},{"rock",2.0},{"fire",0.5},{"grass",0.5},{"electric",0.5},{"flying",1.0}}},
        {"electric", {{"water",2.0},{"flying",2.0},{"ground",0.0},{"grass",0.5},{"electric",0.5},{"rock",1.0},{"fire",1.0}}},
        {"ground", {{"electric",2.0},{"fire",2.0},{"rock",2.0},{"grass",0.5},{"ground",0.5},{"flying",0.0},{"water",1.0}}},
        {"rock", {{"fire",2.0},{"flying",2.0},{"ground",0.5},{"rock",0.5},{"grass",0.5},{"water",1.0},{"electric",1.0}}},
        {"flying", {{"grass",2.0},{"electric",0.5},{"rock",0.0},{"flying",0.5},{"water",1.0},{"fire",1.0},{"ground",1.0}}}
    };

    // helper to split a string by '#'
    static std::vector<std::string> splitTypes(const std::string &s) {
        std::vector<std::string> res;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, '#')) {
            if (!item.empty()) res.push_back(item);
        }
        return res;
    }

    // load from file (if exists)
    void load() {
        std::ifstream fin(fileName);
        if (!fin) return; // file may not exist yet
        std::string line;
        while (std::getline(fin, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string name; int id; std::string typesStr;
            ss >> name >> id >> typesStr;
            auto types = splitTypes(typesStr);
            data.emplace_back(name, id, types);
        }
        // ensure sorted by id
        std::sort(data.begin(), data.end(), [](const Pokemon &a, const Pokemon &b){ return a.id < b.id; });
    }

    // save to file (overwrite)
    void save() const {
        std::ofstream fout(fileName, std::ios::trunc);
        for (const auto &p : data) {
            fout << p.name << ' ' << p.id << ' ';
            for (size_t i = 0; i < p.types.size(); ++i) {
                fout << p.types[i];
                if (i + 1 < p.types.size()) fout << '#';
            }
            fout << '\n';
        }
    }

    // find index by id, -1 if not found
    int findIndex(int id) const {
        auto it = std::lower_bound(data.begin(), data.end(), id,
            [](const Pokemon &p, int value){ return p.id < value; });
        if (it != data.end() && it->id == id) return static_cast<int>(it - data.begin());
        return -1;
    }

public:
    explicit Pokedex(const char *_fileName) : fileName(_fileName) {
        load();
    }
    ~Pokedex() {
        save();
    }

    bool pokeAdd(const char *name_c, int id, const char *types_c) {
        // validation
        std::string name(name_c);
        if (name.empty() || name.size() > 10) {
            throw ArgumentException((std::string("Argument Error: PM Name Invalid (") + name + ")").c_str());
        }
        // name must contain only letters
        for (char ch : name) {
            if (!std::isalpha(static_cast<unsigned char>(ch))) {
                throw ArgumentException((std::string("Argument Error: PM Name Invalid (") + name + ")").c_str());
            }
        }
        if (id <= 0) {
            throw ArgumentException((std::string("Argument Error: PM ID Invalid (") + std::to_string(id) + ")").c_str());
        }
        std::string typesStr(types_c);
        auto types = splitTypes(typesStr);
        if (types.empty() || types.size() > 7) {
            throw ArgumentException("Argument Error: PM Type Invalid (count)");
        }
        for (const auto &t : types) {
            if (validTypes.find(t) == validTypes.end()) {
                throw ArgumentException((std::string("Argument Error: PM Type Invalid (") + t + ")").c_str());
            }
        }
        // uniqueness checks
        if (std::any_of(data.begin(), data.end(), [&](const Pokemon &p){ return std::string(p.name) == name; }))
            return false;
        if (findIndex(id) != -1)
            return false;
        // insert keeping order
        Pokemon p(name, id, types);
        data.insert(std::upper_bound(data.begin(), data.end(), p.id,
            [](int value, const Pokemon &elem){ return value < elem.id; }), p);
        return true;
    }

    bool pokeDel(int id) {
        int idx = findIndex(id);
        if (idx == -1) return false;
        data.erase(data.begin() + idx);
        return true;
    }

    std::string pokeFind(int id) const {
        int idx = findIndex(id);
        if (idx == -1) return "None";
        return std::string(data[idx].name);
    }

    std::string typeFind(const char *types_c) const {
        std::string queryStr(types_c);
        auto queryTypes = splitTypes(queryStr);
        if (queryTypes.empty()) return "None";
        // validate each query type
        for (const auto &qt : queryTypes) {
            if (validTypes.find(qt) == validTypes.end()) {
                throw ArgumentException((std::string("Argument Error: PM Type Invalid (") + qt + ")").c_str());
            }
        }
        std::vector<const Pokemon*> matches;
        for (const auto &p : data) {
            bool ok = true;
            for (const auto &qt : queryTypes) {
                if (std::find(p.types.begin(), p.types.end(), qt) == p.types.end()) { ok = false; break; }
            }
            if (ok) matches.push_back(&p);
        }
        if (matches.empty()) return "None";
        // already sorted by id because data is sorted
        std::ostringstream out;
        out << matches.size() << '\n';
        for (size_t i = 0; i < matches.size(); ++i) {
            out << matches[i]->name;
            if (i + 1 < matches.size()) out << '\n';
        }
        return out.str();
    }

    float attack(const char *type_c, int id) const {
        int idx = findIndex(id);
        if (idx == -1) return -1.0f;
        std::string atkType(type_c);
        // If attacker type is unknown, treat as neutral (1.0) multiplier
        if (validTypes.find(atkType) == validTypes.end()) {
            // No exception, assume neutral damage
        }
        const auto &defTypes = data[idx].types;
        double mult = 1.0;
        for (const auto &def : defTypes) {
            double cur = 1.0;
            auto itAtk = effect.find(atkType);
            if (itAtk != effect.end()) {
                auto itDef = itAtk->second.find(def);
                if (itDef != itAtk->second.end()) cur = itDef->second;
            }
            mult *= cur;
        }
        return static_cast<float>(mult);
    }

    int catchTry() const {
        if (data.empty()) return 0;
        // start with smallest id pokemon
        std::unordered_set<int> ownedIds;
        ownedIds.insert(data.front().id);
        bool progress = true;
        while (progress) {
            progress = false;
            for (const auto &candidate : data) {
                if (ownedIds.count(candidate.id)) continue;
                // check if any owned pokemon can deal >=2x damage
                bool canCapture = false;
                for (int ownedId : ownedIds) {
                    const Pokemon *owner = nullptr;
                    int oidx = findIndex(ownedId);
                    if (oidx != -1) owner = &data[oidx];
                    if (!owner) continue;
                    for (const auto &ownerType : owner->types) {
                        double mult = 1.0;
                        for (const auto &def : candidate.types) {
                            double cur = 1.0;
                            auto itAtk = effect.find(ownerType);
                            if (itAtk != effect.end()) {
                                auto itDef = itAtk->second.find(def);
                                if (itDef != itAtk->second.end()) cur = itDef->second;
                            }
                            mult *= cur;
                        }
                        if (mult >= 2.0 - 1e-9) { canCapture = true; break; }
                    }
                    if (canCapture) break;
                }
                if (canCapture) {
                    ownedIds.insert(candidate.id);
                    progress = true;
                }
            }
        }
        return static_cast<int>(ownedIds.size());
    }

    // iterator implementation
    class iterator {
    private:
        const Pokedex *parent;
        long long pos; // index in data vector
    public:
        iterator() : parent(nullptr), pos(0) {}
        iterator(const Pokedex *p, long long i) : parent(p), pos(i) {}
        // ++ moves to next element (larger id)
        iterator &operator++() {
            if (!parent) throw IteratorException("Iterator not initialized");
            if (pos >= static_cast<long long>(parent->data.size())) throw IteratorException("Iterator increment out of range");
            ++pos;
            return *this;
        }
        iterator operator++(int) {
            iterator tmp = *this; ++(*this); return tmp;
        }
        // -- moves to previous element (smaller id)
        iterator &operator--() {
            if (!parent) throw IteratorException("Iterator not initialized");
            if (pos <= 0) throw IteratorException("Iterator decrement out of range");
            --pos;
            return *this;
        }
        iterator operator--(int) {
            iterator tmp = *this; --(*this); return tmp;
        }
        iterator &operator=(const iterator &rhs) {
            parent = rhs.parent; pos = rhs.pos; return *this;
        }
        bool operator==(const iterator &rhs) const { return parent == rhs.parent && pos == rhs.pos; }
        bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
        Pokemon &operator*() const {
            if (!parent) throw IteratorException("Dereference null iterator");
            if (pos < 0 || pos >= static_cast<long long>(parent->data.size())) throw IteratorException("Dereference out of range");
            return const_cast<Pokemon&>(parent->data[pos]);
        }
        Pokemon *operator->() const {
            if (!parent) throw IteratorException("Arrow null iterator");
            if (pos < 0 || pos >= static_cast<long long>(parent->data.size())) throw IteratorException("Arrow out of range");
            return const_cast<Pokemon*>(&parent->data[pos]);
        }
    };

    // begin points to element with smallest id (first element)
    iterator begin() { return iterator(this, 0); }
    // end points past the last element
    iterator end() { return iterator(this, static_cast<long long>(data.size())); }
};

#endif // SRC_HPP
