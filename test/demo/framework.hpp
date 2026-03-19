#pragma once

#define NOMINMAX

#include <windows.h>
#include <intrin.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "cpueaxh.hpp"
#include "cpu/def.h"
#include "memory/manager.hpp"

void init_cpu_context(CPU_CONTEXT* ctx, MEMORY_MANAGER* mem_mgr);
int cpu_step(CPU_CONTEXT* ctx);

constexpr int kCpuStepOk = 0;
constexpr int kCpuStepException = 4;

extern "C" int cpueaxh_test_run_native(cpueaxh_x86_context* context, void* code, void* bridge_block);

namespace cpueaxh_test {

constexpr std::uint64_t kFlagCF = 1ull << 0;
constexpr std::uint64_t kFlagPF = 1ull << 2;
constexpr std::uint64_t kFlagAF = 1ull << 4;
constexpr std::uint64_t kFlagZF = 1ull << 6;
constexpr std::uint64_t kFlagSF = 1ull << 7;
constexpr std::uint64_t kFlagDF = 1ull << 10;
constexpr std::uint64_t kFlagOF = 1ull << 11;
constexpr std::uint64_t kStatusMask = kFlagCF | kFlagPF | kFlagAF | kFlagZF | kFlagSF | kFlagOF;
constexpr std::uint64_t kLogicMask = kFlagCF | kFlagPF | kFlagZF | kFlagSF | kFlagOF;
constexpr std::uint64_t kIncDecMask = kFlagPF | kFlagAF | kFlagZF | kFlagSF | kFlagOF;
constexpr std::uint64_t kRotationMask = kFlagCF | kFlagOF;
constexpr std::uint64_t kBitTestMask = kFlagCF;
constexpr std::uint64_t kBitScanMask = kFlagZF;
constexpr std::uint64_t kSeedCount = 128;
constexpr std::uint64_t kExceptionSeedCount = 128;
constexpr std::uint64_t kGuestCodeBase = 0x100000;
constexpr std::uint64_t kGuestStackBase = 0x200000;
constexpr std::size_t kCodePageSize = 0x1000;
constexpr std::size_t kStackSize = 0x4000;
constexpr std::size_t kSlotSize = 16;
constexpr std::size_t kBufferSize = 64;
constexpr std::size_t kDataSize = kSlotSize * 4 + kBufferSize * 2;
constexpr std::uint64_t kInitialMxcsr = 0x1F80;
constexpr std::uint64_t kInitialRspOffset = kStackSize - 0x200;

struct TestBridgeBlock {
    std::uint64_t host_rsp;
    std::uint64_t context_ptr;
    std::uint64_t guest_rsp;
    std::uint64_t saved_stack0;
    std::uint64_t saved_stack1;
    std::uint64_t saved_stack2;
    std::uint64_t guest_rax;
    std::uint64_t guest_r10;
};

inline std::uint64_t mix64(std::uint64_t value) {
    value += 0x9E3779B97F4A7C15ull;
    value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ull;
    value = (value ^ (value >> 27)) * 0x94D049BB133111EBull;
    return value ^ (value >> 31);
}

inline std::uint32_t narrow32(std::uint64_t value) {
    return static_cast<std::uint32_t>(value & 0xFFFFFFFFu);
}

inline std::uint8_t narrow8(std::uint64_t value) {
    return static_cast<std::uint8_t>(value & 0xFFu);
}

inline std::uint64_t seeded(std::uint64_t seed, std::uint64_t salt) {
    return mix64(seed + salt * 0x9E3779B97F4A7C15ull + 0xD1B54A32D192ED03ull);
}

inline std::string hex64(std::uint64_t value) {
    std::ostringstream stream;
    stream << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
    return stream.str();
}

inline std::string hex8(std::uint8_t value) {
    std::ostringstream stream;
    stream << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(value);
    return stream.str();
}

enum class Reg : std::uint8_t {
    RAX = 0,
    RCX = 1,
    RDX = 2,
    RBX = 3,
    RSP = 4,
    RBP = 5,
    RSI = 6,
    RDI = 7,
    R8 = 8,
    R9 = 9,
    R10 = 10,
    R11 = 11,
    R12 = 12,
    R13 = 13,
    R14 = 14,
    R15 = 15,
};

enum class Family : std::uint8_t {
    BinaryRegReg,
    BinaryRegImm,
    UnaryReg,
    ShiftImm,
    ShiftCl,
    BitOps,
    FlagOps,
    MoveOps,
    MemoryOps,
    StackOps,
    StringOps,
    VectorOps,
    ControlFlow,
};

enum class BinaryOp : std::uint8_t {
    Add,
    Adc,
    Sub,
    Sbb,
    And,
    Or,
    Xor,
    Cmp,
    Test,
};

enum class UnaryOp : std::uint8_t {
    Inc,
    Dec,
    Neg,
    Not,
};

enum class ShiftOp : std::uint8_t {
    Rol,
    Ror,
    Shl,
    Shr,
    Sar,
};

enum class BitOp : std::uint8_t {
    BtImm,
    BtReg,
    Bsf,
    Bswap,
    BsfAlt,
    BswapAlt,
};

enum class FlagProgram : std::uint8_t {
    Setcc,
    Cmovcc,
    Jcc,
};

enum class MoveProgram : std::uint8_t {
    MovA,
    MovB,
    MovzxByte,
    MovzxWord,
    MovsxByte,
    MovsxWord,
    Movsxd,
    Lea2,
    Lea4,
};

enum class MemoryProgram : std::uint8_t {
    MovRoundtrip,
    AddMem,
    SubMem,
    AndMem,
    OrMem,
    XorMem,
    XaddMem,
    CmpxchgMem,
};

enum class StackProgram : std::uint8_t {
    PushPopPair,
    PushPopFlags,
    EnterLeave,
    PushChain,
};

enum class StringProgram : std::uint8_t {
    Movsb,
    Movsw,
    Movsd,
    Movsq,
    RepMovsb,
    RepMovsw,
    Lodsb,
    Lodsq,
    Stosb,
    Stosq,
    Cmpsb,
    Scasb,
};

enum class VectorProgram : std::uint8_t {
    Paddq,
    Pxor,
    Pand,
    Por,
    Pcmpeqb,
    Pshufd,
    Pslldq,
    Psrldq,
    PaddqPxor,
    Movdqu,
    Pshufb,
    Aesenc,
    Aesenclast,
    Aeskeygenassist,
    Roundsd,
    Roundss,
};

enum class ControlProgram : std::uint8_t {
    JmpSkip,
    CallAdd,
    CallXor,
    CallLea,
};

struct PairSpec {
    Reg dst;
    Reg src;
    const char* name;
};

struct RegSpec {
    Reg reg;
    const char* name;
};

struct CondSpec {
    std::uint8_t code;
    const char* name;
};

struct ProgramSpec {
    Family family;
    std::uint32_t op;
    std::uint32_t variant;
    std::uint64_t flag_mask;
    std::string name;
};

struct BuiltCase {
    ProgramSpec spec;
    std::uint64_t seed;
    std::vector<std::uint8_t> image;
    std::uint32_t data_offset;
    cpueaxh_x86_context initial_context;
};

struct Failure {
    std::string case_name;
    std::string detail;
};

struct HostFeatures {
    bool ssse3 = false;
    bool sse41 = false;
    bool aes = false;
    bool rdpid = false;
};

inline constexpr std::array<std::uint8_t, 4> kAesKeygenAssistImmediates = {{ 0x01, 0x1B, 0x36, 0x80 }};
inline constexpr std::array<std::uint8_t, 6> kRoundImmediates = {{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x08 }};

inline constexpr std::array<PairSpec, 3> kBinaryPairs = {{
    { Reg::RAX, Reg::RBX, "rax_rbx" },
    { Reg::R8, Reg::R9, "r8_r9" },
    { Reg::R10, Reg::R11, "r10_r11" },
}};

inline constexpr std::array<RegSpec, 3> kCoreRegs = {{
    { Reg::RAX, "rax" },
    { Reg::R8, "r8" },
    { Reg::R10, "r10" },
}};

inline constexpr std::array<RegSpec, 4> kUnaryRegs = {{
    { Reg::RAX, "rax" },
    { Reg::R8, "r8" },
    { Reg::R10, "r10" },
    { Reg::R13, "r13" },
}};

inline constexpr std::array<CondSpec, 7> kConditions = {{
    { 0x0, "o" },
    { 0x2, "b" },
    { 0x4, "z" },
    { 0x8, "s" },
    { 0xA, "p" },
    { 0xC, "l" },
    { 0xE, "le" },
}};

inline const char* reg_name(Reg reg) {
    static constexpr const char* names[] = {
        "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };
    return names[static_cast<std::size_t>(reg)];
}

inline const char* binary_name(BinaryOp op) {
    switch (op) {
    case BinaryOp::Add: return "add";
    case BinaryOp::Adc: return "adc";
    case BinaryOp::Sub: return "sub";
    case BinaryOp::Sbb: return "sbb";
    case BinaryOp::And: return "and";
    case BinaryOp::Or: return "or";
    case BinaryOp::Xor: return "xor";
    case BinaryOp::Cmp: return "cmp";
    default: return "test";
    }
}

inline const char* unary_name(UnaryOp op) {
    switch (op) {
    case UnaryOp::Inc: return "inc";
    case UnaryOp::Dec: return "dec";
    case UnaryOp::Neg: return "neg";
    default: return "not";
    }
}

inline const char* shift_name(ShiftOp op) {
    switch (op) {
    case ShiftOp::Rol: return "rol";
    case ShiftOp::Ror: return "ror";
    case ShiftOp::Shl: return "shl";
    case ShiftOp::Shr: return "shr";
    default: return "sar";
    }
}

inline std::uint64_t binary_flag_mask(BinaryOp op) {
    switch (op) {
    case BinaryOp::And:
    case BinaryOp::Or:
    case BinaryOp::Xor:
    case BinaryOp::Test:
        return kLogicMask;
    default:
        return kStatusMask;
    }
}

inline std::uint64_t unary_flag_mask(UnaryOp op) {
    switch (op) {
    case UnaryOp::Inc:
    case UnaryOp::Dec:
        return kIncDecMask;
    case UnaryOp::Neg:
        return kStatusMask;
    default:
        return 0;
    }
}

inline std::uint64_t shift_flag_mask(ShiftOp op) {
    switch (op) {
    case ShiftOp::Rol:
    case ShiftOp::Ror:
        return kRotationMask;
    default:
        return kLogicMask;
    }
}

inline std::uint64_t make_initial_flags(std::uint64_t seed) {
    std::uint64_t flags = 0x202ull;
    std::uint64_t bits = seeded(seed, 0x55);
    if (bits & 0x01) flags |= kFlagCF;
    if (bits & 0x02) flags |= kFlagPF;
    if (bits & 0x04) flags |= kFlagAF;
    if (bits & 0x08) flags |= kFlagZF;
    if (bits & 0x10) flags |= kFlagSF;
    if (bits & 0x20) flags |= kFlagOF;
    return flags;
}

inline HostFeatures query_host_features() {
    HostFeatures features;
    int cpu_info[4] = {};
    __cpuid(cpu_info, 0);
    const int max_leaf = cpu_info[0];
    if (max_leaf >= 1) {
        __cpuidex(cpu_info, 1, 0);
        features.ssse3 = (cpu_info[2] & (1 << 9)) != 0;
        features.sse41 = (cpu_info[2] & (1 << 19)) != 0;
        features.aes = (cpu_info[2] & (1 << 25)) != 0;
    }
    if (max_leaf >= 7) {
        __cpuidex(cpu_info, 7, 0);
        features.rdpid = (cpu_info[2] & (1 << 22)) != 0;
    }
    return features;
}

inline cpueaxh_x86_context make_initial_context(std::uint64_t seed) {
    cpueaxh_x86_context context{};
    for (std::size_t index = 0; index < 16; ++index) {
        context.regs[index] = seeded(seed, 0x100 + index);
    }
    context.rflags = make_initial_flags(seed);
    context.mxcsr = static_cast<std::uint32_t>(kInitialMxcsr);
    return context;
}

inline std::array<std::uint8_t, kDataSize> make_initial_data(std::uint64_t seed) {
    std::array<std::uint8_t, kDataSize> data{};
    for (std::size_t index = 0; index < data.size(); ++index) {
        data[index] = narrow8(seeded(seed, 0x400 + index));
    }
    for (std::size_t index = 0; index < 32; ++index) {
        data[kSlotSize * 4 + index] = narrow8(index + 1);
        data[kSlotSize * 4 + kBufferSize + index] = narrow8(0x80u + index);
    }
    return data;
}

class CodeBuilder {
public:
    struct Label {
        std::size_t id;
    };

    Label make_label() {
        labels_.push_back(static_cast<std::size_t>(-1));
        return Label{ labels_.size() - 1 };
    }

    void mark(Label label) {
        labels_[label.id] = bytes_.size();
    }

    std::size_t offset() const {
        return bytes_.size();
    }

    std::size_t label_offset(Label label) const {
        return labels_[label.id];
    }

    void align(std::size_t alignment) {
        while ((bytes_.size() % alignment) != 0) {
            emit8(0x90);
        }
    }

    void emit8(std::uint8_t value) {
        bytes_.push_back(value);
    }

    void emit16(std::uint16_t value) {
        emit8(static_cast<std::uint8_t>(value & 0xFFu));
        emit8(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
    }

    void emit32(std::uint32_t value) {
        for (int shift = 0; shift < 32; shift += 8) {
            emit8(static_cast<std::uint8_t>((value >> shift) & 0xFFu));
        }
    }

    void emit64(std::uint64_t value) {
        for (int shift = 0; shift < 64; shift += 8) {
            emit8(static_cast<std::uint8_t>((value >> shift) & 0xFFu));
        }
    }

    void emit_bytes(const std::uint8_t* data, std::size_t size) {
        bytes_.insert(bytes_.end(), data, data + size);
    }

    void emit_modrm(std::uint8_t mod, std::uint8_t reg, std::uint8_t rm) {
        emit8(static_cast<std::uint8_t>((mod << 6) | ((reg & 7u) << 3) | (rm & 7u)));
    }

    void emit_sib(std::uint8_t scale, std::uint8_t index, std::uint8_t base) {
        emit8(static_cast<std::uint8_t>((scale << 6) | ((index & 7u) << 3) | (base & 7u)));
    }

    void emit_rex(bool w, std::uint8_t reg, std::uint8_t index, std::uint8_t rm) {
        std::uint8_t rex = 0x40u;
        if (w) rex |= 0x08u;
        if ((reg & 8u) != 0) rex |= 0x04u;
        if ((index & 8u) != 0) rex |= 0x02u;
        if ((rm & 8u) != 0) rex |= 0x01u;
        if (rex != 0x40u) {
            emit8(rex);
        }
    }

    void rip_rel32(Label label) {
        patches_.push_back(Patch{ label.id, bytes_.size(), 4 });
        emit32(0);
    }

    void rel32(Label label) {
        rip_rel32(label);
    }

    void rel8(Label label) {
        patches_.push_back(Patch{ label.id, bytes_.size(), 1 });
        emit8(0);
    }

    void ret() {
        emit8(0xC3);
    }

    void jmp32(Label label) {
        emit8(0xE9);
        rel32(label);
    }

    void call32(Label label) {
        emit8(0xE8);
        rel32(label);
    }

    void jcc32(std::uint8_t cc, Label label) {
        emit8(0x0F);
        emit8(static_cast<std::uint8_t>(0x80u + cc));
        rel32(label);
    }

    void mov_reg_reg(Reg dst, Reg src) {
        emit_rex(true, static_cast<std::uint8_t>(src), 0, static_cast<std::uint8_t>(dst));
        emit8(0x89);
        emit_modrm(3, static_cast<std::uint8_t>(src), static_cast<std::uint8_t>(dst));
    }

    void binary_reg_reg(BinaryOp op, Reg dst, Reg src) {
        emit_rex(true, static_cast<std::uint8_t>(src), 0, static_cast<std::uint8_t>(dst));
        switch (op) {
        case BinaryOp::Add: emit8(0x01); break;
        case BinaryOp::Adc: emit8(0x11); break;
        case BinaryOp::Sub: emit8(0x29); break;
        case BinaryOp::Sbb: emit8(0x19); break;
        case BinaryOp::And: emit8(0x21); break;
        case BinaryOp::Or: emit8(0x09); break;
        case BinaryOp::Xor: emit8(0x31); break;
        case BinaryOp::Cmp: emit8(0x39); break;
        case BinaryOp::Test: emit8(0x85); break;
        }
        emit_modrm(3, static_cast<std::uint8_t>(src), static_cast<std::uint8_t>(dst));
    }

    void binary_reg_imm(BinaryOp op, Reg dst, std::uint32_t imm) {
        std::uint8_t reg_field = 0;
        std::uint8_t opcode = 0x81;
        if (op == BinaryOp::Test) {
            opcode = 0xF7;
            reg_field = 0;
        }
        else {
            switch (op) {
            case BinaryOp::Add: reg_field = 0; break;
            case BinaryOp::Or: reg_field = 1; break;
            case BinaryOp::Adc: reg_field = 2; break;
            case BinaryOp::Sbb: reg_field = 3; break;
            case BinaryOp::And: reg_field = 4; break;
            case BinaryOp::Sub: reg_field = 5; break;
            case BinaryOp::Xor: reg_field = 6; break;
            case BinaryOp::Cmp: reg_field = 7; break;
            case BinaryOp::Test: reg_field = 0; break;
            }
        }
        emit_rex(true, reg_field, 0, static_cast<std::uint8_t>(dst));
        emit8(opcode);
        emit_modrm(3, reg_field, static_cast<std::uint8_t>(dst));
        emit32(imm);
    }

    void unary_reg(UnaryOp op, Reg reg) {
        std::uint8_t reg_field = 0;
        std::uint8_t opcode = 0xFF;
        switch (op) {
        case UnaryOp::Inc: reg_field = 0; opcode = 0xFF; break;
        case UnaryOp::Dec: reg_field = 1; opcode = 0xFF; break;
        case UnaryOp::Not: reg_field = 2; opcode = 0xF7; break;
        case UnaryOp::Neg: reg_field = 3; opcode = 0xF7; break;
        }
        emit_rex(true, reg_field, 0, static_cast<std::uint8_t>(reg));
        emit8(opcode);
        emit_modrm(3, reg_field, static_cast<std::uint8_t>(reg));
    }

    void shift_imm(ShiftOp op, Reg reg, std::uint8_t amount) {
        std::uint8_t reg_field = 0;
        switch (op) {
        case ShiftOp::Rol: reg_field = 0; break;
        case ShiftOp::Ror: reg_field = 1; break;
        case ShiftOp::Shl: reg_field = 4; break;
        case ShiftOp::Shr: reg_field = 5; break;
        case ShiftOp::Sar: reg_field = 7; break;
        }
        emit_rex(true, reg_field, 0, static_cast<std::uint8_t>(reg));
        emit8(0xC1);
        emit_modrm(3, reg_field, static_cast<std::uint8_t>(reg));
        emit8(amount);
    }

    void shift_cl(ShiftOp op, Reg reg) {
        std::uint8_t reg_field = 0;
        switch (op) {
        case ShiftOp::Rol: reg_field = 0; break;
        case ShiftOp::Ror: reg_field = 1; break;
        case ShiftOp::Shl: reg_field = 4; break;
        case ShiftOp::Shr: reg_field = 5; break;
        case ShiftOp::Sar: reg_field = 7; break;
        }
        emit_rex(true, reg_field, 0, static_cast<std::uint8_t>(reg));
        emit8(0xD3);
        emit_modrm(3, reg_field, static_cast<std::uint8_t>(reg));
    }

    void bt_reg_imm(Reg reg, std::uint8_t bit) {
        emit_rex(true, 4, 0, static_cast<std::uint8_t>(reg));
        emit8(0x0F);
        emit8(0xBA);
        emit_modrm(3, 4, static_cast<std::uint8_t>(reg));
        emit8(bit);
    }

    void bt_reg_reg(Reg dst, Reg src) {
        emit_rex(true, static_cast<std::uint8_t>(src), 0, static_cast<std::uint8_t>(dst));
        emit8(0x0F);
        emit8(0xA3);
        emit_modrm(3, static_cast<std::uint8_t>(src), static_cast<std::uint8_t>(dst));
    }

    void bsf(Reg dst, Reg src) {
        emit_rex(true, static_cast<std::uint8_t>(dst), 0, static_cast<std::uint8_t>(src));
        emit8(0x0F);
        emit8(0xBC);
        emit_modrm(3, static_cast<std::uint8_t>(dst), static_cast<std::uint8_t>(src));
    }

    void bswap(Reg reg) {
        emit_rex(true, 0, 0, static_cast<std::uint8_t>(reg));
        emit8(0x0F);
        emit8(static_cast<std::uint8_t>(0xC8u + (static_cast<std::uint8_t>(reg) & 7u)));
    }

    void setcc(std::uint8_t cc, Reg reg) {
        emit_rex(false, 0, 0, static_cast<std::uint8_t>(reg));
        emit8(0x0F);
        emit8(static_cast<std::uint8_t>(0x90u + cc));
        emit_modrm(3, 0, static_cast<std::uint8_t>(reg));
    }

    void cmovcc(std::uint8_t cc, Reg dst, Reg src) {
        emit_rex(true, static_cast<std::uint8_t>(dst), 0, static_cast<std::uint8_t>(src));
        emit8(0x0F);
        emit8(static_cast<std::uint8_t>(0x40u + cc));
        emit_modrm(3, static_cast<std::uint8_t>(dst), static_cast<std::uint8_t>(src));
    }

    void movzx_byte(Reg dst, Label label) {
        emit_rex(true, static_cast<std::uint8_t>(dst), 0, 5);
        emit8(0x0F);
        emit8(0xB6);
        emit_modrm(0, static_cast<std::uint8_t>(dst), 5);
        rip_rel32(label);
    }

    void movzx_word(Reg dst, Label label) {
        emit_rex(true, static_cast<std::uint8_t>(dst), 0, 5);
        emit8(0x0F);
        emit8(0xB7);
        emit_modrm(0, static_cast<std::uint8_t>(dst), 5);
        rip_rel32(label);
    }

    void movsx_byte(Reg dst, Label label) {
        emit_rex(true, static_cast<std::uint8_t>(dst), 0, 5);
        emit8(0x0F);
        emit8(0xBE);
        emit_modrm(0, static_cast<std::uint8_t>(dst), 5);
        rip_rel32(label);
    }

    void movsx_word(Reg dst, Label label) {
        emit_rex(true, static_cast<std::uint8_t>(dst), 0, 5);
        emit8(0x0F);
        emit8(0xBF);
        emit_modrm(0, static_cast<std::uint8_t>(dst), 5);
        rip_rel32(label);
    }

    void movsxd_dword(Reg dst, Label label) {
        emit_rex(true, static_cast<std::uint8_t>(dst), 0, 5);
        emit8(0x63);
        emit_modrm(0, static_cast<std::uint8_t>(dst), 5);
        rip_rel32(label);
    }

    void lea_scaled(Reg dst, Reg base, Reg index, std::uint8_t scale, std::int8_t disp) {
        emit_rex(true, static_cast<std::uint8_t>(dst), static_cast<std::uint8_t>(index), static_cast<std::uint8_t>(base));
        emit8(0x8D);
        emit_modrm(1, static_cast<std::uint8_t>(dst), 4);
        emit_sib(scale, static_cast<std::uint8_t>(index), static_cast<std::uint8_t>(base));
        emit8(static_cast<std::uint8_t>(disp));
    }

    void lea_rip(Reg dst, Label label) {
        emit_rex(true, static_cast<std::uint8_t>(dst), 0, 5);
        emit8(0x8D);
        emit_modrm(0, static_cast<std::uint8_t>(dst), 5);
        rip_rel32(label);
    }

    void mov_mem_reg(Label label, Reg src) {
        emit_rex(true, static_cast<std::uint8_t>(src), 0, 5);
        emit8(0x89);
        emit_modrm(0, static_cast<std::uint8_t>(src), 5);
        rip_rel32(label);
    }

    void mov_reg_mem(Reg dst, Label label) {
        emit_rex(true, static_cast<std::uint8_t>(dst), 0, 5);
        emit8(0x8B);
        emit_modrm(0, static_cast<std::uint8_t>(dst), 5);
        rip_rel32(label);
    }

    void binary_mem_reg(BinaryOp op, Label label, Reg src) {
        emit_rex(true, static_cast<std::uint8_t>(src), 0, 5);
        switch (op) {
        case BinaryOp::Add: emit8(0x01); break;
        case BinaryOp::Sub: emit8(0x29); break;
        case BinaryOp::And: emit8(0x21); break;
        case BinaryOp::Or: emit8(0x09); break;
        case BinaryOp::Xor: emit8(0x31); break;
        default: emit8(0x01); break;
        }
        emit_modrm(0, static_cast<std::uint8_t>(src), 5);
        rip_rel32(label);
    }

    void xadd_mem_reg(Label label, Reg src) {
        emit_rex(true, static_cast<std::uint8_t>(src), 0, 5);
        emit8(0x0F);
        emit8(0xC1);
        emit_modrm(0, static_cast<std::uint8_t>(src), 5);
        rip_rel32(label);
    }

    void cmpxchg_mem_reg(Label label, Reg src) {
        emit_rex(true, static_cast<std::uint8_t>(src), 0, 5);
        emit8(0x0F);
        emit8(0xB1);
        emit_modrm(0, static_cast<std::uint8_t>(src), 5);
        rip_rel32(label);
    }

    void push_reg(Reg reg) {
        emit_rex(false, 0, 0, static_cast<std::uint8_t>(reg));
        emit8(static_cast<std::uint8_t>(0x50u + (static_cast<std::uint8_t>(reg) & 7u)));
    }

    void pop_reg(Reg reg) {
        emit_rex(false, 0, 0, static_cast<std::uint8_t>(reg));
        emit8(static_cast<std::uint8_t>(0x58u + (static_cast<std::uint8_t>(reg) & 7u)));
    }

    void pushf() {
        emit8(0x9C);
    }

    void lahf() {
        emit8(0x9F);
    }

    void enter(std::uint16_t size) {
        emit8(0xC8);
        emit16(size);
        emit8(0x00);
    }

    void leave() {
        emit8(0xC9);
    }

    void mov_r32_imm(Reg reg, std::uint32_t imm) {
        emit_rex(false, 0, 0, static_cast<std::uint8_t>(reg));
        emit8(static_cast<std::uint8_t>(0xB8u + (static_cast<std::uint8_t>(reg) & 7u)));
        emit32(imm);
    }

    void mov_r8_imm(Reg reg, std::uint8_t imm) {
        emit_rex(false, 0, 0, static_cast<std::uint8_t>(reg));
        emit8(static_cast<std::uint8_t>(0xB0u + (static_cast<std::uint8_t>(reg) & 7u)));
        emit8(imm);
    }

    void rep() {
        emit8(0xF3);
    }

    void movsb() { emit8(0xA4); }
    void movsw() { emit8(0x66); emit8(0xA5); }
    void movsd() { emit8(0xA5); }
    void movsq() { emit8(0x48); emit8(0xA5); }
    void lodsb() { emit8(0xAC); }
    void lodsq() { emit8(0x48); emit8(0xAD); }
    void stosb() { emit8(0xAA); }
    void stosq() { emit8(0x48); emit8(0xAB); }
    void cmpsb() { emit8(0xA6); }
    void scasb() { emit8(0xAE); }

    void movdqu_load(Reg dst, Label label) {
        emit_rex(false, static_cast<std::uint8_t>(dst), 0, 5);
        emit8(0xF3);
        emit8(0x0F);
        emit8(0x6F);
        emit_modrm(0, static_cast<std::uint8_t>(dst), 5);
        rip_rel32(label);
    }

    void movdqu_store(Label label, Reg src) {
        emit_rex(false, static_cast<std::uint8_t>(src), 0, 5);
        emit8(0xF3);
        emit8(0x0F);
        emit8(0x7F);
        emit_modrm(0, static_cast<std::uint8_t>(src), 5);
        rip_rel32(label);
    }

    void paddq(Reg dst, Reg src) {
        emit_rex(false, static_cast<std::uint8_t>(dst), 0, static_cast<std::uint8_t>(src));
        emit8(0x66);
        emit8(0x0F);
        emit8(0xD4);
        emit_modrm(3, static_cast<std::uint8_t>(dst), static_cast<std::uint8_t>(src));
    }

    void pxor(Reg dst, Reg src) {
        emit_rex(false, static_cast<std::uint8_t>(dst), 0, static_cast<std::uint8_t>(src));
        emit8(0x66);
        emit8(0x0F);
        emit8(0xEF);
        emit_modrm(3, static_cast<std::uint8_t>(dst), static_cast<std::uint8_t>(src));
    }

    void pand(Reg dst, Reg src) {
        emit_rex(false, static_cast<std::uint8_t>(dst), 0, static_cast<std::uint8_t>(src));
        emit8(0x66);
        emit8(0x0F);
        emit8(0xDB);
        emit_modrm(3, static_cast<std::uint8_t>(dst), static_cast<std::uint8_t>(src));
    }

    void por(Reg dst, Reg src) {
        emit_rex(false, static_cast<std::uint8_t>(dst), 0, static_cast<std::uint8_t>(src));
        emit8(0x66);
        emit8(0x0F);
        emit8(0xEB);
        emit_modrm(3, static_cast<std::uint8_t>(dst), static_cast<std::uint8_t>(src));
    }

    void pcmpeqb(Reg dst, Reg src) {
        emit_rex(false, static_cast<std::uint8_t>(dst), 0, static_cast<std::uint8_t>(src));
        emit8(0x66);
        emit8(0x0F);
        emit8(0x74);
        emit_modrm(3, static_cast<std::uint8_t>(dst), static_cast<std::uint8_t>(src));
    }

    void pshufd(Reg dst, Reg src, std::uint8_t imm) {
        emit_rex(false, static_cast<std::uint8_t>(dst), 0, static_cast<std::uint8_t>(src));
        emit8(0x66);
        emit8(0x0F);
        emit8(0x70);
        emit_modrm(3, static_cast<std::uint8_t>(dst), static_cast<std::uint8_t>(src));
        emit8(imm);
    }

    void pslldq(Reg reg, std::uint8_t imm) {
        emit_rex(false, 7, 0, static_cast<std::uint8_t>(reg));
        emit8(0x66);
        emit8(0x0F);
        emit8(0x73);
        emit_modrm(3, 7, static_cast<std::uint8_t>(reg));
        emit8(imm);
    }

    void psrldq(Reg reg, std::uint8_t imm) {
        emit_rex(false, 3, 0, static_cast<std::uint8_t>(reg));
        emit8(0x66);
        emit8(0x0F);
        emit8(0x73);
        emit_modrm(3, 3, static_cast<std::uint8_t>(reg));
        emit8(imm);
    }

    void pshufb(Reg dst, Reg src) {
        emit_rex(false, static_cast<std::uint8_t>(dst), 0, static_cast<std::uint8_t>(src));
        emit8(0x66);
        emit8(0x0F);
        emit8(0x38);
        emit8(0x00);
        emit_modrm(3, static_cast<std::uint8_t>(dst), static_cast<std::uint8_t>(src));
    }

    void aesenc(Reg dst, Reg src) {
        emit_rex(false, static_cast<std::uint8_t>(dst), 0, static_cast<std::uint8_t>(src));
        emit8(0x66);
        emit8(0x0F);
        emit8(0x38);
        emit8(0xDC);
        emit_modrm(3, static_cast<std::uint8_t>(dst), static_cast<std::uint8_t>(src));
    }

    void aesenclast(Reg dst, Reg src) {
        emit_rex(false, static_cast<std::uint8_t>(dst), 0, static_cast<std::uint8_t>(src));
        emit8(0x66);
        emit8(0x0F);
        emit8(0x38);
        emit8(0xDD);
        emit_modrm(3, static_cast<std::uint8_t>(dst), static_cast<std::uint8_t>(src));
    }

    void aeskeygenassist(Reg dst, Reg src, std::uint8_t imm) {
        emit_rex(false, static_cast<std::uint8_t>(dst), 0, static_cast<std::uint8_t>(src));
        emit8(0x66);
        emit8(0x0F);
        emit8(0x3A);
        emit8(0xDF);
        emit_modrm(3, static_cast<std::uint8_t>(dst), static_cast<std::uint8_t>(src));
        emit8(imm);
    }

    void roundsd(Reg dst, Reg src, std::uint8_t imm) {
        emit_rex(false, static_cast<std::uint8_t>(dst), 0, static_cast<std::uint8_t>(src));
        emit8(0x66);
        emit8(0x0F);
        emit8(0x3A);
        emit8(0x0B);
        emit_modrm(3, static_cast<std::uint8_t>(dst), static_cast<std::uint8_t>(src));
        emit8(imm);
    }

    void roundss(Reg dst, Reg src, std::uint8_t imm) {
        emit_rex(false, static_cast<std::uint8_t>(dst), 0, static_cast<std::uint8_t>(src));
        emit8(0x66);
        emit8(0x0F);
        emit8(0x3A);
        emit8(0x0A);
        emit_modrm(3, static_cast<std::uint8_t>(dst), static_cast<std::uint8_t>(src));
        emit8(imm);
    }

    bool finalize() {
        for (const Patch& patch : patches_) {
            const std::size_t target = labels_[patch.label_id];
            if (target == static_cast<std::size_t>(-1)) {
                return false;
            }
            const std::int64_t next = static_cast<std::int64_t>(patch.offset + patch.size);
            const std::int64_t disp = static_cast<std::int64_t>(target) - next;
            if (patch.size == 4) {
                const std::int32_t value = static_cast<std::int32_t>(disp);
                for (int shift = 0; shift < 32; shift += 8) {
                    bytes_[patch.offset + shift / 8] = static_cast<std::uint8_t>((value >> shift) & 0xFF);
                }
            }
            else {
                const std::int8_t value = static_cast<std::int8_t>(disp);
                bytes_[patch.offset] = static_cast<std::uint8_t>(value);
            }
        }
        return true;
    }

    const std::vector<std::uint8_t>& bytes() const {
        return bytes_;
    }

private:
    struct Patch {
        std::size_t label_id;
        std::size_t offset;
        std::size_t size;
    };

    std::vector<std::uint8_t> bytes_;
    std::vector<std::size_t> labels_;
    std::vector<Patch> patches_;
};

inline std::vector<ProgramSpec> make_specs(const HostFeatures& features) {
    std::vector<ProgramSpec> specs;
    const BinaryOp binary_ops[] = {
        BinaryOp::Add, BinaryOp::Adc, BinaryOp::Sub, BinaryOp::Sbb,
        BinaryOp::And, BinaryOp::Or, BinaryOp::Xor, BinaryOp::Cmp, BinaryOp::Test
    };
    const BinaryOp imm_ops[] = {
        BinaryOp::Add, BinaryOp::Sub, BinaryOp::And, BinaryOp::Or,
        BinaryOp::Xor, BinaryOp::Cmp, BinaryOp::Test
    };
    const UnaryOp unary_ops[] = { UnaryOp::Inc, UnaryOp::Dec, UnaryOp::Neg, UnaryOp::Not };
    const ShiftOp shift_ops[] = { ShiftOp::Rol, ShiftOp::Ror, ShiftOp::Shl, ShiftOp::Shr, ShiftOp::Sar };
    for (const BinaryOp op : binary_ops) {
        for (std::size_t index = 0; index < kBinaryPairs.size(); ++index) {
            specs.push_back({ Family::BinaryRegReg, static_cast<std::uint32_t>(op), static_cast<std::uint32_t>(index), binary_flag_mask(op), std::string(binary_name(op)) + "_rr_" + kBinaryPairs[index].name });
        }
    }
    for (const BinaryOp op : imm_ops) {
        for (std::size_t index = 0; index < kCoreRegs.size(); ++index) {
            specs.push_back({ Family::BinaryRegImm, static_cast<std::uint32_t>(op), static_cast<std::uint32_t>(index), binary_flag_mask(op), std::string(binary_name(op)) + "_ri_" + kCoreRegs[index].name });
        }
    }
    for (const UnaryOp op : unary_ops) {
        for (std::size_t index = 0; index < kUnaryRegs.size(); ++index) {
            specs.push_back({ Family::UnaryReg, static_cast<std::uint32_t>(op), static_cast<std::uint32_t>(index), unary_flag_mask(op), std::string(unary_name(op)) + "_" + kUnaryRegs[index].name });
        }
    }
    for (const ShiftOp op : shift_ops) {
        for (std::size_t index = 0; index < kCoreRegs.size(); ++index) {
            const std::uint64_t mask = shift_flag_mask(op);
            specs.push_back({ Family::ShiftImm, static_cast<std::uint32_t>(op), static_cast<std::uint32_t>(index), mask, std::string(shift_name(op)) + "_imm_" + kCoreRegs[index].name });
            specs.push_back({ Family::ShiftCl, static_cast<std::uint32_t>(op), static_cast<std::uint32_t>(index), mask, std::string(shift_name(op)) + "_cl_" + kCoreRegs[index].name });
        }
    }
    specs.push_back({ Family::BitOps, static_cast<std::uint32_t>(BitOp::BtImm), 0, kBitTestMask, "bt_imm_rax" });
    specs.push_back({ Family::BitOps, static_cast<std::uint32_t>(BitOp::BtReg), 0, kBitTestMask, "bt_reg_r8_rcx" });
    specs.push_back({ Family::BitOps, static_cast<std::uint32_t>(BitOp::Bsf), 0, kBitScanMask, "bsf_rdx_rbx" });
    specs.push_back({ Family::BitOps, static_cast<std::uint32_t>(BitOp::Bswap), 0, kStatusMask, "bswap_r11" });
    specs.push_back({ Family::BitOps, static_cast<std::uint32_t>(BitOp::BsfAlt), 0, kBitScanMask, "bsf_r9_r10" });
    specs.push_back({ Family::BitOps, static_cast<std::uint32_t>(BitOp::BswapAlt), 0, kStatusMask, "bswap_rax" });
    for (const CondSpec& condition : kConditions) {
        specs.push_back({ Family::FlagOps, static_cast<std::uint32_t>(FlagProgram::Setcc), condition.code, kStatusMask, std::string("set") + condition.name });
        specs.push_back({ Family::FlagOps, static_cast<std::uint32_t>(FlagProgram::Cmovcc), condition.code, kStatusMask, std::string("cmov") + condition.name });
        specs.push_back({ Family::FlagOps, static_cast<std::uint32_t>(FlagProgram::Jcc), condition.code, kStatusMask, std::string("j") + condition.name });
    }
    specs.push_back({ Family::MoveOps, static_cast<std::uint32_t>(MoveProgram::MovA), 0, 0, "mov_rax_r8" });
    specs.push_back({ Family::MoveOps, static_cast<std::uint32_t>(MoveProgram::MovB), 0, 0, "mov_r9_rbx" });
    specs.push_back({ Family::MoveOps, static_cast<std::uint32_t>(MoveProgram::MovzxByte), 0, 0, "movzx_byte" });
    specs.push_back({ Family::MoveOps, static_cast<std::uint32_t>(MoveProgram::MovzxWord), 0, 0, "movzx_word" });
    specs.push_back({ Family::MoveOps, static_cast<std::uint32_t>(MoveProgram::MovsxByte), 0, 0, "movsx_byte" });
    specs.push_back({ Family::MoveOps, static_cast<std::uint32_t>(MoveProgram::MovsxWord), 0, 0, "movsx_word" });
    specs.push_back({ Family::MoveOps, static_cast<std::uint32_t>(MoveProgram::Movsxd), 0, 0, "movsxd_dword" });
    specs.push_back({ Family::MoveOps, static_cast<std::uint32_t>(MoveProgram::Lea2), 0, 0, "lea_scale2" });
    specs.push_back({ Family::MoveOps, static_cast<std::uint32_t>(MoveProgram::Lea4), 0, 0, "lea_scale4" });
    specs.push_back({ Family::MemoryOps, static_cast<std::uint32_t>(MemoryProgram::MovRoundtrip), 0, 0, "mov_mem_roundtrip" });
    specs.push_back({ Family::MemoryOps, static_cast<std::uint32_t>(MemoryProgram::AddMem), 0, kStatusMask, "add_mem" });
    specs.push_back({ Family::MemoryOps, static_cast<std::uint32_t>(MemoryProgram::SubMem), 0, kStatusMask, "sub_mem" });
    specs.push_back({ Family::MemoryOps, static_cast<std::uint32_t>(MemoryProgram::AndMem), 0, kLogicMask, "and_mem" });
    specs.push_back({ Family::MemoryOps, static_cast<std::uint32_t>(MemoryProgram::OrMem), 0, kLogicMask, "or_mem" });
    specs.push_back({ Family::MemoryOps, static_cast<std::uint32_t>(MemoryProgram::XorMem), 0, kLogicMask, "xor_mem" });
    specs.push_back({ Family::MemoryOps, static_cast<std::uint32_t>(MemoryProgram::XaddMem), 0, kStatusMask, "xadd_mem" });
    specs.push_back({ Family::MemoryOps, static_cast<std::uint32_t>(MemoryProgram::CmpxchgMem), 0, kStatusMask, "cmpxchg_mem" });
    specs.push_back({ Family::StackOps, static_cast<std::uint32_t>(StackProgram::PushPopPair), 0, 0, "push_pop_pair" });
    specs.push_back({ Family::StackOps, static_cast<std::uint32_t>(StackProgram::PushPopFlags), 0, 0, "pushf_pop_lahf" });
    specs.push_back({ Family::StackOps, static_cast<std::uint32_t>(StackProgram::EnterLeave), 0, 0, "enter_leave" });
    specs.push_back({ Family::StackOps, static_cast<std::uint32_t>(StackProgram::PushChain), 0, 0, "push_chain" });
    specs.push_back({ Family::StringOps, static_cast<std::uint32_t>(StringProgram::Movsb), 0, 0, "movsb" });
    specs.push_back({ Family::StringOps, static_cast<std::uint32_t>(StringProgram::Movsw), 0, 0, "movsw" });
    specs.push_back({ Family::StringOps, static_cast<std::uint32_t>(StringProgram::Movsd), 0, 0, "movsd" });
    specs.push_back({ Family::StringOps, static_cast<std::uint32_t>(StringProgram::Movsq), 0, 0, "movsq" });
    specs.push_back({ Family::StringOps, static_cast<std::uint32_t>(StringProgram::RepMovsb), 0, 0, "rep_movsb" });
    specs.push_back({ Family::StringOps, static_cast<std::uint32_t>(StringProgram::RepMovsw), 0, 0, "rep_movsw" });
    specs.push_back({ Family::StringOps, static_cast<std::uint32_t>(StringProgram::Lodsb), 0, 0, "lodsb" });
    specs.push_back({ Family::StringOps, static_cast<std::uint32_t>(StringProgram::Lodsq), 0, 0, "lodsq" });
    specs.push_back({ Family::StringOps, static_cast<std::uint32_t>(StringProgram::Stosb), 0, 0, "stosb" });
    specs.push_back({ Family::StringOps, static_cast<std::uint32_t>(StringProgram::Stosq), 0, 0, "stosq" });
    specs.push_back({ Family::StringOps, static_cast<std::uint32_t>(StringProgram::Cmpsb), 0, kStatusMask, "cmpsb" });
    specs.push_back({ Family::StringOps, static_cast<std::uint32_t>(StringProgram::Scasb), 0, kStatusMask, "scasb" });
    specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::Paddq), 0, 0, "paddq" });
    specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::Pxor), 0, 0, "pxor" });
    specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::Pand), 0, 0, "pand" });
    specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::Por), 0, 0, "por" });
    specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::Pcmpeqb), 0, 0, "pcmpeqb" });
    specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::Pshufd), 0, 0, "pshufd" });
    specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::Pslldq), 0, 0, "pslldq" });
    specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::Psrldq), 0, 0, "psrldq" });
    specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::PaddqPxor), 0, 0, "paddq_pxor" });
    specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::Movdqu), 0, 0, "movdqu_roundtrip" });
    if (features.ssse3) {
        specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::Pshufb), 0, 0, "pshufb" });
    }
    if (features.aes) {
        specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::Aesenc), 0, 0, "aesenc" });
        specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::Aesenclast), 0, 0, "aesenclast" });
        for (std::size_t index = 0; index < kAesKeygenAssistImmediates.size(); ++index) {
            specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::Aeskeygenassist), static_cast<std::uint32_t>(index), 0, "aeskeygenassist_imm" + std::to_string(kAesKeygenAssistImmediates[index]) });
        }
    }
    if (features.sse41) {
        for (std::size_t index = 0; index < kRoundImmediates.size(); ++index) {
            specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::Roundsd), static_cast<std::uint32_t>(index), 0, "roundsd_imm" + std::to_string(kRoundImmediates[index]) });
            specs.push_back({ Family::VectorOps, static_cast<std::uint32_t>(VectorProgram::Roundss), static_cast<std::uint32_t>(index), 0, "roundss_imm" + std::to_string(kRoundImmediates[index]) });
        }
    }
    specs.push_back({ Family::ControlFlow, static_cast<std::uint32_t>(ControlProgram::JmpSkip), 0, kStatusMask, "jmp_skip" });
    specs.push_back({ Family::ControlFlow, static_cast<std::uint32_t>(ControlProgram::CallAdd), 0, kStatusMask, "call_add" });
    specs.push_back({ Family::ControlFlow, static_cast<std::uint32_t>(ControlProgram::CallXor), 0, kLogicMask, "call_xor" });
    specs.push_back({ Family::ControlFlow, static_cast<std::uint32_t>(ControlProgram::CallLea), 0, 0, "call_lea" });
    return specs;
}

