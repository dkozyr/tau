#pragma once

#include "tau/http/Field.h"
#include <etl/string.h>
#include <optional>
#include <cstdint>
#include <cstddef>

struct Config {
    struct Ip {
        etl::string<16> public_ip = {};
        etl::string<16> private_ip = {};
    };
    Ip ip;

    struct Wss {
        uint16_t port = 8443;
        tau::http::Fields http_fields = {};
        struct Validation {
            etl::string<64> origin_host = {};
        };
        Validation validation = {};
    };
    Wss wss = {};

    struct Ssl {
        bool self_signed = true;
        struct Ca {
            etl::string<256> certificate = PROJECT_SOURCE_DIR "/data/keys/ca.crt";
            etl::string<256> key         = PROJECT_SOURCE_DIR "/data/keys/ca.key";
        };
        Ca ca = {};
        struct Server {
            etl::string<256> certificate = PROJECT_SOURCE_DIR "/data/keys/server.crt";
            etl::string<256> key         = PROJECT_SOURCE_DIR "/data/keys/server.key";
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

std::optional<Config> ParseAndValidateConfig(const etl::string_view& config_str);
