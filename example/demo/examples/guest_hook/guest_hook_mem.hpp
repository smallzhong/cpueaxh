constexpr uint64_t GUEST_MEM_HOOK_CODE_BASE = 0x110000;
constexpr uint64_t GUEST_MEM_HOOK_STACK_BASE = 0x210000;
constexpr uint64_t GUEST_MEM_HOOK_STACK_SIZE = 0x1000;
constexpr uint64_t GUEST_MEM_HOOK_DATA_BASE = 0x310000;
constexpr uint64_t GUEST_MEM_HOOK_RESULT_ADDRESS = GUEST_MEM_HOOK_DATA_BASE + 8;
constexpr uint8_t GUEST_MEM_HOOK_CODE[] = {
    0x48, 0xBB, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x48, 0x8B, 0x03,
    0x48, 0x83, 0xC0, 0x05,
    0x48, 0x89, 0x43, 0x08,
    0x90,
};

inline const char* guest_mem_hook_type_name(uint32_t type) {
    switch (type) {
    case CPUEAXH_HOOK_MEM_READ: return "mem-read";
    case CPUEAXH_HOOK_MEM_WRITE: return "mem-write";
    case CPUEAXH_HOOK_MEM_FETCH: return "mem-fetch";
    default: return "mem-unknown";
    }
}

inline void guest_memory_access_hook(cpueaxh_engine* engine, uint32_t type, uint64_t address, size_t size, uint64_t value, void* user_data) {
    (void)engine;
    (void)user_data;

    std::cout << guest_mem_hook_type_name(type)
        << " @ 0x" << std::hex << std::setfill('0') << std::setw(16) << address
        << " size=" << std::dec << size
        << " value=0x" << std::hex << std::setw((int)(size * 2)) << value
        << std::dec << std::endl;
}

inline bool setup_guest_memory_hook_demo(cpueaxh_engine** out_engine) {
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

    err = cpueaxh_mem_map(engine, GUEST_MEM_HOOK_CODE_BASE, 0x1000, CPUEAXH_PROT_READ | CPUEAXH_PROT_WRITE | CPUEAXH_PROT_EXEC);
    if (err != CPUEAXH_ERR_OK) {
        std::cerr << "cpueaxh_mem_map(code) failed: " << cpueaxh_err_string(err)
            << " (" << err << ")" << std::endl;
        cpueaxh_close(engine);
        return false;
    }

    err = cpueaxh_mem_write(engine, GUEST_MEM_HOOK_CODE_BASE, GUEST_MEM_HOOK_CODE, sizeof(GUEST_MEM_HOOK_CODE));
    if (err != CPUEAXH_ERR_OK) {
        std::cerr << "cpueaxh_mem_write(code) failed: " << cpueaxh_err_string(err)
            << " (" << err << ")" << std::endl;
        cpueaxh_close(engine);
        return false;
    }

    err = cpueaxh_mem_map(engine, GUEST_MEM_HOOK_STACK_BASE, GUEST_MEM_HOOK_STACK_SIZE, CPUEAXH_PROT_READ | CPUEAXH_PROT_WRITE);
    if (err != CPUEAXH_ERR_OK) {
        std::cerr << "cpueaxh_mem_map(stack) failed: " << cpueaxh_err_string(err)
            << " (" << err << ")" << std::endl;
        cpueaxh_close(engine);
        return false;
    }

    err = cpueaxh_mem_map(engine, GUEST_MEM_HOOK_DATA_BASE, 0x1000, CPUEAXH_PROT_READ | CPUEAXH_PROT_WRITE);
    if (err != CPUEAXH_ERR_OK) {
        std::cerr << "cpueaxh_mem_map(data) failed: " << cpueaxh_err_string(err)
            << " (" << err << ")" << std::endl;
        cpueaxh_close(engine);
        return false;
    }

    uint64_t initial_value = 0x123456789ABCDEF0ull;
    err = cpueaxh_mem_write(engine, GUEST_MEM_HOOK_DATA_BASE, &initial_value, sizeof(initial_value));
    if (err != CPUEAXH_ERR_OK) {
        std::cerr << "cpueaxh_mem_write(data) failed: " << cpueaxh_err_string(err)
            << " (" << err << ")" << std::endl;
        cpueaxh_close(engine);
        return false;
    }

    uint64_t value = GUEST_MEM_HOOK_STACK_BASE + GUEST_MEM_HOOK_STACK_SIZE - 0x80;
    cpueaxh_reg_write(engine, CPUEAXH_X86_REG_RSP, &value);
    cpueaxh_reg_write(engine, CPUEAXH_X86_REG_RBP, &value);

    value = GUEST_MEM_HOOK_CODE_BASE;
    cpueaxh_reg_write(engine, CPUEAXH_X86_REG_RIP, &value);

    value = 0x202ull;
    cpueaxh_reg_write(engine, CPUEAXH_X86_REG_EFLAGS, &value);

    *out_engine = engine;
    return true;
}