inline BuiltCase build_case(const ProgramSpec& spec, std::uint64_t seed) {
    BuiltCase built{};
    built.spec = spec;
    built.seed = seed;
    built.initial_context = make_initial_context(seed);
    auto data = make_initial_data(seed);

    CodeBuilder code;
    const CodeBuilder::Label slot0 = code.make_label();
    const CodeBuilder::Label slot1 = code.make_label();
    const CodeBuilder::Label slot2 = code.make_label();
    const CodeBuilder::Label slot3 = code.make_label();
    const CodeBuilder::Label buffer0 = code.make_label();
    const CodeBuilder::Label buffer1 = code.make_label();
    const CodeBuilder::Label label_true = code.make_label();
    const CodeBuilder::Label label_false = code.make_label();
    const CodeBuilder::Label label_end = code.make_label();
    const CodeBuilder::Label helper = code.make_label();
    const CodeBuilder::Label after_helper = code.make_label();

    switch (spec.family) {
    case Family::BinaryRegReg: {
        const auto op = static_cast<BinaryOp>(spec.op);
        const PairSpec pair = kBinaryPairs[spec.variant % kBinaryPairs.size()];
        code.binary_reg_reg(op, pair.dst, pair.src);
        break;
    }
    case Family::BinaryRegImm: {
        const auto op = static_cast<BinaryOp>(spec.op);
        const Reg reg = kCoreRegs[spec.variant % kCoreRegs.size()].reg;
        std::uint32_t imm = narrow32(seeded(seed, 0x600));
        if (imm == 0) imm = 1;
        code.binary_reg_imm(op, reg, imm);
        break;
    }
    case Family::UnaryReg: {
        const auto op = static_cast<UnaryOp>(spec.op);
        const Reg reg = kUnaryRegs[spec.variant % kUnaryRegs.size()].reg;
        code.unary_reg(op, reg);
        break;
    }
    case Family::ShiftImm: {
        const auto op = static_cast<ShiftOp>(spec.op);
        const Reg reg = kCoreRegs[spec.variant % kCoreRegs.size()].reg;
        code.shift_imm(op, reg, 1);
        break;
    }
    case Family::ShiftCl: {
        const auto op = static_cast<ShiftOp>(spec.op);
        const Reg reg = kCoreRegs[spec.variant % kCoreRegs.size()].reg;
        built.initial_context.regs[static_cast<std::size_t>(Reg::RCX)] = 1;
        code.shift_cl(op, reg);
        break;
    }
    case Family::BitOps: {
        switch (static_cast<BitOp>(spec.op)) {
        case BitOp::BtImm:
            code.bt_reg_imm(Reg::RAX, static_cast<std::uint8_t>(seed % 63));
            break;
        case BitOp::BtReg:
            built.initial_context.regs[static_cast<std::size_t>(Reg::RCX)] = seed % 63;
            code.bt_reg_reg(Reg::R8, Reg::RCX);
            break;
        case BitOp::Bsf:
            built.initial_context.regs[static_cast<std::size_t>(Reg::RBX)] |= 1;
            code.bsf(Reg::RDX, Reg::RBX);
            break;
        case BitOp::Bswap:
            code.bswap(Reg::R11);
            break;
        case BitOp::BsfAlt:
            built.initial_context.regs[static_cast<std::size_t>(Reg::R10)] |= 1;
            code.bsf(Reg::R9, Reg::R10);
            break;
        case BitOp::BswapAlt:
            code.bswap(Reg::RAX);
            break;
        }
        break;
    }
    case Family::FlagOps: {
        const std::uint8_t cc = static_cast<std::uint8_t>(spec.variant);
        const auto program = static_cast<FlagProgram>(spec.op);
        code.binary_reg_reg(BinaryOp::Cmp, Reg::RAX, Reg::RBX);
        if (program == FlagProgram::Setcc) {
            code.setcc(cc, Reg::RAX);
        }
        else if (program == FlagProgram::Cmovcc) {
            code.cmovcc(cc, Reg::R10, Reg::R11);
        }
        else {
            code.jcc32(cc, label_true);
            code.mov_reg_reg(Reg::R12, Reg::R13);
            code.jmp32(label_end);
            code.mark(label_true);
            code.mov_reg_reg(Reg::R12, Reg::R14);
            code.mark(label_end);
        }
        break;
    }
    case Family::MoveOps: {
        switch (static_cast<MoveProgram>(spec.op)) {
        case MoveProgram::MovA:
            code.mov_reg_reg(Reg::RAX, Reg::R8);
            break;
        case MoveProgram::MovB:
            code.mov_reg_reg(Reg::R9, Reg::RBX);
            break;
        case MoveProgram::MovzxByte:
            code.movzx_byte(Reg::RAX, slot0);
            break;
        case MoveProgram::MovzxWord:
            code.movzx_word(Reg::R10, slot0);
            break;
        case MoveProgram::MovsxByte:
            data[0] |= 0x80u;
            code.movsx_byte(Reg::R11, slot0);
            break;
        case MoveProgram::MovsxWord:
            data[0] = 0x34u;
            data[1] = 0xF2u;
            code.movsx_word(Reg::R12, slot0);
            break;
        case MoveProgram::Movsxd:
            data[0] = 0x78u;
            data[1] = 0x56u;
            data[2] = 0x34u;
            data[3] = 0xF2u;
            code.movsxd_dword(Reg::R13, slot0);
            break;
        case MoveProgram::Lea2:
            code.lea_scaled(Reg::R14, Reg::RBX, Reg::RSI, 1, 0x24);
            break;
        case MoveProgram::Lea4:
            code.lea_scaled(Reg::R15, Reg::R8, Reg::R9, 2, 0x18);
            break;
        }
        break;
    }
    case Family::MemoryOps: {
        switch (static_cast<MemoryProgram>(spec.op)) {
        case MemoryProgram::MovRoundtrip:
            code.mov_mem_reg(buffer0, Reg::RAX);
            code.mov_reg_mem(Reg::RBX, buffer0);
            break;
        case MemoryProgram::AddMem:
            code.binary_mem_reg(BinaryOp::Add, buffer0, Reg::R8);
            code.mov_reg_mem(Reg::RDX, buffer0);
            break;
        case MemoryProgram::SubMem:
            code.binary_mem_reg(BinaryOp::Sub, buffer0, Reg::R9);
            code.mov_reg_mem(Reg::RDX, buffer0);
            break;
        case MemoryProgram::AndMem:
            code.binary_mem_reg(BinaryOp::And, buffer0, Reg::R10);
            code.mov_reg_mem(Reg::RDX, buffer0);
            break;
        case MemoryProgram::OrMem:
            code.binary_mem_reg(BinaryOp::Or, buffer0, Reg::R11);
            code.mov_reg_mem(Reg::RDX, buffer0);
            break;
        case MemoryProgram::XorMem:
            code.binary_mem_reg(BinaryOp::Xor, buffer0, Reg::R12);
            code.mov_reg_mem(Reg::RDX, buffer0);
            break;
        case MemoryProgram::XaddMem:
            code.xadd_mem_reg(buffer0, Reg::R13);
            code.mov_reg_mem(Reg::RDX, buffer0);
            break;
        case MemoryProgram::CmpxchgMem:
            code.cmpxchg_mem_reg(buffer0, Reg::R14);
            code.mov_reg_mem(Reg::RDX, buffer0);
            break;
        }
        break;
    }
    case Family::StackOps: {
        switch (static_cast<StackProgram>(spec.op)) {
        case StackProgram::PushPopPair:
            code.push_reg(Reg::RAX);
            code.push_reg(Reg::RBX);
            code.pop_reg(Reg::R10);
            code.pop_reg(Reg::R11);
            break;
        case StackProgram::PushPopFlags:
            code.pushf();
            code.pop_reg(Reg::RAX);
            code.lahf();
            break;
        case StackProgram::EnterLeave:
            code.enter(0x20);
            code.mov_reg_reg(Reg::R12, Reg::RBP);
            code.leave();
            break;
        case StackProgram::PushChain:
            code.push_reg(Reg::R8);
            code.push_reg(Reg::R9);
            code.push_reg(Reg::R10);
            code.pop_reg(Reg::R11);
            code.pop_reg(Reg::R12);
            code.pop_reg(Reg::R13);
            break;
        }
        break;
    }
    case Family::StringOps: {
        const auto program = static_cast<StringProgram>(spec.op);
        code.lea_rip(Reg::RSI, buffer0);
        code.lea_rip(Reg::RDI, buffer1);
        code.mov_r32_imm(Reg::RCX, 4);
        switch (program) {
        case StringProgram::Movsb:
            code.movsb();
            break;
        case StringProgram::Movsw:
            code.movsw();
            break;
        case StringProgram::Movsd:
            code.movsd();
            break;
        case StringProgram::Movsq:
            code.movsq();
            break;
        case StringProgram::RepMovsb:
            code.mov_r32_imm(Reg::RCX, 12);
            code.rep();
            code.movsb();
            break;
        case StringProgram::RepMovsw:
            code.mov_r32_imm(Reg::RCX, 6);
            code.rep();
            code.movsw();
            break;
        case StringProgram::Lodsb:
            code.lodsb();
            break;
        case StringProgram::Lodsq:
            code.lodsq();
            break;
        case StringProgram::Stosb:
            code.mov_r8_imm(Reg::RAX, narrow8(seeded(seed, 0x700)));
            code.stosb();
            break;
        case StringProgram::Stosq:
            code.stosq();
            break;
        case StringProgram::Cmpsb:
            code.cmpsb();
            break;
        case StringProgram::Scasb:
            code.mov_r8_imm(Reg::RAX, data[kSlotSize * 4 + 5]);
            code.scasb();
            break;
        }
        break;
    }
    case Family::VectorOps: {
        const auto program = static_cast<VectorProgram>(spec.op);
        code.movdqu_load(Reg::RAX, slot0);
        code.movdqu_load(Reg::RCX, slot1);
        switch (program) {
        case VectorProgram::Paddq:
            code.paddq(Reg::RAX, Reg::RCX);
            code.movdqu_store(buffer0, Reg::RAX);
            break;
        case VectorProgram::Pxor:
            code.pxor(Reg::RAX, Reg::RCX);
            code.movdqu_store(buffer0, Reg::RAX);
            break;
        case VectorProgram::Pand:
            code.pand(Reg::RAX, Reg::RCX);
            code.movdqu_store(buffer0, Reg::RAX);
            break;
        case VectorProgram::Por:
            code.por(Reg::RAX, Reg::RCX);
            code.movdqu_store(buffer0, Reg::RAX);
            break;
        case VectorProgram::Pcmpeqb:
            code.pcmpeqb(Reg::RAX, Reg::RCX);
            code.movdqu_store(buffer0, Reg::RAX);
            break;
        case VectorProgram::Pshufd:
            code.pshufd(Reg::RAX, Reg::RAX, 0x1Bu);
            code.movdqu_store(buffer0, Reg::RAX);
            break;
        case VectorProgram::Pslldq:
            code.pslldq(Reg::RAX, 4);
            code.movdqu_store(buffer0, Reg::RAX);
            break;
        case VectorProgram::Psrldq:
            code.psrldq(Reg::RAX, 5);
            code.movdqu_store(buffer0, Reg::RAX);
            break;
        case VectorProgram::PaddqPxor:
            code.paddq(Reg::RAX, Reg::RCX);
            code.pxor(Reg::RAX, Reg::RCX);
            code.movdqu_store(buffer0, Reg::RAX);
            break;
        case VectorProgram::Movdqu:
            code.movdqu_store(buffer0, Reg::RAX);
            code.movdqu_load(Reg::RDX, buffer0);
            code.movdqu_store(buffer1, Reg::RDX);
            break;
        case VectorProgram::Pshufb:
            code.pshufb(Reg::RAX, Reg::RCX);
            code.movdqu_store(buffer0, Reg::RAX);
            break;
        case VectorProgram::Aesenc:
            code.aesenc(Reg::RAX, Reg::RCX);
            code.movdqu_store(buffer0, Reg::RAX);
            break;
        case VectorProgram::Aesenclast:
            code.aesenclast(Reg::RAX, Reg::RCX);
            code.movdqu_store(buffer0, Reg::RAX);
            break;
        case VectorProgram::Aeskeygenassist:
            code.aeskeygenassist(Reg::RAX, Reg::RCX, kAesKeygenAssistImmediates[spec.variant % kAesKeygenAssistImmediates.size()]);
            code.movdqu_store(buffer0, Reg::RAX);
            break;
        case VectorProgram::Roundsd:
            code.roundsd(Reg::RAX, Reg::RCX, kRoundImmediates[spec.variant % kRoundImmediates.size()]);
            code.movdqu_store(buffer0, Reg::RAX);
            break;
        case VectorProgram::Roundss:
            code.roundss(Reg::RAX, Reg::RCX, kRoundImmediates[spec.variant % kRoundImmediates.size()]);
            code.movdqu_store(buffer0, Reg::RAX);
            break;
        }
        break;
    }
    case Family::ControlFlow: {
        switch (static_cast<ControlProgram>(spec.op)) {
        case ControlProgram::JmpSkip:
            code.jmp32(label_end);
            code.mov_reg_reg(Reg::RAX, Reg::R15);
            code.mark(label_end);
            code.binary_reg_reg(BinaryOp::Add, Reg::RAX, Reg::RBX);
            break;
        case ControlProgram::CallAdd:
            code.call32(helper);
            code.mov_reg_reg(Reg::R8, Reg::RAX);
            code.jmp32(after_helper);
            code.mark(helper);
            code.binary_reg_reg(BinaryOp::Add, Reg::RAX, Reg::RBX);
            code.ret();
            code.mark(after_helper);
            break;
        case ControlProgram::CallXor:
            code.call32(helper);
            code.mov_reg_reg(Reg::R9, Reg::R10);
            code.jmp32(after_helper);
            code.mark(helper);
            code.binary_reg_reg(BinaryOp::Xor, Reg::R10, Reg::R11);
            code.ret();
            code.mark(after_helper);
            break;
        case ControlProgram::CallLea:
            code.call32(helper);
            code.mov_reg_reg(Reg::R12, Reg::R14);
            code.jmp32(after_helper);
            code.mark(helper);
            code.lea_scaled(Reg::R14, Reg::RAX, Reg::RBX, 1, 0x10);
            code.ret();
            code.mark(after_helper);
            break;
        }
        break;
    }
    }

    code.ret();
    code.align(16);
    built.data_offset = static_cast<std::uint32_t>(code.offset());
    code.mark(slot0);
    code.emit_bytes(data.data(), kSlotSize);
    code.mark(slot1);
    code.emit_bytes(data.data() + kSlotSize, kSlotSize);
    code.mark(slot2);
    code.emit_bytes(data.data() + kSlotSize * 2, kSlotSize);
    code.mark(slot3);
    code.emit_bytes(data.data() + kSlotSize * 3, kSlotSize);
    code.mark(buffer0);
    code.emit_bytes(data.data() + kSlotSize * 4, kBufferSize);
    code.mark(buffer1);
    code.emit_bytes(data.data() + kSlotSize * 4 + kBufferSize, kBufferSize);
    if (!code.finalize()) {
        built.image.clear();
        return built;
    }
    built.image = code.bytes();
    return built;
}

