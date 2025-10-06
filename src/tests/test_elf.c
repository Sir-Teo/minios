#include "../kernel/loader/elf.h"
#include "../kernel/kprintf.h"
#include "../arch/x86_64/mm/vmm.h"
#include "../kernel/mm/pmm.h"
#include "../kernel/mm/kmalloc.h"
#include <stdint.h>

// Freestanding C library functions (from support.c)
extern void *memcpy(void *restrict dest, const void *restrict src, size_t n);
extern void *memset(void *dest, int c, size_t n);
extern char *strstr(const char *haystack, const char *needle);

// Test counters
static int tests_run = 0;
static int tests_passed = 0;

// Helper macros
#define TEST(name) do { \
    kprintf("[TEST] %s...", name); \
    tests_run++; \
} while(0)

#define PASS() do { \
    kprintf(" PASS\n"); \
    tests_passed++; \
} while(0)

#define FAIL(msg) do { \
    kprintf(" FAIL: %s\n", msg); \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        FAIL(msg); \
        return; \
    } \
} while(0)

// Test ELF header data
static uint8_t test_elf_minimal[] = {
    // ELF Header
    0x7F, 'E', 'L', 'F',        // Magic
    ELF_CLASS_64,               // 64-bit
    ELF_DATA_2LSB,              // Little endian
    ELF_VERSION_CURRENT,        // Version
    0, 0, 0, 0, 0, 0, 0, 0, 0,  // Padding
    0x02, 0x00,                 // e_type = ET_EXEC
    0x3E, 0x00,                 // e_machine = EM_X86_64
    0x01, 0x00, 0x00, 0x00,     // e_version = 1
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // e_entry = 0x1000
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // e_phoff = 64
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // e_shoff = 0
    0x00, 0x00, 0x00, 0x00,     // e_flags = 0
    0x40, 0x00,                 // e_ehsize = 64
    0x38, 0x00,                 // e_phentsize = 56
    0x01, 0x00,                 // e_phnum = 1
    0x00, 0x00,                 // e_shentsize = 0
    0x00, 0x00,                 // e_shnum = 0
    0x00, 0x00,                 // e_shstrndx = 0
};

// Program header for minimal ELF
static uint8_t test_phdr[] = {
    0x01, 0x00, 0x00, 0x00,     // p_type = PT_LOAD
    0x05, 0x00, 0x00, 0x00,     // p_flags = PF_R | PF_X
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // p_offset = 0
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // p_vaddr = 0x1000
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // p_paddr = 0x1000
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // p_filesz = 0x1000
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // p_memsz = 0x1000
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // p_align = 0x1000
};

/**
 * Test 1: ELF subsystem initialization
 */
static void test_elf_init(void) {
    TEST("ELF subsystem initialization");

    elf_init();

    PASS();
}

/**
 * Test 2: Validate correct ELF magic number
 */
static void test_elf_validate_magic(void) {
    TEST("ELF validation - correct magic number");

    bool result = elf_validate(test_elf_minimal, sizeof(test_elf_minimal));
    ASSERT(result == true, "Valid ELF should pass validation");

    PASS();
}

/**
 * Test 3: Reject invalid magic number
 */
static void test_elf_validate_invalid_magic(void) {
    TEST("ELF validation - invalid magic number");

    uint8_t bad_elf[64];
    memcpy(bad_elf, test_elf_minimal, sizeof(bad_elf));
    bad_elf[0] = 0x00;  // Corrupt magic

    bool result = elf_validate(bad_elf, sizeof(bad_elf));
    ASSERT(result == false, "Invalid magic should fail validation");

    const char *error = elf_strerror(0);
    ASSERT(strstr(error, "magic") != NULL, "Error should mention magic number");

    PASS();
}

/**
 * Test 4: Reject 32-bit ELF
 */
static void test_elf_validate_32bit(void) {
    TEST("ELF validation - reject 32-bit");

    uint8_t bad_elf[64];
    memcpy(bad_elf, test_elf_minimal, sizeof(bad_elf));
    bad_elf[EI_CLASS] = ELF_CLASS_32;  // Set to 32-bit

    bool result = elf_validate(bad_elf, sizeof(bad_elf));
    ASSERT(result == false, "32-bit ELF should fail validation");

    const char *error = elf_strerror(0);
    ASSERT(strstr(error, "64-bit") != NULL, "Error should mention 64-bit");

    PASS();
}