inline void run_guest_hook_mem_demo() {
    cpueaxh_engine* engine = 0;
    if (!setup_guest_memory_hook_demo(&engine)) {
        return;
    }

    cpueaxh_hook fetch_hook = 0;
    cpueaxh_hook read_hook = 0;
    cpueaxh_hook write_hook = 0;

    cpueaxh_err err = cpueaxh_hook_add(engine, &fetch_hook, CPUEAXH_HOOK_MEM_FETCH, (void*)guest_memory_access_hook, 0,
        GUEST_MEM_HOOK_CODE_BASE, GUEST_MEM_HOOK_CODE_BASE + sizeof(GUEST_MEM_HOOK_CODE) - 1);
    if (err == CPUEAXH_ERR_OK) {
        err = cpueaxh_hook_add(engine, &read_hook, CPUEAXH_HOOK_MEM_READ, (void*)guest_memory_access_hook, 0,
            GUEST_MEM_HOOK_DATA_BASE, GUEST_MEM_HOOK_DATA_BASE + 0xFFF);
    }
    if (err == CPUEAXH_ERR_OK) {
        err = cpueaxh_hook_add(engine, &write_hook, CPUEAXH_HOOK_MEM_WRITE, (void*)guest_memory_access_hook, 0,
            GUEST_MEM_HOOK_DATA_BASE, GUEST_MEM_HOOK_DATA_BASE + 0xFFF);
    }
    if (err != CPUEAXH_ERR_OK) {
        std::cerr << "cpueaxh_hook_add(memory) failed: " << cpueaxh_err_string(err)
            << " (" << err << ")" << std::endl;
        if (fetch_hook) cpueaxh_hook_del(engine, fetch_hook);
        if (read_hook) cpueaxh_hook_del(engine, read_hook);
        if (write_hook) cpueaxh_hook_del(engine, write_hook);
        cpueaxh_close(engine);
        return;
    }

    print_context(engine, "guest memory hook before");

    err = cpueaxh_emu_start(engine, GUEST_MEM_HOOK_CODE_BASE, GUEST_MEM_HOOK_CODE_BASE + sizeof(GUEST_MEM_HOOK_CODE), 0, 0);
    std::cout << "guest memory hook result: " << cpueaxh_err_string(err)
        << " (" << err << ")" << std::endl;

    uint64_t result_value = 0;
    cpueaxh_err read_err = cpueaxh_mem_read(engine, GUEST_MEM_HOOK_RESULT_ADDRESS, &result_value, sizeof(result_value));
    if (read_err != CPUEAXH_ERR_OK) {
        std::cerr << "cpueaxh_mem_read(result) failed: " << cpueaxh_err_string(read_err)
            << " (" << read_err << ")" << std::endl;
    }
    else {
        std::cout << "result @ 0x" << std::hex << std::setfill('0') << std::setw(16) << GUEST_MEM_HOOK_RESULT_ADDRESS
            << " = 0x" << std::setw(16) << result_value << std::dec << std::endl;
    }

    print_context(engine, "guest memory hook after");

    cpueaxh_hook_del(engine, fetch_hook);
    cpueaxh_hook_del(engine, read_hook);
    cpueaxh_hook_del(engine, write_hook);
    cpueaxh_close(engine);
}