inline bool write_engine_reg(cpueaxh_engine* engine, int reg, std::uint64_t value) {
    return cpueaxh_reg_write(engine, reg, &value) == CPUEAXH_ERR_OK;
}

inline bool read_engine_reg(cpueaxh_engine* engine, int reg, std::uint64_t& value) {
    return cpueaxh_reg_read(engine, reg, &value) == CPUEAXH_ERR_OK;
}

inline bool initialize_manual_engine(cpueaxh_engine* engine, const std::vector<std::uint8_t>& code, const cpueaxh_x86_context& initial, std::uint64_t guest_rsp, Failure& failure, const std::string& name) {
    cpueaxh_err err = cpueaxh_set_memory_mode(engine, CPUEAXH_MEMORY_MODE_GUEST);
    if (err != CPUEAXH_ERR_OK) {
        failure.case_name = name;
        failure.detail = "cpueaxh_set_memory_mode failed";
        return false;
    }
    err = cpueaxh_mem_map(engine, kGuestCodeBase, kCodePageSize, CPUEAXH_PROT_READ | CPUEAXH_PROT_WRITE | CPUEAXH_PROT_EXEC);
    if (err != CPUEAXH_ERR_OK) {
        failure.case_name = name;
        failure.detail = "cpueaxh_mem_map code failed";
        return false;
    }
    err = cpueaxh_mem_map(engine, kGuestStackBase, kStackSize, CPUEAXH_PROT_READ | CPUEAXH_PROT_WRITE);
    if (err != CPUEAXH_ERR_OK) {
        failure.case_name = name;
        failure.detail = "cpueaxh_mem_map stack failed";
        return false;
    }
    err = cpueaxh_mem_write(engine, kGuestCodeBase, code.data(), code.size());
    if (err != CPUEAXH_ERR_OK) {
        failure.case_name = name;
        failure.detail = "cpueaxh_mem_write failed";
        return false;
    }

    if (!write_engine_reg(engine, CPUEAXH_X86_REG_RAX, initial.regs[0]) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_RCX, initial.regs[1]) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_RDX, initial.regs[2]) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_RBX, initial.regs[3]) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_RSP, guest_rsp) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_RBP, initial.regs[5]) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_RSI, initial.regs[6]) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_RDI, initial.regs[7]) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_R8, initial.regs[8]) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_R9, initial.regs[9]) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_R10, initial.regs[10]) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_R11, initial.regs[11]) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_R12, initial.regs[12]) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_R13, initial.regs[13]) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_R14, initial.regs[14]) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_R15, initial.regs[15]) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_RIP, kGuestCodeBase) ||
        !write_engine_reg(engine, CPUEAXH_X86_REG_EFLAGS, initial.rflags)) {
        failure.case_name = name;
        failure.detail = "register initialization failed";
        return false;
    }

    return true;
}

