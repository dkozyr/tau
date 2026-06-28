# Crypto Module

This module provides basic cryptographic utilities using OpenSSL. It includes:

* A `Certificate` class to load and manage certificates and private keys
* Support for generating self-signed certificates using a custom Certificate Authority (CA)
* A set of hash function wrappers (e.g., MD5, HMAC)
* Cryptographic random numbers

## Self-Signed Certificates with Custom CA

The `Certificate` class supports creation of self-signed certificates using a custom CA certificate and key.

To generate a CA for development:

```bash
# Generate CA private key
openssl ecparam -name prime256v1 -genkey -noout -out ca.key

# Create self-signed CA certificate
openssl req -new -x509 -sha256 -key ca.key -out ca.crt -days 3650 \
  -subj "/CN=Tau/OU=SelfSecurity"
```

You can then use `ca.key` and `ca.crt` to sign new certificates for debugging

## Example

```cpp
crypto::Certificate ca(crypto::Certificate::Options{
    .cert = "/path/to/ca.crt",
    .key  = "/path/to/ca.key"
});
crypto::Certificate cert(crypto::Certificate::OptionsSelfSigned{
    .ca = ca
});
```
