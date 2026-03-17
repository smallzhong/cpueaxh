constexpr uint64_t GUEST_MEM_INVALID_CODE_BASE = 0x120000;
constexpr uint64_t GUEST_MEM_INVALID_STACK_BASE = 0x220000;
constexpr uint64_t GUEST_MEM_INVALID_STACK_SIZE = 0x1000;
constexpr uint64_t GUEST_MEM_INVALID_SRC_BASE = 0x320000;
constexpr uint64_t GUEST_MEM_INVALID_DST_BASE = 0x330000;
constexpr uint64_t GUEST_MEM_INVALID_EXPECTED_SOURCE = 0x1122334455667788ull;
constexpr uint64_t GUEST_MEM_INVALID_EXPECTED_RESULT = GUEST_MEM_INVALID_EXPECTED_SOURCE + 5ull;
constexpr uint8_t GUEST_MEM_INVALID_CODE[] = {
    0x48, 0xBB, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x48, 0xB9, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x48, 0x8B, 0x03,
    0x48, 0x83, 0xC0, 0x05,
    0x48, 0x89, 0x01,
    0x90,
};

struct GuestInvalidMemHookState {
    int unmapped_read_hits;
    int write_prot_hits;
};

inline bool setup_guest_invalid_memory_hook_demo(cpueaxh_engine** out_engine) {
    cpueaxh_engine* engine = 0;
    cpueaxh_err err = cpueaxh_open(CPUEAXH_ARCH_X86, CPUEAXH_MODE_64, &engine);
    if (err != CPUEAXH_ERR_OK) {
        std::cerr << "cpueaxh_open failed: " << cpueaxh_err_string(err)
            << " (" << err << ")" << std::endl;
        return false;
    }

    err = cpueaxh_set_memory_mode(engine, CPUEAXH_MEMORY_MODE_GUEST);
    if (err != CPUEAXH_ERR_OK) {
        std::cerr << "cpueaxh_set_memory_mode(guest) failed: " << cpueaxh_err_string(err)
            << " (" << err << ")" << std::endl;
        cpueaxh_close(engine);
        return false;
    }

    err = cpueaxh_mem_map(engine, GUEST_MEM_INVALID_CODE_BASE, 0x1000, CPUEAXH_PROT_READ | CPUEAXH_PROT_WRITE | CPUEAXH_PROT_EXEC);
    if (err != CPUEAXH_ERR_OK) {
        std::cerr << "cpueaxh_mem_map(code) failed: " << cpueaxh_err_string(err)
            << " (" << err << ")" << std::endl;
        cpueaxh_close(engine);
        return false;
    }

    err = cpueaxh_mem_write(engine, GUEST_MEM_INVALID_CODE_BASE, GUEST_MEM_INVALID_CODE, sizeof(GUEST_MEM_INVALID_CODE));
    if (err != CPUEAXH_ERR_OK) {
        std::cerr << "cpueaxh_mem_write(code) failed: " << cpueaxh_err_string(err)
            << " (" << err << ")" << std::endl;
        cpueaxh_close(engine);
        return false;
    }

    err = cpueaxh_mem_map(engine, GUEST_MEM_INVALID_STACK_BASE, GUEST_MEM_INVALID_STACK_SIZE, CPUEAXH_PROT_READ | CPUEAXH_PROT_WRITE);
    if (err != CPUEAXH_ERR_OK) {
        std::cerr << "cpueaxh_mem_map(stack) failed: " << cpueaxh_err_string(err)
            << " (" << err << ")" << std::endl;
        cpueaxh_close(engine);
        return false;
    }

    err = cpueaxh_mem_map(engine, GUEST_MEM_INVALID_DST_BASE, 0x1000, CPUEAXH_PROT_READ);
    if (err != CPUEAXH_ERR_OK) {
        std::cerr << "cpueaxh_mem_map(dst) failed: " << cpueaxh_err_string(err)
            << " (" << err << ")" << std::endl;
        cpueaxh_close(engine);
        return false;
    }

    uint64_t value = GUEST_MEM_INVALID_STACK_BASE + GUEST_MEM_INVALID_STACK_SIZE - 0x80;
    cpueaxh_reg_write(engine, CPUEAXH_X86_REG_RSP, &value);
    cpueaxh_reg_write(engine, CPUEAXH_X86_REG_RBP, &value);

    value = GUEST_MEM_INVALID_CODE_BASE;
    cpueaxh_reg_write(engine, CPUEAXH_X86_REG_RIP, &value);

    value = 0x202ull;
    cpueaxh_reg_write(engine, CPUEAXH_X86_REG_EFLAGS, &value);

    *out_engine = engine;
    return true;
}