inline bool initialize_manual_cpu_context(CPU_CONTEXT& context, MEMORY_MANAGER& memory_manager, const std::vector<std::uint8_t>& code, const cpueaxh_x86_context& initial, std::uint64_t guest_rsp, Failure& failure, const std::string& name) {
    mm_init(&memory_manager);
    init_cpu_context(&context, &memory_manager);

    if (!mm_map_internal(&memory_manager, kGuestCodeBase, kCodePageSize, MM_PROT_READ | MM_PROT_WRITE | MM_PROT_EXEC)) {
        failure.case_name = name;
        failure.detail = "internal code map failed";
        return false;
    }
    if (!mm_map_internal(&memory_manager, kGuestStackBase, kStackSize, MM_PROT_READ | MM_PROT_WRITE)) {
        failure.case_name = name;
        failure.detail = "internal stack map failed";
        return false;
    }

    for (std::size_t index = 0; index < code.size(); ++index) {
        if (mm_write_byte_checked(&memory_manager, kGuestCodeBase + index, code[index]) != MM_ACCESS_OK) {
            failure.case_name = name;
            failure.detail = "internal code write failed";
            return false;
        }
    }

    for (std::size_t index = 0; index < 16; ++index) {
        context.regs[index] = initial.regs[index];
    }
    context.regs[REG_RSP] = guest_rsp;
    context.rip = kGuestCodeBase;
    context.rflags = initial.rflags;
    context.mxcsr = static_cast<std::uint32_t>(initial.mxcsr);
    return true;
}

