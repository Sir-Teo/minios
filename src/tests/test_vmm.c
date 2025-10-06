#include "../arch/x86_64/mm/vmm.h"
#include "../kernel/mm/pmm.h"
#include <stdint.h>
#include <stdbool.h>

extern void serial_write(const char *s);
extern void serial_write_hex(uint64_t value);
extern void serial_write_dec(uint64_t value);

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            serial_write("[TEST PASS] "); \
            serial_write(message); \
            serial_write("\n"); \
            tests_passed++; \
        } else { \
            serial_write("[TEST FAIL] "); \
            serial_write(message); \
            serial_write("\n"); \
            tests_failed++; \
        } \
    } while(0)

void test_vmm_basic_mapping(void) {
    serial_write("\n=== Testing VMM Basic Mapping ===\n");
    
    // Create a new address space
    address_space_t *as = vmm_create_address_space();
    TEST_ASSERT(as != NULL, "Create address space");
    
    if (!as) return;
    
    // Allocate a physical page
    uint64_t phys = pmm_alloc();
    TEST_ASSERT(phys != 0, "Allocate physical page");
    
    // Map it to a virtual address
    uint64_t virt = 0x400000;  // 4 MB mark (user space)
    bool mapped = vmm_map_page(as, virt, phys, VMM_WRITABLE | VMM_USER);
    TEST_ASSERT(mapped, "Map virtual page to physical");
    
    // Verify mapping
    uint64_t retrieved_phys = vmm_get_physical(as, virt);
    TEST_ASSERT(retrieved_phys == phys, "Verify physical address retrieval");
    
    // Check if mapped
    bool is_mapped = vmm_is_mapped(as, virt);
    TEST_ASSERT(is_mapped, "Check page is mapped");
    
    // Unmap the page
    bool unmapped = vmm_unmap_page(as, virt);
    TEST_ASSERT(unmapped, "Unmap page");
    
    // Verify it's unmapped
    is_mapped = vmm_is_mapped(as, virt);
    TEST_ASSERT(!is_mapped, "Verify page is unmapped");
    
    // Clean up
    pmm_free(phys);
    vmm_destroy_address_space(as);
    
    serial_write("=== Basic Mapping Tests Complete ===\n");
}

void test_vmm_multiple_mappings(void) {
    serial_write("\n=== Testing VMM Multiple Mappings ===\n");
    
    address_space_t *as = vmm_create_address_space();
    TEST_ASSERT(as != NULL, "Create address space for multiple mappings");
    
    if (!as) return;
    
    // Map multiple pages
    const size_t num_pages = 10;
    uint64_t phys_pages[num_pages];
    uint64_t virt_base = 0x400000;
    
    for (size_t i = 0; i < num_pages; i++) {
        phys_pages[i] = pmm_alloc();
        TEST_ASSERT(phys_pages[i] != 0, "Allocate physical page for multiple mapping");
        
        uint64_t virt = virt_base + (i * PAGE_SIZE);
        bool mapped = vmm_map_page(as, virt, phys_pages[i], VMM_WRITABLE | VMM_USER);
        TEST_ASSERT(mapped, "Map multiple pages");
    }
    
    // Verify all mappings
    for (size_t i = 0; i < num_pages; i++) {
        uint64_t virt = virt_base + (i * PAGE_SIZE);
        uint64_t phys = vmm_get_physical(as, virt);
        TEST_ASSERT(phys == phys_pages[i], "Verify multiple mappings");
    }
    
    // Clean up
    for (size_t i = 0; i < num_pages; i++) {
        uint64_t virt = virt_base + (i * PAGE_SIZE);
        vmm_unmap_page(as, virt);
        pmm_free(phys_pages[i]);
    }
    
    vmm_destroy_address_space(as);
    
    serial_write("=== Multiple Mappings Tests Complete ===\n");
}