inline int guest_invalid_mem_recovery_hook(cpueaxh_engine* engine, uint32_t type, uint64_t address, size_t size, uint64_t value, void* user_data) {
    GuestInvalidMemHookState* state = (GuestInvalidMemHookState*)user_data;

    std::cout << "invalid hook type=" << guest_mem_hook_type_name(type)
        << " @ 0x" << std::hex << std::setfill('0') << std::setw(16) << address
        << " size=" << std::dec << size
        << " value=0x" << std::hex << std::setw((int)(size * 2)) << value
        << std::dec << std::endl;

    if (type == CPUEAXH_HOOK_MEM_READ_UNMAPPED && address >= GUEST_MEM_INVALID_SRC_BASE && address < (GUEST_MEM_INVALID_SRC_BASE + 0x1000)) {
        ++state->unmapped_read_hits;
        cpueaxh_err err = cpueaxh_mem_map(engine, GUEST_MEM_INVALID_SRC_BASE, 0x1000, CPUEAXH_PROT_READ | CPUEAXH_PROT_WRITE);
        if (err != CPUEAXH_ERR_OK) {
            std::cerr << "cpueaxh_mem_map(src recover) failed: " << cpueaxh_err_string(err)
                << " (" << err << ")" << std::endl;
            return 0;
        }

        err = cpueaxh_mem_write(engine, GUEST_MEM_INVALID_SRC_BASE, &GUEST_MEM_INVALID_EXPECTED_SOURCE, sizeof(GUEST_MEM_INVALID_EXPECTED_SOURCE));
        if (err != CPUEAXH_ERR_OK) {
            std::cerr << "cpueaxh_mem_write(src recover) failed: " << cpueaxh_err_string(err)
                << " (" << err << ")" << std::endl;
            return 0;
        }

        return 1;
    }

    if (type == CPUEAXH_HOOK_MEM_WRITE_PROT && address >= GUEST_MEM_INVALID_DST_BASE && address < (GUEST_MEM_INVALID_DST_BASE + 0x1000)) {
        ++state->write_prot_hits;
        cpueaxh_err err = cpueaxh_mem_protect(engine, GUEST_MEM_INVALID_DST_BASE, 0x1000, CPUEAXH_PROT_READ | CPUEAXH_PROT_WRITE);
        if (err != CPUEAXH_ERR_OK) {
            std::cerr << "cpueaxh_mem_protect(dst recover) failed: " << cpueaxh_err_string(err)
                << " (" << err << ")" << std::endl;
            return 0;
        }

        return 1;
    }

    return 0;
}

inline void run_guest_hook_mem_invalid_demo() {
    cpueaxh_engine* engine = 0;
    if (!setup_guest_invalid_memory_hook_demo(&engine)) {
        return;
    }

    GuestInvalidMemHookState state = {};
    cpueaxh_hook read_unmapped_hook = 0;
    cpueaxh_hook write_prot_hook = 0;

    cpueaxh_err err = cpueaxh_hook_add(engine, &read_unmapped_hook, CPUEAXH_HOOK_MEM_READ_UNMAPPED, (void*)guest_invalid_mem_recovery_hook,
        &state, GUEST_MEM_INVALID_SRC_BASE, GUEST_MEM_INVALID_SRC_BASE + 0xFFF);
    if (err == CPUEAXH_ERR_OK) {
        err = cpueaxh_hook_add(engine, &write_prot_hook, CPUEAXH_HOOK_MEM_WRITE_PROT, (void*)guest_invalid_mem_recovery_hook,
            &state, GUEST_MEM_INVALID_DST_BASE, GUEST_MEM_INVALID_DST_BASE + 0xFFF);
    }
    if (err != CPUEAXH_ERR_OK) {
        std::cerr << "cpueaxh_hook_add(invalid memory) failed: " << cpueaxh_err_string(err)
            << " (" << err << ")" << std::endl;
        if (read_unmapped_hook) cpueaxh_hook_del(engine, read_unmapped_hook);
        if (write_prot_hook) cpueaxh_hook_del(engine, write_prot_hook);
        cpueaxh_close(engine);
        return;
    }

    print_context(engine, "guest invalid memory hook before");

    err = cpueaxh_emu_start(engine, GUEST_MEM_INVALID_CODE_BASE, GUEST_MEM_INVALID_CODE_BASE + sizeof(GUEST_MEM_INVALID_CODE), 0, 0);
    std::cout << "guest invalid memory hook result: " << cpueaxh_err_string(err)
        << " (" << err << ")" << std::endl;

    uint64_t result_value = 0;
    cpueaxh_err read_err = cpueaxh_mem_read(engine, GUEST_MEM_INVALID_DST_BASE, &result_value, sizeof(result_value));
    if (read_err != CPUEAXH_ERR_OK) {
        std::cerr << "cpueaxh_mem_read(dst result) failed: " << cpueaxh_err_string(read_err)
            << " (" << read_err << ")" << std::endl;
    }
    else {
        std::cout << "dst result @ 0x" << std::hex << std::setfill('0') << std::setw(16) << GUEST_MEM_INVALID_DST_BASE
            << " = 0x" << std::setw(16) << result_value << std::dec << std::endl;
    }

    const bool passed = err == CPUEAXH_ERR_OK &&
        read_err == CPUEAXH_ERR_OK &&
        result_value == GUEST_MEM_INVALID_EXPECTED_RESULT &&
        state.unmapped_read_hits == 1 &&
        state.write_prot_hits == 1;

    std::cout << "invalid memory recovery summary: unmapped_read_hits=" << state.unmapped_read_hits
        << ", write_prot_hits=" << state.write_prot_hits
        << ", expected_result=0x" << std::hex << GUEST_MEM_INVALID_EXPECTED_RESULT
        << ", actual_result=0x" << result_value << std::dec << std::endl;
    std::cout << "invalid memory recovery test: " << (passed ? "PASS" : "FAIL") << std::endl;

    print_context(engine, "guest invalid memory hook after");

    cpueaxh_hook_del(engine, read_unmapped_hook);
    cpueaxh_hook_del(engine, write_prot_hook);
    cpueaxh_close(engine);
}
