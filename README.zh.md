# openssl-m

> 以 C++23 模块使用 OpenSSL 3.5 LTS 的 libcrypto 与 libssl。

[English](README.md) · **简体中文** · [Upstream](https://github.com/openssl/openssl) · [Releases](https://github.com/mcpplibs/openssl-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [Index](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![mcpp-community](https://img.shields.io/badge/mcpp--community-mwrap-blue)](https://github.com/mcpp-community)
[![CI](https://github.com/mcpplibs/openssl-m/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/mcpplibs/openssl-m/actions/workflows/ci.yml)

上游 C API 不变：`SSL_CTX`、`EVP_DigestInit_ex`、`X509_verify_cert` 就是 `<openssl/...>`
声明的上游全局实体。库由 `compat.openssl` 3.5.9 提供，用 OpenSSL 自己的 Configure 从发布包构建
（Linux、macOS 带汇编；Windows 为 `no-asm`）。模块接口由编译器从配置后的头文件推导：全部声明、
头文件中 `static ossl_inline` 函数的同名重新声明，以及每个宏的映射或保留原因。

## Modules

- `import mcpplibs.mwrap.openssl;`：下列全部
- `import mcpplibs.mwrap.openssl.crypto;`：libcrypto 头文件 —— EVP、BIO、ERR、RAND、X509、PEM、BN、EC、KDF/MAC、OSSL_PARAM、provider
- `import mcpplibs.mwrap.openssl.ssl;`：libssl 头文件 —— `SSL_CTX`、`SSL`、TLS、DTLS、QUIC
- `import mcpplibs.mwrap.openssl.macros;`：`SSL_OP_*`、`TLS1_3_VERSION`、`EVP_MAX_MD_SIZE`、`SSL_CTX_set_min_proto_version`、`BIO_get_mem_data` 等，位于 `mcpplibs::mwrap::openssl::macros`

> More Modules API -> [用法与 API](docs/usage.md)

## 快速开始

```sh
mcpp new hashes --template mcpplibs.mwrap.openssl@3.5.9
cd hashes
mcpp run
```

默认模板用公开测试向量核对 SHA-256 与 HMAC-SHA-256，并用 AES-256-GCM 加密一条消息。
`--template mcpplibs.mwrap.openssl@3.5.9:tls-loopback` 在运行时生成密钥和自签证书，
让 TLS 1.3 客户端与服务端经一对内存 BIO 通信 —— 不用网络，不读文件。

```toml
[dependencies]
mcpplibs.mwrap.openssl = "3.5.9"
```

```cpp
import std;
import mcpplibs.mwrap.openssl;
namespace m = mcpplibs::mwrap::openssl::macros;

std::string sha256(std::string_view text) {
    unsigned char digest[m::EVP_MAX_MD_SIZE];
    unsigned int size = 0;
    EVP_Digest(text.data(), text.size(), digest, &size, EVP_sha256(), nullptr);
    std::string hex;
    for (unsigned i = 0; i < size; ++i) hex += std::format("{:02x}", digest[i]);
    return hex;
}
```

## 宏与内联函数

模块不能导出宏。常量（11632 个）是展开类型的 `inline constexpr`；展开为一次函数调用的宏
（510 个，如 `SSL_CTX_set_min_proto_version`、`SSL_set_tlsext_host_name`、`BIO_get_mem_data`）
是同名函数，参数类型取被调函数的参数类型。头文件中 `static ossl_inline` 的函数（`ERR_GET_LIB`、
`ERR_GET_REASON`、`OPENSSL_load_u32_le`、`lh_*` 等 334 个）在 `mcpplibs::mwrap::openssl` 中重新声明。
其余保持为宏，由 `<mwrap/openssl/macros.hpp>` 提供：类型泛型的 `sk_TYPE_*` / `lh_*` / `STACK_OF`、
OID 弧列表 `OBJ_*`、记录 `__FILE__` 的 `OPENSSL_malloc` / `OPENSSL_free` 等，以及 `OPENSSL_NO_*`
开关。该头文件放在 import 之前（[全部宏及分类](docs/macros.md)）。

## 平台

Linux x86_64（GCC、LLVM）与 aarch64（LLVM）、macOS（arm64）、Windows（x86_64，定义
`NOMINMAX` 与 `WIN32_LEAN_AND_MEAN`）。compat.openssl 在安装时用 OpenSSL 的 Configure
按宿主 glibc 构建，因此没有交叉目标，也不支持 musl 图：不支持 wasm32，也不支持 Linux aarch64
上 mcpp 的 GCC（它是 musl 工具链）。

## 构建与测试

CI 在每个平台与编译器（GCC 16、LLVM 23）上构建模块并运行测试：摘要、MAC 与 AES-GCM
的公开向量，错误队列，自签证书校验，经内存 BIO 的 TLS 1.2 与 1.3 握手，每个常量与其宏的比较，
以及头文件混用。导出审计（[tools/check_exports.py](tools/check_exports.py)）检查生成的源码是否最新，
并在 Linux LLVM 行上从头文件重新推导记录。另一个作业把两个模板作为已安装包生成并运行。

## 许可

封装层为 [Apache-2.0](LICENSE)，归属 mcpp-community（[NOTICE](NOTICE)）。OpenSSL 保持
Apache License 2.0（[upstream/openssl/LICENSE.txt](upstream/openssl/LICENSE.txt)、
[README](upstream/openssl/README.md)）。