inline bool run_manual_special_case(
    const std::string& name,
    const std::vector<std::uint8_t>& code,
    std::uint64_t seed,
    std::uint32_t processor_id,
    bool expect_rdpid,
    Failure& failure) {
    cpueaxh_engine* engine = nullptr;
    cpueaxh_err err = cpueaxh_open(CPUEAXH_ARCH_X86, CPUEAXH_MODE_64, &engine);
    if (err != CPUEAXH_ERR_OK) {
        failure.case_name = name;
        failure.detail = "cpueaxh_open failed";
        return false;
    }

    bool ok = false;
    do {
        cpueaxh_x86_context initial = make_initial_context(seed);
        const std::uint64_t guest_rsp = kGuestStackBase + kInitialRspOffset;
        if (!initialize_manual_engine(engine, code, initial, guest_rsp, failure, name)) {
            break;
        }

        cpueaxh_set_processor_id(engine, processor_id);

        err = cpueaxh_emu_start_function(engine, kGuestCodeBase, 0, 1000);
        if (err != CPUEAXH_ERR_OK) {
            failure.case_name = name;
            failure.detail = "execution failed";
            break;
        }
        if (cpueaxh_code_exception(engine) != CPUEAXH_EXCEPTION_NONE) {
            failure.case_name = name;
            failure.detail = "unexpected exception";
            break;
        }

        std::array<std::uint64_t, 16> regs{};
        for (std::size_t index = 0; index < regs.size(); ++index) {
            if (!read_engine_reg(engine, static_cast<int>(index), regs[index])) {
                failure.case_name = name;
                failure.detail = "register read failed";
                goto cleanup;
            }
        }
        std::uint64_t flags = 0;
        if (!read_engine_reg(engine, CPUEAXH_X86_REG_EFLAGS, flags)) {
            failure.case_name = name;
            failure.detail = "flags read failed";
            break;
        }

        for (std::size_t index = 1; index < regs.size(); ++index) {
            if (index == static_cast<std::size_t>(Reg::RSP)) {
                continue;
            }
            if (regs[index] != initial.regs[index]) {
                failure.case_name = name;
                failure.detail = std::string(reg_name(static_cast<Reg>(index))) + " changed unexpectedly";
                goto cleanup;
            }
        }
        if ((flags & kStatusMask) != (initial.rflags & kStatusMask)) {
            failure.case_name = name;
            failure.detail = "flags changed unexpectedly";
            break;
        }
        if (expect_rdpid) {
            if (regs[0] != processor_id) {
                failure.case_name = name;
                failure.detail = "rdpid result mismatch";
                break;
            }
        }
        else if (regs[0] != initial.regs[0]) {
            failure.case_name = name;
            failure.detail = "rax changed unexpectedly";
            break;
        }

        ok = true;
    } while (false);

cleanup:
    cpueaxh_close(engine);
    return ok;
}

