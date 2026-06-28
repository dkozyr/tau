#include "apps/webrtc-echo-server/Config.h"
#include <tau/common/Json.h>
#include <tau/common/Log.h>

using namespace tau;

std::optional<Config> ParseAndValidateConfig(const etl::string_view& config_str) {
    try {
        Config config = {};

        auto config_json = Json::parse(config_str.data());

        auto config_ip = config_json.at("ip");
        if(auto private_ip = config_ip.try_at("private")) {
            config.ip.private_ip.append(json::GetStringView(*private_ip));
        }
        if(auto public_ip = config_ip.try_at("public")) {
            if(public_ip->is_string()) {
                config.ip.public_ip.append(json::GetStringView(*public_ip));
            }
        }

        auto wss = config_json.at("wss");
        if(auto port = wss.try_at("port")) {
            config.wss.port = Json::value_to<uint16_t>(*port);

            if(auto http_fields = wss.try_at("http_fields")) {
                if(auto server = http_fields->try_at("server")) {
                    if(server->is_string()) {
                        const auto value = json::GetStringView(*server);
                        config.wss.http_fields.push_back(http::Field{
                            beast_http::field::server,
                            config_str.substr(config_str.find(value), value.size())
                        });
                    }
                }
                if(auto allow_origin = http_fields->try_at("access_control_allow_origin")) {
                    if(allow_origin->is_string()) {
                        const auto value = json::GetStringView(*allow_origin);
                        config.wss.http_fields.push_back(http::Field{
                            beast_http::field::access_control_allow_origin,
                            config_str.substr(config_str.find(value), value.size())
                        });
                    }
                }
            }

            if(auto validation = wss.try_at("validation")) {
                if(auto origin_host = validation->try_at("origin_host")) {
                    if(origin_host->is_string()) {
                        config.wss.validation.origin_host.append(json::GetStringView(*origin_host));
                    }
                }
            }
        };

        auto ssl = config_json.at("ssl");
        if(auto self_signed = ssl.try_at("self_signed")) {
            config.ssl.self_signed = Json::value_to<bool>(*self_signed);
        }
        if(auto ssl_ca = ssl.try_at("ca")) {
            if(auto certificate = ssl_ca->try_at("certificate")) {
                config.ssl.ca.certificate = json::GetStringView(*certificate);
            }
            if(auto key = ssl_ca->try_at("key")) {
                config.ssl.ca.key = json::GetStringView(*key);
            }
        }
        if(auto ssl_server = ssl.try_at("server")) {
            if(auto certificate = ssl_server->try_at("certificate")) {
                config.ssl.server.certificate = json::GetStringView(*certificate);
            }
            if(auto key = ssl_server->try_at("key")) {
                config.ssl.server.key = json::GetStringView(*key);
            }
        }

        auto logging = config_json.at("logging");
        if(auto console = logging.try_at("console")) {
            config.logging.console = Json::value_to<bool>(*console);
        }
        if(auto severity = logging.try_at("severity")) {
            config.logging.severity = Json::value_to<size_t>(*severity);
        }
        return config;
    } catch(const std::exception& e) {
        TAU_LOG_ERROR("Exception: " << e.what());
    }
    return std::nullopt;
}
