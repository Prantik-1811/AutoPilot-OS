#pragma once

#include <compare>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace autopilot {

class VirtualPath {
public:
    static VirtualPath parse(std::string_view value);

    const std::string& string() const noexcept;
    VirtualPath parent() const;
    bool isWithin(const VirtualPath& root) const noexcept;

    auto operator<=>(const VirtualPath&) const = default;

private:
    explicit VirtualPath(std::string value);
    std::string value_;
};

struct FileRecord {
    std::uint64_t size;
    std::string sha256;

    bool operator==(const FileRecord&) const = default;
};

class State {
public:
    explicit State(std::vector<VirtualPath> allowlisted_roots);

    bool isAllowed(const VirtualPath& path) const noexcept;
    bool directoryExists(const VirtualPath& path) const noexcept;
    bool fileExists(const VirtualPath& path) const noexcept;
    const FileRecord* file(const VirtualPath& path) const noexcept;

    void addDirectory(const VirtualPath& path);
    void addFile(const VirtualPath& path, FileRecord record);

    const std::set<VirtualPath>& allowlistedRoots() const noexcept;
    const std::set<VirtualPath>& directories() const noexcept;
    const std::map<VirtualPath, FileRecord>& files() const noexcept;

private:
    friend class SimulatedEnvironment;

    std::set<VirtualPath> allowlisted_roots_;
    std::set<VirtualPath> directories_;
    std::map<VirtualPath, FileRecord> files_;
};

} // namespace autopilot