inline bool run_manual_exception_case(
    const std::string& name,
    const std::vector<std::uint8_t>& code,
    std::uint64_t seed,
    std::uint32_t expected_exception,
    Failure& failure) {
    MEMORY_MANAGER memory_manager = {};
    CPU_CONTEXT context = {};
    bool ok = false;
    do {
        cpueaxh_x86_context initial = make_initial_context(seed);
        const std::uint64_t guest_rsp = kGuestStackBase + kInitialRspOffset;
        std::vector<std::uint8_t> image = code;
        image.push_back(0x90);
        image.push_back(0x90);
        if (!initialize_manual_cpu_context(context, memory_manager, image, initial, guest_rsp, failure, name)) {
            break;
        }

        const int status = cpu_step(&context);
        if (status != kCpuStepException) {
            failure.case_name = name;
            failure.detail = "expected CPU_STEP_EXCEPTION";
            break;
        }
        if (context.exception.code != expected_exception) {
            failure.case_name = name;
            failure.detail = "unexpected exception code";
            break;
        }

        if (context.regs[REG_RAX] != initial.regs[0]) {
            failure.case_name = name;
            failure.detail = "rax changed unexpectedly";
            break;
        }
        if ((context.rflags & kStatusMask) != (initial.rflags & kStatusMask)) {
            failure.case_name = name;
            failure.detail = "flags changed unexpectedly";
            break;
        }

        cpu_clear_exception(&context);
        context.rip = kGuestCodeBase + static_cast<std::uint64_t>(code.size());

        const int resume0 = cpu_step(&context);
        if (resume0 != kCpuStepOk || cpu_has_exception(&context)) {
            failure.case_name = name;
            failure.detail = "resume step 0 failed";
            break;
        }
        const std::uint64_t expected_rip_after_resume0 = kGuestCodeBase + static_cast<std::uint64_t>(code.size()) + 1;
        if (context.rip != expected_rip_after_resume0) {
            failure.case_name = name;
            failure.detail = "resume rip 0 mismatch";
            break;
        }

        const int resume1 = cpu_step(&context);
        if (resume1 != kCpuStepOk || cpu_has_exception(&context)) {
            failure.case_name = name;
            failure.detail = "resume step 1 failed";
            break;
        }
        const std::uint64_t expected_rip_after_resume1 = expected_rip_after_resume0 + 1;
        if (context.rip != expected_rip_after_resume1) {
            failure.case_name = name;
            failure.detail = "resume rip 1 mismatch";
            break;
        }
        ok = true;
    } while (false);

    mm_destroy(&memory_manager);
    return ok;
}

