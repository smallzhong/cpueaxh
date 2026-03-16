extern "C" {
#include <ntddk.h>
#include <wdf.h>
#include <ntstrsafe.h>
}

#include "cpueaxh.hpp"

extern "C" DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD kexample_driver_unload;

static const char* kexample_err_string(cpueaxh_err err) {
	switch (err) {
	case CPUEAXH_ERR_OK: return "CPUEAXH_ERR_OK";
	case CPUEAXH_ERR_NOMEM: return "CPUEAXH_ERR_NOMEM";
	case CPUEAXH_ERR_ARG: return "CPUEAXH_ERR_ARG";
	case CPUEAXH_ERR_ARCH: return "CPUEAXH_ERR_ARCH";
	case CPUEAXH_ERR_MODE: return "CPUEAXH_ERR_MODE";
	case CPUEAXH_ERR_MAP: return "CPUEAXH_ERR_MAP";
	case CPUEAXH_ERR_READ_UNMAPPED: return "CPUEAXH_ERR_READ_UNMAPPED";
	case CPUEAXH_ERR_WRITE_UNMAPPED: return "CPUEAXH_ERR_WRITE_UNMAPPED";
	case CPUEAXH_ERR_FETCH_UNMAPPED: return "CPUEAXH_ERR_FETCH_UNMAPPED";
	case CPUEAXH_ERR_EXCEPTION: return "CPUEAXH_ERR_EXCEPTION";
	case CPUEAXH_ERR_HOOK: return "CPUEAXH_ERR_HOOK";
	case CPUEAXH_ERR_READ_PROT: return "CPUEAXH_ERR_READ_PROT";
	case CPUEAXH_ERR_WRITE_PROT: return "CPUEAXH_ERR_WRITE_PROT";
	case CPUEAXH_ERR_FETCH_PROT: return "CPUEAXH_ERR_FETCH_PROT";
	case CPUEAXH_ERR_PATCH: return "CPUEAXH_ERR_PATCH";
	default: return "CPUEAXH_ERR_UNKNOWN";
	}
}

static void kexample_log(const char* format, ...) {
	char buffer[512] = {};
	va_list args;
	va_start(args, format);
	NTSTATUS status = RtlStringCbVPrintfA(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "kexample: log formatting failed (0x%08X)\n", status);
		return;
	}

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "kexample: %s\n", buffer);
}

static cpueaxh_err kexample_run_guest_demo() {
	constexpr uint64_t code_base = 0x100000;
	constexpr uint64_t stack_base = 0x200000;
	constexpr uint64_t stack_size = 0x1000;
	constexpr uint8_t code[] = {
		0x48, 0x89, 0xC8,
		0x48, 0x01, 0xD0,
		0x48, 0x83, 0xE8, 0x05,
		0x49, 0x89, 0xC0,
		0x4D, 0x31, 0xC9,
		0x90,
	};

	cpueaxh_engine* engine = nullptr;
	cpueaxh_err err = cpueaxh_open(CPUEAXH_ARCH_X86, CPUEAXH_MODE_64, &engine);
	if (err != CPUEAXH_ERR_OK) {
		return err;
	}

	err = cpueaxh_set_memory_mode(engine, CPUEAXH_MEMORY_MODE_GUEST);
	if (err == CPUEAXH_ERR_OK) {
		err = cpueaxh_mem_map(engine, code_base, 0x1000, CPUEAXH_PROT_READ | CPUEAXH_PROT_WRITE | CPUEAXH_PROT_EXEC);
	}
	if (err == CPUEAXH_ERR_OK) {
		err = cpueaxh_mem_write(engine, code_base, code, sizeof(code));
	}
	if (err == CPUEAXH_ERR_OK) {
		err = cpueaxh_mem_map(engine, stack_base, stack_size, CPUEAXH_PROT_READ | CPUEAXH_PROT_WRITE);
	}

	if (err == CPUEAXH_ERR_OK) {
		uint64_t value = stack_base + stack_size - 0x80;
		cpueaxh_reg_write(engine, CPUEAXH_X86_REG_RSP, &value);
		cpueaxh_reg_write(engine, CPUEAXH_X86_REG_RBP, &value);

		value = 0x123456789ABCDEF0ull;
		cpueaxh_reg_write(engine, CPUEAXH_X86_REG_RCX, &value);
		value = 0x1111111111111111ull;
		cpueaxh_reg_write(engine, CPUEAXH_X86_REG_RDX, &value);
		value = 0x2222222222222222ull;
		cpueaxh_reg_write(engine, CPUEAXH_X86_REG_R8, &value);
		value = 0x3333333333333333ull;
		cpueaxh_reg_write(engine, CPUEAXH_X86_REG_R9, &value);
		value = code_base;
		cpueaxh_reg_write(engine, CPUEAXH_X86_REG_RIP, &value);
		value = 0x202ull;
		cpueaxh_reg_write(engine, CPUEAXH_X86_REG_EFLAGS, &value);
	}

	if (err == CPUEAXH_ERR_OK) {
		err = cpueaxh_emu_start(engine, code_base, code_base + sizeof(code), 0, 0);
	}

	if (err == CPUEAXH_ERR_OK) {
		uint64_t rax = 0;
		uint64_t r8 = 0;
		cpueaxh_reg_read(engine, CPUEAXH_X86_REG_RAX, &rax);
		cpueaxh_reg_read(engine, CPUEAXH_X86_REG_R8, &r8);
		kexample_log("guest demo complete: RAX=0x%llX, R8=0x%llX", rax, r8);
	}
	else {
		kexample_log("guest demo failed: %s (%d), exception=0x%08X, error=0x%08X",
			kexample_err_string(err),
			err,
			cpueaxh_code_exception(engine),
			cpueaxh_error_code_exception(engine));
	}

	cpueaxh_close(engine);
	return err;
}

VOID kexample_driver_unload(_In_ WDFDRIVER Driver) {
	UNREFERENCED_PARAMETER(Driver);
	kexample_log("driver unload");
}

extern "C" NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	WDF_DRIVER_CONFIG config;
	NTSTATUS status;

	WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);
	config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
	config.EvtDriverUnload = kexample_driver_unload;

	status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
	if (!NT_SUCCESS(status)) {
		kexample_log("WdfDriverCreate failed: 0x%08X", status);
		return status;
	}

	kexample_log("driver loaded, starting cpueaxh guest demo");
	cpueaxh_err demo_err = kexample_run_guest_demo();
	if (demo_err != CPUEAXH_ERR_OK) {
		kexample_log("cpueaxh demo finished with error: %s (%d)", kexample_err_string(demo_err), demo_err);
	}
	else {
		kexample_log("cpueaxh demo finished successfully");
	}

	return STATUS_SUCCESS;
}
