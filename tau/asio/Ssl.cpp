#include "tau/asio/Ssl.h"

namespace tau {

using ssl_context = asio_ssl::context;

void InitSslContext(SslContext& ctx, const std::vector<uint8_t>& cert, const std::vector<uint8_t>& key);

SslContext CreateSslContext(const std::vector<uint8_t>& cert, const std::vector<uint8_t>& key) {
    SslContext ctx(ssl_context::tls);
    InitSslContext(ctx, cert, key);
    return ctx;
}

SslContextPtr CreateSslContextPtr(const std::vector<uint8_t>& cert, const std::vector<uint8_t>& key) {
    auto ctx = std::make_unique<SslContext>(ssl_context::tls);
    InitSslContext(*ctx, cert, key);
    return ctx;
}

void InitSslContext(SslContext& ctx, const std::vector<uint8_t>& cert, const std::vector<uint8_t>& key) {
    ctx.set_options(ssl_context::default_workarounds | ssl_context::no_sslv2);

    ctx.set_default_verify_paths();
    ctx.set_verify_mode(asio_ssl::verify_none);

    ctx.use_certificate_chain(asio::buffer(cert.data(), cert.size()));
    ctx.use_private_key(asio::buffer(key.data(), key.size()), ssl_context::file_format::pem);
}

}