inline bool run_manual_exception_case_public(
    const std::string& name,
    const std::vector<std::uint8_t>& code,
    std::uint64_t seed,
    std::uint32_t expected_exception,
    Failure& failure) {
    cpueaxh_engine* engine = nullptr;
    cpueaxh_err err = cpueaxh_open(CPUEAXH_ARCH_X86, CPUEAXH_MODE_64, &engine);
    if (err != CPUEAXH_ERR_OK) {
        failure.case_name = name;
        failure.detail = "cpueaxh_open failed";
        return false;
    }

    bool ok = false;
    do {
        cpueaxh_x86_context initial = make_initial_context(seed);
        const std::uint64_t guest_rsp = kGuestStackBase + kInitialRspOffset;
        if (!initialize_manual_engine(engine, code, initial, guest_rsp, failure, name)) {
            break;
        }

        err = cpueaxh_emu_start(engine, kGuestCodeBase, 0, 0, 1);
        if (err != CPUEAXH_ERR_EXCEPTION) {
            failure.case_name = name;
            failure.detail = "expected CPUEAXH_ERR_EXCEPTION";
            break;
        }
        if (cpueaxh_code_exception(engine) != expected_exception) {
            failure.case_name = name;
            failure.detail = "unexpected exception code";
            break;
        }
        ok = true;
    } while (false);

    cpueaxh_close(engine);
    return ok;
}

inline std::uint64_t manual_special_case_count(const HostFeatures& features) {
    (void)features;
    return kSeedCount * 6ull + kExceptionSeedCount * 8ull;
}

inline bool run_manual_special_tests(const HostFeatures& features, std::uint64_t& executed, std::uint64_t total) {
    const std::vector<std::uint8_t> endbr64 = { 0xF3, 0x0F, 0x1E, 0xFA, 0xC3 };
    const std::vector<std::uint8_t> endbr32 = { 0xF3, 0x0F, 0x1E, 0xFB, 0xC3 };
    const std::vector<std::uint8_t> rdsspq = { 0xF3, 0x48, 0x0F, 0x1E, 0xC8, 0xC3 };
    const std::vector<std::uint8_t> rdsspd = { 0xF3, 0x0F, 0x1E, 0xC8, 0xC3 };
    const std::vector<std::uint8_t> rdpid64 = { 0xF3, 0x48, 0x0F, 0xC7, 0xF8, 0xC3 };
    const std::vector<std::uint8_t> rdpid32 = { 0xF3, 0x0F, 0xC7, 0xF8, 0xC3 };
    const std::vector<std::uint8_t> invalid_endbr_lock = { 0xF0, 0xF3, 0x0F, 0x1E, 0xFA };
    const std::vector<std::uint8_t> invalid_rdssp_lock = { 0xF0, 0xF3, 0x48, 0x0F, 0x1E, 0xC8 };
    const std::vector<std::uint8_t> invalid_rdpid_no_f3 = { 0x48, 0x0F, 0xC7, 0xF8 };
    const std::vector<std::uint8_t> invalid_rdpid_with_66 = { 0x66, 0xF3, 0x48, 0x0F, 0xC7, 0xF8 };

    auto tick = [&](bool result, const Failure& failure) -> bool {
        if (!result) {
            std::cerr << "FAIL " << failure.case_name << std::endl;
            std::cerr << failure.detail << std::endl;
            return false;
        }
        ++executed;
        if ((executed % 1024) == 0 || executed == total) {
            std::cout << "progress " << executed << "/" << total << std::endl;
        }
        return true;
    };

    for (std::uint64_t seed_index = 0; seed_index < kSeedCount; ++seed_index) {
        Failure failure;
        const std::uint64_t seed0 = seeded(seed_index, 0xE001);
        if (!tick(run_manual_special_case("endbr64:" + std::to_string(seed0), endbr64, seed0, 0, false, failure), failure)) return false;

        const std::uint64_t seed_endbr32 = seeded(seed_index, 0xE011);
        if (!tick(run_manual_special_case("endbr32:" + std::to_string(seed_endbr32), endbr32, seed_endbr32, 0, false, failure), failure)) return false;

        const std::uint64_t seed1 = seeded(seed_index, 0xE002);
        if (!tick(run_manual_special_case("rdsspq:" + std::to_string(seed1), rdsspq, seed1, 0, false, failure), failure)) return false;

        const std::uint64_t seed_rdsspd = seeded(seed_index, 0xE012);
        if (!tick(run_manual_special_case("rdsspd:" + std::to_string(seed_rdsspd), rdsspd, seed_rdsspd, 0, false, failure), failure)) return false;

        const std::uint64_t seed2 = seeded(seed_index, 0xE003);
        const std::uint32_t processor_id = static_cast<std::uint32_t>(seeded(seed2, 0x90));
        if (!tick(run_manual_special_case("rdpid64:" + std::to_string(seed2), rdpid64, seed2, processor_id, true, failure), failure)) return false;

        const std::uint64_t seed_rdpid32 = seeded(seed_index, 0xE013);
        const std::uint32_t processor_id32 = static_cast<std::uint32_t>(seeded(seed_rdpid32, 0x91));
        if (!tick(run_manual_special_case("rdpid32:" + std::to_string(seed_rdpid32), rdpid32, seed_rdpid32, processor_id32, true, failure), failure)) return false;
    }

    for (std::uint64_t seed_index = 0; seed_index < kExceptionSeedCount; ++seed_index) {
        Failure failure;
        const std::uint64_t seed3 = seeded(seed_index, 0xE004);
        if (!tick(run_manual_exception_case_public("public_endbr_lock_ud:" + std::to_string(seed3), invalid_endbr_lock, seed3, CPUEAXH_EXCEPTION_UD, failure), failure)) return false;
        if (!tick(run_manual_exception_case("endbr_lock_ud:" + std::to_string(seed3), invalid_endbr_lock, seed3, CPUEAXH_EXCEPTION_UD, failure), failure)) return false;

        const std::uint64_t seed4 = seeded(seed_index, 0xE005);
        if (!tick(run_manual_exception_case_public("public_rdssp_lock_ud:" + std::to_string(seed4), invalid_rdssp_lock, seed4, CPUEAXH_EXCEPTION_UD, failure), failure)) return false;
        if (!tick(run_manual_exception_case("rdssp_lock_ud:" + std::to_string(seed4), invalid_rdssp_lock, seed4, CPUEAXH_EXCEPTION_UD, failure), failure)) return false;

        const std::uint64_t seed5 = seeded(seed_index, 0xE006);
        if (!tick(run_manual_exception_case_public("public_rdpid_no_f3_ud:" + std::to_string(seed5), invalid_rdpid_no_f3, seed5, CPUEAXH_EXCEPTION_UD, failure), failure)) return false;
        if (!tick(run_manual_exception_case("rdpid_no_f3_ud:" + std::to_string(seed5), invalid_rdpid_no_f3, seed5, CPUEAXH_EXCEPTION_UD, failure), failure)) return false;

        const std::uint64_t seed6 = seeded(seed_index, 0xE007);
        if (!tick(run_manual_exception_case_public("public_rdpid_66_ud:" + std::to_string(seed6), invalid_rdpid_with_66, seed6, CPUEAXH_EXCEPTION_UD, failure), failure)) return false;
        if (!tick(run_manual_exception_case("rdpid_66_ud:" + std::to_string(seed6), invalid_rdpid_with_66, seed6, CPUEAXH_EXCEPTION_UD, failure), failure)) return false;
    }

    return true;
}

