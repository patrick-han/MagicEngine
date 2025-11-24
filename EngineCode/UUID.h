#pragma once
#include "Platform.h"
#include <string>
#include <array>
#include <cstring>     // std::memcmp, std::memcpy
#include <cctype>
#include <stdexcept>
#include <functional>  // for std::hash specialization

#if PLATFORM_WINDOWS
  #include <windows.h>
  #include <rpc.h>
  #pragma comment(lib, "rpcrt4.lib")
#elif PLATFORM_MACOS
  #include <uuid/uuid.h>
#endif

namespace Magic {

class UUID
{
public:
    UUID()
    {
    #if PLATFORM_WINDOWS
        UuidCreate(&platform_uuid);
    #elif PLATFORM_MACOS
        uuid_generate(platform_uuid);
    #endif
    }

    // ---- parsing (no-throw) ----
    static bool TryParse(const std::string& s, UUID& out) noexcept
    {
        std::string canon = NormalizeUUIDString(s);
    #if PLATFORM_WINDOWS
        RPC_STATUS status = UuidFromStringA(
            reinterpret_cast<RPC_CSTR>(const_cast<char*>(canon.c_str())),
            &out.platform_uuid);
        return status == RPC_S_OK;
    #elif PLATFORM_MACOS
        return uuid_parse(canon.c_str(), out.platform_uuid) == 0;
    #endif
    }

    // ---- string ----
    std::string ToString() const
    {
    #if PLATFORM_WINDOWS
        RPC_CSTR str = nullptr;
        UuidToStringA(&platform_uuid, &str);
        std::string result(reinterpret_cast<char*>(str));
        RpcStringFreeA(&str);
        return result;
    #elif PLATFORM_MACOS
        char str[37];
        uuid_unparse(platform_uuid, str);
        return std::string(str);
    #endif
    }

    // ---- 16-byte stable view for hashing/compare/serialize ----
    std::array<std::uint8_t, 16> ToBytes() const noexcept
    {
        std::array<std::uint8_t, 16> out {};
    #if PLATFORM_WINDOWS
        // ::UUID layout: Data1(4 bytes) | Data2(2 bytes) | Data3(2 bytes) | Data4[8 bytes]
        // https://learn.microsoft.com/en-us/windows/win32/api/guiddef/ns-guiddef-guid
        const ::UUID& u = platform_uuid;
        out[0]  = static_cast<std::uint8_t>((u.Data1 >> 24) & 0xFF);
        out[1]  = static_cast<std::uint8_t>((u.Data1 >> 16) & 0xFF);
        out[2]  = static_cast<std::uint8_t>((u.Data1 >>  8) & 0xFF);
        out[3]  = static_cast<std::uint8_t>((u.Data1      ) & 0xFF);
        out[4]  = static_cast<std::uint8_t>((u.Data2 >>  8) & 0xFF);
        out[5]  = static_cast<std::uint8_t>((u.Data2      ) & 0xFF);
        out[6]  = static_cast<std::uint8_t>((u.Data3 >>  8) & 0xFF);
        out[7]  = static_cast<std::uint8_t>((u.Data3      ) & 0xFF);
        // Data4 already bytes
        std::memcpy(&out[8],  u.Data4, 8);
    #elif PLATFORM_MACOS
        std::memcpy(out.data(), platform_uuid, 16);
    #endif
        return out;
    }

    // ---- comparisons on canonical bytes ----
    bool operator==(const UUID& other) const noexcept
    {
        auto a = ToBytes();
        auto b = other.ToBytes();
        return std::memcmp(a.data(), b.data(), 16) == 0;
    }

    bool operator<(const UUID& other) const noexcept
    {
        auto a = ToBytes();
        auto b = other.ToBytes();
        return std::memcmp(a.data(), b.data(), 16) < 0;
    }

    static std::string NormalizeUUIDString(const std::string& s) noexcept
    {
        auto is_hex = [](char c){
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return (c>='0'&&c<='9')||(c>='a'&&c<='f');
        };
        if (s.size() == 32) {
            for (char c : s) if (!is_hex(c)) return s;
            std::string t; t.reserve(36);
            for (size_t i = 0; i < 32; ++i) {
                if (i==8 || i==12 || i==16 || i==20) t.push_back('-');
                t.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(s[i]))));
            }
            return t;
        }
        if (s.size() == 36) {
            std::string t = s;
            for (char& c : t) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return t;
        }
        return s;
    }

private:
#if PLATFORM_WINDOWS
    ::UUID platform_uuid{};
#elif PLATFORM_MACOS
    uuid_t platform_uuid{};
#endif
};

} // namespace Magic

// -------------------- std::hash specialization --------------------
namespace std 
{
template<>
struct hash<Magic::UUID>
{
    size_t operator()(const Magic::UUID& id) const noexcept
    {
        // Hash the 16 canonical bytes (no padding). Use a simple, solid mixer.
        std::array<std::uint8_t, 16> bytes = id.ToBytes();
        // 64-bit splitmix (good avalanche, fast)
        constexpr unsigned long long magic = 0x9e3779b97f4a7c15ull;
        auto mix64 = [](std::uint64_t x) {
            x += magic;
            x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
            x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
            return x ^ (x >> 31);
        };
        std::uint64_t lo, hi;
        std::memcpy(&lo, bytes.data() + 0,  8);
        std::memcpy(&hi, bytes.data() + 8,  8);
        std::uint64_t h = mix64(lo) ^ mix64(hi + magic); // in the case that hi==lo or are similar, add to break symmetry
        if constexpr (sizeof(size_t) == 4) 
        {
            // fold to 32-bit when size_t is 32-bit
            h ^= (h >> 32);
        }
        return static_cast<size_t>(h);
    }
};
} // namespace std
