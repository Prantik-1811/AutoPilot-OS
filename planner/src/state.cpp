#include "state.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace autopilot {

VirtualPath::VirtualPath(std::string value) : value_(std::move(value)) {}

VirtualPath VirtualPath::parse(std::string_view value) {
    if (value.empty() || value.front() != '/' || value.find('\\') != std::string_view::npos) {
        throw std::invalid_argument("Virtual paths must be absolute and use '/' separators.");
    }
    if (value == "/") {
        return VirtualPath("/");
    }
    if (value.back() == '/') {
        throw std::invalid_argument("Virtual paths must not have a trailing separator.");
    }

    std::size_t segment_start = 1;
    while (segment_start < value.size()) {
        const auto segment_end = value.find('/', segment_start);
        const auto end = segment_end == std::string_view::npos ? value.size() : segment_end;
        const auto segment = value.substr(segment_start, end - segment_start);
        if (segment.empty() || segment == "." || segment == "..") {
            throw std::invalid_argument("Virtual paths must not contain empty, '.' or '..' segments.");
        }
        for (const unsigned char character : segment) {
            if (character < 0x20 || character == 0x7f) {
                throw std::invalid_argument("Virtual paths must not contain control characters.");
            }
        }
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_start = segment_end + 1;
    }

    return VirtualPath(std::string(value));
}

const std::string& VirtualPath::string() const noexcept {
    return value_;
}

VirtualPath VirtualPath::parent() const {
    if (value_ == "/") {
        return *this;
    }
    const auto separator = value_.find_last_of('/');
    if (separator == 0) {
        return VirtualPath("/");
    }
    return VirtualPath(value_.substr(0, separator));
}

bool VirtualPath::isWithin(const VirtualPath& root) const noexcept {
    if (root.value_ == "/") {
        return !value_.empty() && value_.front() == '/';
    }
    return value_ == root.value_ ||
           (value_.size() > root.value_.size() &&
            value_.compare(0, root.value_.size(), root.value_) == 0 &&
            value_[root.value_.size()] == '/');
}

State::State(std::vector<VirtualPath> allowlisted_roots) {
    if (allowlisted_roots.empty()) {
        throw std::invalid_argument("At least one allow-listed root is required.");
    }

    directories_.insert(VirtualPath::parse("/"));
    for (const auto& root : allowlisted_roots) {
        if (root.string() == "/") {
            throw std::invalid_argument("The filesystem root cannot be allow-listed.");
        }
        allowlisted_roots_.insert(root);
        auto directory = root;
        while (directory.string() != "/") {
            directories_.insert(directory);
            directory = directory.parent();
        }
    }
}

bool State::isAllowed(const VirtualPath& path) const noexcept {
    return std::any_of(allowlisted_roots_.begin(), allowlisted_roots_.end(),
        [&path](const auto& root) { return path.isWithin(root); });
}

bool State::directoryExists(const VirtualPath& path) const noexcept {
    return directories_.contains(path);
}

bool State::fileExists(const VirtualPath& path) const noexcept {
    return files_.contains(path);
}

const FileRecord* State::file(const VirtualPath& path) const noexcept {
    const auto found = files_.find(path);
    return found == files_.end() ? nullptr : &found->second;
}

void State::addDirectory(const VirtualPath& path) {
    if (!isAllowed(path)) {
        throw std::invalid_argument("Directory is outside all allow-listed roots.");
    }
    if (directoryExists(path) || fileExists(path)) {
        throw std::invalid_argument("Directory path is already in use.");
    }
    if (!directoryExists(path.parent())) {
        throw std::invalid_argument("Parent directory does not exist.");
    }
    directories_.insert(path);
}

void State::addFile(const VirtualPath& path, FileRecord record) {
    if (!isAllowed(path)) {
        throw std::invalid_argument("File is outside all allow-listed roots.");
    }
    if (directoryExists(path) || fileExists(path)) {
        throw std::invalid_argument("File path is already in use.");
    }
    if (!directoryExists(path.parent())) {
        throw std::invalid_argument("Parent directory does not exist.");
    }
    if (record.sha256.size() != 64 ||
        !std::all_of(record.sha256.begin(), record.sha256.end(), [](unsigned char character) {
            return (character >= '0' && character <= '9') ||
                   (character >= 'a' && character <= 'f') ||
                   (character >= 'A' && character <= 'F');
        })) {
        throw std::invalid_argument("File record must contain a 64-character SHA-256 hex digest.");
    }
    std::transform(record.sha256.begin(), record.sha256.end(), record.sha256.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    files_.emplace(path, std::move(record));
}

const std::set<VirtualPath>& State::allowlistedRoots() const noexcept {
    return allowlisted_roots_;
}

const std::set<VirtualPath>& State::directories() const noexcept {
    return directories_;
}

const std::map<VirtualPath, FileRecord>& State::files() const noexcept {
    return files_;
}

} // namespace autopilot