class Harness {
public:
    Harness() {
        native_code_ = static_cast<std::uint8_t*>(::VirtualAlloc(nullptr, kCodePageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
        native_stack_ = static_cast<std::uint8_t*>(::VirtualAlloc(nullptr, kStackSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        cpueaxh_err err = cpueaxh_open(CPUEAXH_ARCH_X86, CPUEAXH_MODE_64, &engine_);
        if (err != CPUEAXH_ERR_OK) {
            init_error_ = std::string("cpueaxh_open failed: ") + std::to_string(err);
            return;
        }
        err = cpueaxh_set_memory_mode(engine_, CPUEAXH_MEMORY_MODE_GUEST);
        if (err != CPUEAXH_ERR_OK) {
            init_error_ = std::string("cpueaxh_set_memory_mode failed: ") + std::to_string(err);
            return;
        }
        err = cpueaxh_mem_map(engine_, kGuestCodeBase, kCodePageSize, CPUEAXH_PROT_READ | CPUEAXH_PROT_WRITE | CPUEAXH_PROT_EXEC);
        if (err != CPUEAXH_ERR_OK) {
            init_error_ = std::string("cpueaxh_mem_map code failed: ") + std::to_string(err);
            return;
        }
        err = cpueaxh_mem_map(engine_, kGuestStackBase, kStackSize, CPUEAXH_PROT_READ | CPUEAXH_PROT_WRITE);
        if (err != CPUEAXH_ERR_OK) {
            init_error_ = std::string("cpueaxh_mem_map stack failed: ") + std::to_string(err);
            return;
        }
        ready_ = native_code_ != nullptr && native_stack_ != nullptr;
        if (!ready_ && init_error_.empty()) {
            init_error_ = "memory allocation failed";
        }
    }

    ~Harness() {
        if (engine_) {
            cpueaxh_close(engine_);
        }
        if (native_code_) {
            ::VirtualFree(native_code_, 0, MEM_RELEASE);
        }
        if (native_stack_) {
            ::VirtualFree(native_stack_, 0, MEM_RELEASE);
        }
    }

    bool ready() const {
        return ready_;
    }

    const std::string& init_error() const {
        return init_error_;
    }

    bool run_case(const BuiltCase& built, Failure& failure) {
        if (built.image.empty() || built.image.size() > kCodePageSize) {
            failure.case_name = built.spec.name;
            failure.detail = "code generation failed";
            return false;
        }

        std::memset(native_code_, 0, kCodePageSize);
        std::memset(native_stack_, 0, kStackSize);
        std::memcpy(native_code_, built.image.data(), built.image.size());

        const std::uint64_t guest_rsp = kGuestStackBase + kInitialRspOffset;
        const std::uint64_t native_rsp = reinterpret_cast<std::uint64_t>(native_stack_) + kInitialRspOffset;

        cpueaxh_x86_context native_context = built.initial_context;
        native_context.regs[static_cast<std::size_t>(Reg::RSP)] = native_rsp;
        native_context.rip = reinterpret_cast<std::uint64_t>(native_code_);
        native_context.rflags = built.initial_context.rflags;
        native_context.mxcsr = static_cast<std::uint32_t>(kInitialMxcsr);

        TestBridgeBlock block{};
        if (cpueaxh_test_run_native(&native_context, native_code_, &block) != 0) {
            failure.case_name = built.spec.name + ":" + std::to_string(built.seed);
            failure.detail = "native execution failed";
            return false;
        }

        static const std::vector<std::uint8_t> zero_code(kCodePageSize, 0);
        static const std::vector<std::uint8_t> zero_stack(kStackSize, 0);
        cpueaxh_err err = cpueaxh_mem_write(engine_, kGuestCodeBase, zero_code.data(), zero_code.size());
        if (err != CPUEAXH_ERR_OK) {
            failure.case_name = built.spec.name;
            failure.detail = "guest zero code failed";
            return false;
        }
        err = cpueaxh_mem_write(engine_, kGuestStackBase, zero_stack.data(), zero_stack.size());
        if (err != CPUEAXH_ERR_OK) {
            failure.case_name = built.spec.name;
            failure.detail = "guest zero stack failed";
            return false;
        }
        err = cpueaxh_mem_write(engine_, kGuestCodeBase, built.image.data(), built.image.size());
        if (err != CPUEAXH_ERR_OK) {
            failure.case_name = built.spec.name;
            failure.detail = "guest write code failed";
            return false;
        }

        if (!write_reg(CPUEAXH_X86_REG_RAX, built.initial_context.regs[0])) return fail_reg_write(failure, built, "rax");
        if (!write_reg(CPUEAXH_X86_REG_RCX, built.initial_context.regs[1])) return fail_reg_write(failure, built, "rcx");
        if (!write_reg(CPUEAXH_X86_REG_RDX, built.initial_context.regs[2])) return fail_reg_write(failure, built, "rdx");
        if (!write_reg(CPUEAXH_X86_REG_RBX, built.initial_context.regs[3])) return fail_reg_write(failure, built, "rbx");
        if (!write_reg(CPUEAXH_X86_REG_RSP, guest_rsp)) return fail_reg_write(failure, built, "rsp");
        if (!write_reg(CPUEAXH_X86_REG_RBP, built.initial_context.regs[5])) return fail_reg_write(failure, built, "rbp");
        if (!write_reg(CPUEAXH_X86_REG_RSI, built.initial_context.regs[6])) return fail_reg_write(failure, built, "rsi");
        if (!write_reg(CPUEAXH_X86_REG_RDI, built.initial_context.regs[7])) return fail_reg_write(failure, built, "rdi");
        if (!write_reg(CPUEAXH_X86_REG_R8, built.initial_context.regs[8])) return fail_reg_write(failure, built, "r8");
        if (!write_reg(CPUEAXH_X86_REG_R9, built.initial_context.regs[9])) return fail_reg_write(failure, built, "r9");
        if (!write_reg(CPUEAXH_X86_REG_R10, built.initial_context.regs[10])) return fail_reg_write(failure, built, "r10");
        if (!write_reg(CPUEAXH_X86_REG_R11, built.initial_context.regs[11])) return fail_reg_write(failure, built, "r11");
        if (!write_reg(CPUEAXH_X86_REG_R12, built.initial_context.regs[12])) return fail_reg_write(failure, built, "r12");
        if (!write_reg(CPUEAXH_X86_REG_R13, built.initial_context.regs[13])) return fail_reg_write(failure, built, "r13");
        if (!write_reg(CPUEAXH_X86_REG_R14, built.initial_context.regs[14])) return fail_reg_write(failure, built, "r14");
        if (!write_reg(CPUEAXH_X86_REG_R15, built.initial_context.regs[15])) return fail_reg_write(failure, built, "r15");
        if (!write_reg(CPUEAXH_X86_REG_RIP, kGuestCodeBase)) return fail_reg_write(failure, built, "rip");
        if (!write_reg(CPUEAXH_X86_REG_EFLAGS, built.initial_context.rflags)) return fail_reg_write(failure, built, "rflags");

        err = cpueaxh_emu_start_function(engine_, kGuestCodeBase, 0, 100000);
        if (err != CPUEAXH_ERR_OK) {
            failure.case_name = built.spec.name + ":" + std::to_string(built.seed);
            failure.detail = std::string("guest execution failed: ") + std::to_string(err) + " exc=" + std::to_string(cpueaxh_code_exception(engine_));
            return false;
        }

        std::array<std::uint64_t, 16> guest_regs{};
        for (std::size_t index = 0; index < guest_regs.size(); ++index) {
            if (!read_reg(static_cast<int>(index), guest_regs[index])) {
                failure.case_name = built.spec.name + ":" + std::to_string(built.seed);
                failure.detail = "guest read register failed";
                return false;
            }
        }
        std::uint64_t guest_flags = 0;
        if (!read_reg(CPUEAXH_X86_REG_EFLAGS, guest_flags)) {
            failure.case_name = built.spec.name + ":" + std::to_string(built.seed);
            failure.detail = "guest read flags failed";
            return false;
        }

        std::vector<std::uint8_t> guest_data(kDataSize);
        err = cpueaxh_mem_read(engine_, kGuestCodeBase + built.data_offset, guest_data.data(), guest_data.size());
        if (err != CPUEAXH_ERR_OK) {
            failure.case_name = built.spec.name + ":" + std::to_string(built.seed);
            failure.detail = "guest read data failed";
            return false;
        }

        const std::uint8_t* native_data = native_code_ + built.data_offset;
        for (std::size_t index = 0; index < 16; ++index) {
            const std::uint64_t guest_value = guest_regs[index];
            const std::uint64_t native_value = normalize_native_value(native_context.regs[index], built);
            if (guest_value != native_value) {
                failure.case_name = built.spec.name + ":" + std::to_string(built.seed);
                failure.detail = std::string(reg_name(static_cast<Reg>(index))) + " guest=" + hex64(guest_value) + " native=" + hex64(native_value);
                return false;
            }
        }

        const std::uint64_t guest_masked = guest_flags & built.spec.flag_mask;
        const std::uint64_t native_masked = native_context.rflags & built.spec.flag_mask;
        if (guest_masked != native_masked) {
            failure.case_name = built.spec.name + ":" + std::to_string(built.seed);
            failure.detail = std::string("flags guest=") + hex64(guest_masked) + " native=" + hex64(native_masked);
            return false;
        }

        for (std::size_t index = 0; index < guest_data.size(); ++index) {
            if (guest_data[index] != native_data[index]) {
                failure.case_name = built.spec.name + ":" + std::to_string(built.seed);
                failure.detail = std::string("data[") + std::to_string(index) + "] guest=" + hex8(guest_data[index]) + " native=" + hex8(native_data[index]);
                return false;
            }
        }

        return true;
    }

private:
    std::uint64_t normalize_native_value(std::uint64_t value, const BuiltCase& built) const {
        const std::uint64_t native_code_begin = reinterpret_cast<std::uint64_t>(native_code_);
        const std::uint64_t native_code_end = native_code_begin + kCodePageSize;
        if (value >= native_code_begin && value < native_code_end) {
            return kGuestCodeBase + (value - native_code_begin);
        }

        const std::uint64_t native_stack_begin = reinterpret_cast<std::uint64_t>(native_stack_);
        const std::uint64_t native_stack_end = native_stack_begin + kStackSize;
        if (value >= native_stack_begin && value < native_stack_end) {
            return kGuestStackBase + (value - native_stack_begin);
        }

        (void)built;
        return value;
    }

    bool write_reg(int reg, std::uint64_t value) {
        return cpueaxh_reg_write(engine_, reg, &value) == CPUEAXH_ERR_OK;
    }

    bool read_reg(int reg, std::uint64_t& value) {
        return cpueaxh_reg_read(engine_, reg, &value) == CPUEAXH_ERR_OK;
    }

    bool fail_reg_write(Failure& failure, const BuiltCase& built, const char* name) {
        failure.case_name = built.spec.name + ":" + std::to_string(built.seed);
        failure.detail = std::string("guest write ") + name + " failed";
        return false;
    }

    cpueaxh_engine* engine_ = nullptr;
    std::uint8_t* native_code_ = nullptr;
    std::uint8_t* native_stack_ = nullptr;
    bool ready_ = false;
    std::string init_error_;
};

inline bool run_all_tests() {
    const HostFeatures features = query_host_features();
    const std::vector<ProgramSpec> specs = make_specs(features);
    const std::uint64_t total = static_cast<std::uint64_t>(specs.size()) * kSeedCount + manual_special_case_count(features);
    std::cout << "cpueaxh test cases: " << total << std::endl;

    Harness harness;
    if (!harness.ready()) {
        std::cerr << harness.init_error() << std::endl;
        return false;
    }

    std::uint64_t executed = 0;
    for (const ProgramSpec& spec : specs) {
        for (std::uint64_t seed_index = 0; seed_index < kSeedCount; ++seed_index) {
            const std::uint64_t seed = seeded(seed_index, static_cast<std::uint64_t>(spec.op) + (static_cast<std::uint64_t>(spec.family) << 8) + (static_cast<std::uint64_t>(spec.variant) << 16));
            BuiltCase built = build_case(spec, seed);
            Failure failure;
            if (!harness.run_case(built, failure)) {
                std::cerr << "FAIL " << failure.case_name << std::endl;
                std::cerr << failure.detail << std::endl;
                return false;
            }
            ++executed;
            if ((executed % 1024) == 0 || executed == total) {
                std::cout << "progress " << executed << "/" << total << std::endl;
            }
        }
    }

    if (!run_manual_special_tests(features, executed, total)) {
        return false;
    }

    std::cout << "PASS " << executed << "/" << total << std::endl;
    return true;
}

}