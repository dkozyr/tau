#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace tau {

inline std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path.string());
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

inline void WriteFile(const std::filesystem::path& path, std::string_view data, bool append = false) {
    std::ofstream file(path.string(), append ? std::ios_base::app : std::ios_base::out);
    file << data;
    file.close();
}

inline void CreateDirectory(const std::filesystem::path& path) {
    if(!std::filesystem::exists(path)) {
        std::filesystem::create_directory(path);
    }
}

inline void CreateDirectoryWithSubdirectories(const std::filesystem::path& path) {
    auto parent_path = path.parent_path();
    if(!std::filesystem::exists(parent_path)) {
        CreateDirectoryWithSubdirectories(parent_path);
    }
    CreateDirectory(path);
}

// inline std::filesystem::path CurrentPath() {
//     return std::filesystem::current_path();
// }

// inline std::vector<std::string> ListAllFiles(const std::filesystem::path& root_dir) {
//     std::vector<std::string> enumerated;
//     for(const auto& path : std::filesystem::recursive_directory_iterator(root_dir)) {
//         if(!std::filesystem::is_directory(path)) {
//             enumerated.push_back(path.path());
//         }
//     }
//     return enumerated;
// }

}
