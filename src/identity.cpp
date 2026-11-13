#include "identity.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <openssl/evp.h>
#include <sys/types.h>
#include <unistd.h>

namespace onhit {
namespace {
std::string hex(const unsigned char* data, std::size_t size) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(size * 2);
    for (std::size_t i = 0; i < size; ++i) {
        result.push_back(digits[data[i] >> 4]);
        result.push_back(digits[data[i] & 15]);
    }
    return result;
}

std::string file_sha256(std::string& error) {
    const int fd = open("/proc/self/exe", O_RDONLY | O_CLOEXEC);
    if (fd < 0) { error = std::strerror(errno); return {}; }
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) { close(fd); error = "EVP_MD_CTX_new failed"; return {}; }
    bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1;
    std::array<unsigned char, 64 * 1024> buf{};
    while (ok) {
        const ssize_t got = read(fd, buf.data(), buf.size());
        if (got < 0 && errno == EINTR) continue;
        if (got < 0) { error = std::strerror(errno); ok = false; break; }
        if (got == 0) break;
        ok = EVP_DigestUpdate(ctx, buf.data(), static_cast<std::size_t>(got)) == 1;
    }
    unsigned char digest[EVP_MAX_MD_SIZE]{};
    unsigned int len = 0;
    if (ok) ok = EVP_DigestFinal_ex(ctx, digest, &len) == 1 && len == 32;
    EVP_MD_CTX_free(ctx);
    close(fd);
    if (!ok) { if (error.empty()) error = "OpenSSL SHA-256 failed"; return {}; }
    return hex(digest, len);
}

struct MainImage {
    std::uintptr_t bias = 0;
    const ElfW(Phdr)* phdr = nullptr;
    ElfW(Half) phnum = 0;
    bool found = false;
};

int find_main(struct dl_phdr_info* info, std::size_t, void* opaque) {
    // glibc reports the main program first and with an empty name.
    if (info->dlpi_name && info->dlpi_name[0]) return 0;
    auto* image = static_cast<MainImage*>(opaque);
    image->bias = info->dlpi_addr;
    image->phdr = info->dlpi_phdr;
    image->phnum = info->dlpi_phnum;
    image->found = true;
    return 1;
}

std::string build_id(const MainImage& info) {
    for (ElfW(Half) i = 0; i < info.phnum; ++i) {
        const auto& ph = info.phdr[i];
        if (ph.p_type != PT_NOTE) continue;
        const auto* p = reinterpret_cast<const unsigned char*>(info.bias + ph.p_vaddr);
        std::size_t left = ph.p_filesz;
        while (left >= sizeof(ElfW(Nhdr))) {
            ElfW(Nhdr) note{};
            std::memcpy(&note, p, sizeof(note));
            const auto name_size = (static_cast<std::uint64_t>(note.n_namesz) + 3) & ~std::uint64_t{3};
            const auto desc_size = (static_cast<std::uint64_t>(note.n_descsz) + 3) & ~std::uint64_t{3};
            if (name_size > left - sizeof(note) || desc_size > left - sizeof(note) - name_size) break;
            const auto* name = p + sizeof(note);
            const auto* desc = name + name_size;
            if (note.n_type == NT_GNU_BUILD_ID && note.n_namesz == 4 &&
                std::memcmp(name, "GNU\0", 4) == 0) return hex(desc, note.n_descsz);
            const std::size_t consumed = sizeof(note) + name_size + desc_size;
            p += consumed;
            left -= consumed;
        }
    }
    return {};
}
}

Identity inspect_executable() {
    Identity result;
    result.sha256 = file_sha256(result.error);
    if (result.sha256.empty()) return result;
    MainImage image;
    dl_iterate_phdr(find_main, &image);
    if (!image.found) { result.error = "main ELF image not found"; return result; }
    result.load_bias = image.bias;
    result.build_id = build_id(image);
    if (result.sha256 != kExpectedSha256) {
        result.error = "executable SHA-256 does not match the investigated BDS";
        return result;
    }
    if (result.build_id != kExpectedBuildId) {
        result.error = "GNU Build ID missing or different";
        return result;
    }
    result.matched = true;
    return result;
}

bool executable_address(std::uintptr_t address, std::size_t length) {
    MainImage image;
    dl_iterate_phdr(find_main, &image);
    if (!image.found || !length) return false;
    for (ElfW(Half) i = 0; i < image.phnum; ++i) {
        const auto& ph = image.phdr[i];
        if (ph.p_type != PT_LOAD || !(ph.p_flags & PF_X)) continue;
        const std::uintptr_t start = image.bias + ph.p_vaddr;
        if (address >= start && address - start < ph.p_memsz &&
            length <= ph.p_memsz - (address - start)) return true;
    }
    return false;
}
} // namespace onhit
