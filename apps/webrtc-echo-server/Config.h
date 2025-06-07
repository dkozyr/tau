#pragma once

#include "tau/http/Field.h"
#include <optional>
#include <cstdint>
#include <cstddef>

struct Config {
    struct Ip {
        std::string public_ip = {};
        std::string private_ip = {};
    };
    Ip ip;

    struct Wss {
        uint16_t port = 8443;
        tau::http::Fields http_fields = {};
        struct Validation {
            std::string origin_host = {};
        };
        Validation validation = {};
    };
    Wss wss = {};

    struct Ssl {
        bool self_signed = true;
        struct Ca {
            std::string certificate = std::string{PROJECT_SOURCE_DIR} + "/data/keys/ca.crt";
            std::string key = std::string{PROJECT_SOURCE_DIR} + "/data/keys/ca.key";
        };
        Ca ca = {};
        struct Server {
            std::string certificate = std::string{PROJECT_SOURCE_DIR} + "/data/keys/server.crt";
            std::string key = std::string{PROJECT_SOURCE_DIR} + "/data/keys/server.key";
        };
        Server server = {};
    };
    Ssl ssl = {};

    struct Logging {
        bool console = false;
        size_t severity = 2;
    };
    Logging logging = {};
};

std::optional<Config> ParseAndValidateConfig(const std::string& config_path);