/**
 * Test 5: Reject big-endian ELF
 */
static void test_elf_validate_endian(void) {
    TEST("ELF validation - reject big-endian");

    uint8_t bad_elf[64];
    memcpy(bad_elf, test_elf_minimal, sizeof(bad_elf));
    bad_elf[EI_DATA] = ELF_DATA_2MSB;  // Set to big-endian

    bool result = elf_validate(bad_elf, sizeof(bad_elf));
    ASSERT(result == false, "Big-endian ELF should fail validation");

    const char *error = elf_strerror(0);
    ASSERT(strstr(error, "endian") != NULL, "Error should mention endian");

    PASS();
}

/**
 * Test 6: Reject wrong architecture
 */
static void test_elf_validate_arch(void) {
    TEST("ELF validation - reject wrong architecture");

    uint8_t bad_elf[64];
    memcpy(bad_elf, test_elf_minimal, sizeof(bad_elf));
    bad_elf[18] = 0x03;  // Set to i386

    bool result = elf_validate(bad_elf, sizeof(bad_elf));
    ASSERT(result == false, "Non-x86_64 ELF should fail validation");

    const char *error = elf_strerror(0);
    ASSERT(strstr(error, "x86_64") != NULL, "Error should mention x86_64");

    PASS();
}

/**
 * Test 7: Reject ELF with no program headers
 */
static void test_elf_validate_no_phdrs(void) {
    TEST("ELF validation - reject no program headers");

    uint8_t bad_elf[64];
    memcpy(bad_elf, test_elf_minimal, sizeof(bad_elf));
    bad_elf[56] = 0x00;  // e_phnum = 0

    bool result = elf_validate(bad_elf, sizeof(bad_elf));
    ASSERT(result == false, "ELF with no program headers should fail");

    const char *error = elf_strerror(0);
    ASSERT(strstr(error, "program header") != NULL, "Error should mention program headers");

    PASS();
}

/**
 * Test 8: Reject NULL or too-small data
 */
static void test_elf_validate_null(void) {
    TEST("ELF validation - reject NULL/small data");

    bool result = elf_validate(NULL, 1024);
    ASSERT(result == false, "NULL data should fail validation");

    result = elf_validate(test_elf_minimal, 10);
    ASSERT(result == false, "Too-small data should fail validation");

    PASS();
}

/**
 * Test 9: Create valid complete ELF and load it
 */
static void test_elf_load_simple(void) {
    TEST("ELF loading - simple executable");

    // Create a complete minimal ELF file in memory
    size_t elf_size = sizeof(test_elf_minimal) + sizeof(test_phdr) + 0x1000;
    uint8_t *elf_data = (uint8_t *)kmalloc(elf_size);
    ASSERT(elf_data != NULL, "Failed to allocate memory for ELF");

    memset(elf_data, 0, elf_size);
    memcpy(elf_data, test_elf_minimal, sizeof(test_elf_minimal));
    memcpy(elf_data + 64, test_phdr, sizeof(test_phdr));

    // Add some code (NOP instructions) at the program data
    for (int i = 0; i < 16; i++) {
        elf_data[0x100 + i] = 0x90;  // NOP instruction
    }

    uint64_t entry = 0;
    void *addr_space = elf_load(elf_data, elf_size, &entry);

    ASSERT(addr_space != NULL, "Failed to load ELF");
    ASSERT(entry == 0x1000, "Entry point should be 0x1000");

    // Clean up
    if (addr_space) {
        vmm_destroy_address_space((address_space_t *)addr_space);
    }
    kfree(elf_data);

    PASS();
}

/**
 * Test 10: Load ELF with BSS section
 */