void test_vmm_address_space_isolation(void) {
    serial_write("\n=== Testing VMM Address Space Isolation ===\n");
    
    // Create two address spaces
    address_space_t *as1 = vmm_create_address_space();
    address_space_t *as2 = vmm_create_address_space();
    
    TEST_ASSERT(as1 != NULL, "Create first address space");
    TEST_ASSERT(as2 != NULL, "Create second address space");
    
    if (!as1 || !as2) {
        if (as1) vmm_destroy_address_space(as1);
        if (as2) vmm_destroy_address_space(as2);
        return;
    }
    
    // Map same virtual address to different physical pages in each space
    uint64_t virt = 0x500000;
    uint64_t phys1 = pmm_alloc();
    uint64_t phys2 = pmm_alloc();
    
    TEST_ASSERT(phys1 != 0 && phys2 != 0, "Allocate physical pages for isolation test");
    
    vmm_map_page(as1, virt, phys1, VMM_WRITABLE | VMM_USER);
    vmm_map_page(as2, virt, phys2, VMM_WRITABLE | VMM_USER);
    
    // Verify isolation
    uint64_t retrieved1 = vmm_get_physical(as1, virt);
    uint64_t retrieved2 = vmm_get_physical(as2, virt);
    
    TEST_ASSERT(retrieved1 == phys1, "First address space mapping correct");
    TEST_ASSERT(retrieved2 == phys2, "Second address space mapping correct");
    TEST_ASSERT(retrieved1 != retrieved2, "Address spaces are isolated");
    
    // Clean up
    vmm_unmap_page(as1, virt);
    vmm_unmap_page(as2, virt);
    pmm_free(phys1);
    pmm_free(phys2);
    vmm_destroy_address_space(as1);
    vmm_destroy_address_space(as2);
    
    serial_write("=== Address Space Isolation Tests Complete ===\n");
}

void test_vmm_kernel_mappings(void) {
    serial_write("\n=== Testing VMM Kernel Mappings ===\n");
    
    address_space_t *kernel_as = vmm_get_kernel_space();
    TEST_ASSERT(kernel_as != NULL, "Get kernel address space");
    
    // Create user address space
    address_space_t *user_as = vmm_create_address_space();
    TEST_ASSERT(user_as != NULL, "Create user address space");
    
    if (!user_as) return;
    
    // Kernel mappings should be present in user address space (top half)
    // We can't easily test this without actually reading memory, but we can
    // verify the structure was created correctly
    TEST_ASSERT(user_as->pml4_virt != NULL, "User space has PML4");
    TEST_ASSERT(user_as->pml4_phys != 0, "User space PML4 has physical address");
    
    vmm_destroy_address_space(user_as);
    
    serial_write("=== Kernel Mappings Tests Complete ===\n");
}

void test_vmm_page_alignment(void) {
    serial_write("\n=== Testing VMM Page Alignment ===\n");
    
    address_space_t *as = vmm_create_address_space();
    if (!as) return;
    
    uint64_t phys = pmm_alloc();
    
    // Test unaligned virtual address (should be aligned down)
    uint64_t virt_unaligned = 0x400567;  // Not page-aligned
    uint64_t virt_aligned = PAGE_ALIGN_DOWN(virt_unaligned);
    
    vmm_map_page(as, virt_unaligned, phys, VMM_WRITABLE);
    
    // Should be mapped at aligned address
    uint64_t retrieved = vmm_get_physical(as, virt_aligned);
    TEST_ASSERT(retrieved == phys, "Unaligned virtual address mapped to aligned");
    
    vmm_unmap_page(as, virt_unaligned);
    pmm_free(phys);
    vmm_destroy_address_space(as);
    
    serial_write("=== Page Alignment Tests Complete ===\n");
}

void run_vmm_tests(void) {
    serial_write("\n");
    serial_write("╔════════════════════════════════════════════════════════════╗\n");
    serial_write("║          VIRTUAL MEMORY MANAGER TEST SUITE                ║\n");
    serial_write("╚════════════════════════════════════════════════════════════╝\n");
    serial_write("\n");
    
    tests_passed = 0;
    tests_failed = 0;
    
    test_vmm_basic_mapping();
    test_vmm_multiple_mappings();
    test_vmm_address_space_isolation();
    test_vmm_kernel_mappings();
    test_vmm_page_alignment();
    
    serial_write("\n");
    serial_write("╔════════════════════════════════════════════════════════════╗\n");
    serial_write("║                    TEST RESULTS                            ║\n");
    serial_write("╚════════════════════════════════════════════════════════════╝\n");
    serial_write("\n");
    serial_write("Tests Passed: ");
    serial_write_dec(tests_passed);
    serial_write("\n");
    serial_write("Tests Failed: ");
    serial_write_dec(tests_failed);
    serial_write("\n");
    
    if (tests_failed == 0) {
        serial_write("\n✓ ALL VMM TESTS PASSED!\n\n");
    } else {
        serial_write("\n✗ SOME VMM TESTS FAILED\n\n");
    }
}