static void test_elf_load_bss(void) {
    TEST("ELF loading - with BSS section");

    // Create ELF with BSS (p_memsz > p_filesz)
    size_t elf_size = sizeof(test_elf_minimal) + sizeof(test_phdr) + 0x1000;
    uint8_t *elf_data = (uint8_t *)kmalloc(elf_size);
    ASSERT(elf_data != NULL, "Failed to allocate memory for ELF");

    memset(elf_data, 0, elf_size);
    memcpy(elf_data, test_elf_minimal, sizeof(test_elf_minimal));
    memcpy(elf_data + 64, test_phdr, sizeof(test_phdr));

    // Modify p_filesz to be smaller than p_memsz (creates BSS)
    uint8_t *phdr = elf_data + 64;
    // p_filesz = 0x800 (at offset 32 in program header)
    phdr[32] = 0x00;
    phdr[33] = 0x08;
    // p_memsz remains 0x1000

    uint64_t entry = 0;
    void *addr_space = elf_load(elf_data, elf_size, &entry);

    ASSERT(addr_space != NULL, "Failed to load ELF with BSS");
    ASSERT(entry == 0x1000, "Entry point should be 0x1000");

    // Clean up
    if (addr_space) {
        vmm_destroy_address_space((address_space_t *)addr_space);
    }
    kfree(elf_data);

    PASS();
}

/**
 * Test 11: Load ELF with multiple segments
 */
static void test_elf_load_multi_segment(void) {
    TEST("ELF loading - multiple segments");

    // Create ELF with 2 program headers
    size_t elf_size = sizeof(test_elf_minimal) + (2 * sizeof(test_phdr)) + 0x2000;
    uint8_t *elf_data = (uint8_t *)kmalloc(elf_size);
    ASSERT(elf_data != NULL, "Failed to allocate memory for ELF");

    memset(elf_data, 0, elf_size);
    memcpy(elf_data, test_elf_minimal, sizeof(test_elf_minimal));

    // Update e_phnum to 2
    elf_data[56] = 0x02;

    // First segment: code at 0x1000
    memcpy(elf_data + 64, test_phdr, sizeof(test_phdr));

    // Second segment: data at 0x2000 (writable)
    uint8_t phdr2[56];
    memcpy(phdr2, test_phdr, sizeof(phdr2));
    phdr2[4] = 0x06;  // p_flags = PF_R | PF_W
    // p_vaddr = 0x2000
    phdr2[16] = 0x00;
    phdr2[17] = 0x20;
    // p_paddr = 0x2000
    phdr2[24] = 0x00;
    phdr2[25] = 0x20;
    memcpy(elf_data + 64 + sizeof(test_phdr), phdr2, sizeof(phdr2));

    uint64_t entry = 0;
    void *addr_space = elf_load(elf_data, elf_size, &entry);

    ASSERT(addr_space != NULL, "Failed to load multi-segment ELF");
    ASSERT(entry == 0x1000, "Entry point should be 0x1000");

    // Clean up
    if (addr_space) {
        vmm_destroy_address_space((address_space_t *)addr_space);
    }
    kfree(elf_data);

    PASS();
}

/**
 * Test 12: Error string formatting
 */
static void test_elf_strerror(void) {
    TEST("ELF error strings");

    // Test that we can get error strings
    const char *err = elf_strerror(0);
    ASSERT(err != NULL, "Error string should not be NULL");

    // Validate returns success
    elf_validate(test_elf_minimal, sizeof(test_elf_minimal));
    err = elf_strerror(0);
    ASSERT(strstr(err, "Success") != NULL, "Should return success message");

    PASS();
}

/**
 * Run all ELF loader tests
 */
void test_elf_run_all(void) {
    kprintf("\n=== ELF Loader Tests ===\n");

    tests_run = 0;
    tests_passed = 0;

    // Run all tests
    test_elf_init();
    test_elf_validate_magic();
    test_elf_validate_invalid_magic();
    test_elf_validate_32bit();
    test_elf_validate_endian();
    test_elf_validate_arch();
    test_elf_validate_no_phdrs();
    test_elf_validate_null();
    test_elf_load_simple();
    test_elf_load_bss();
    test_elf_load_multi_segment();
    test_elf_strerror();

    // Print summary
    kprintf("\n=== ELF Loader Test Summary ===\n");
    kprintf("Tests run: %d\n", tests_run);
    kprintf("Tests passed: %d\n", tests_passed);
    kprintf("Tests failed: %d\n", tests_run - tests_passed);

    if (tests_passed == tests_run) {
        kprintf("Result: ALL TESTS PASSED\n");
    } else {
        kprintf("Result: SOME TESTS FAILED\n");
    }
}
