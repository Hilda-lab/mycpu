#include "mycpu/mmu.h"
#include "mycpu/memory.h"

#include <string.h>

void mmu_init(mmu_t *mmu, memory_t *memory) {
    memset(mmu, 0, sizeof(mmu_t));
    mmu->memory = memory;
    
    // 清空TLB
    for (int i = 0; i < 64; i++) {
        mmu->tlb[i].valid = false;
    }
}

void mmu_enable_paging(mmu_t *mmu) {
    mmu->cr0 |= CR0_PG;
}

void mmu_disable_paging(mmu_t *mmu) {
    mmu->cr0 &= ~CR0_PG;
}

bool mmu_is_paging_enabled(const mmu_t *mmu) {
    return (mmu->cr0 & CR0_PG) != 0;
}

void mmu_set_page_dir(mmu_t *mmu, uint32_t phys_addr) {
    mmu->cr3 = phys_addr & CR3_FRAME;
    mmu->page_dir_phys = mmu->cr3;
    
    // 切换页目录时刷新TLB
    mmu_flush_tlb(mmu);
}

uint32_t mmu_get_page_dir(const mmu_t *mmu) {
    return mmu->cr3;
}

// TLB查找
static int tlb_lookup(mmu_t *mmu, uint32_t vaddr) {
    uint32_t page = vaddr & PAGE_MASK;
    
    for (int i = 0; i < 64; i++) {
        if (mmu->tlb[i].valid && mmu->tlb[i].virtual_addr == page) {
            mmu->tlb_hits++;
            return i;
        }
    }
    
    mmu->tlb_misses++;
    return -1;
}

// TLB插入
static void tlb_insert(mmu_t *mmu, uint32_t vaddr, uint32_t paddr) {
    // 使用简单的轮转替换策略
    static int tlb_index = 0;
    
    int idx = tlb_index;
    tlb_index = (tlb_index + 1) % 64;
    
    mmu->tlb[idx].virtual_addr = vaddr & PAGE_MASK;
    mmu->tlb[idx].physical_addr = paddr & PAGE_MASK;
    mmu->tlb[idx].valid = true;
}

bool mmu_translate(const mmu_t *mmu, uint32_t virtual_addr, uint32_t *physical_addr) {
    if (!mmu_is_paging_enabled(mmu)) {
        *physical_addr = virtual_addr;
        return true;
    }
    
    // 检查TLB
    mmu_t *non_const_mmu = (mmu_t *)mmu;  // 移除const以允许TLB统计
    int tlb_idx = tlb_lookup(non_const_mmu, virtual_addr);
    if (tlb_idx >= 0) {
        *physical_addr = (mmu->tlb[tlb_idx].physical_addr) | 
                         (virtual_addr & ~PAGE_MASK);
        return true;
    }
    
    // 页表遍历
    uint32_t page_dir_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t page_table_index = (virtual_addr >> 12) & 0x3FF;
    uint32_t page_offset = virtual_addr & 0xFFF;
    
    // 读取页目录项
    uint32_t pde_addr = mmu->cr3 + page_dir_index * 4;
    uint32_t pde_value;
    if (!memory_read32(mmu->memory, pde_addr, &pde_value)) {
        // 页目录读取失败
        non_const_mmu->cr2 = virtual_addr;
        non_const_mmu->page_faults++;
        return false;
    }
    
    // 检查页目录项是否存在
    if (!(pde_value & PTE_PRESENT)) {
        non_const_mmu->cr2 = virtual_addr;
        non_const_mmu->page_faults++;
        return false;
    }
    
    // 获取页表地址
    uint32_t page_table_addr = pde_value & PTE_FRAME;
    
    // 读取页表项
    uint32_t pte_addr = page_table_addr + page_table_index * 4;
    uint32_t pte_value;
    if (!memory_read32(mmu->memory, pte_addr, &pte_value)) {
        non_const_mmu->cr2 = virtual_addr;
        non_const_mmu->page_faults++;
        return false;
    }
    
    // 检查页表项是否存在
    if (!(pte_value & PTE_PRESENT)) {
        non_const_mmu->cr2 = virtual_addr;
        non_const_mmu->page_faults++;
        return false;
    }
    
    // 计算物理地址
    uint32_t page_frame = pte_value & PTE_FRAME;
    *physical_addr = page_frame | page_offset;
    
    // 更新访问位
    pte_value |= PTE_ACCESSED;
    memory_write32(non_const_mmu->memory, pte_addr, pte_value);
    
    // 插入TLB
    tlb_insert(non_const_mmu, virtual_addr, *physical_addr);
    
    return true;
}

bool mmu_read8(mmu_t *mmu, uint32_t vaddr, uint8_t *value) {
    uint32_t paddr;
    if (!mmu_translate(mmu, vaddr, &paddr)) {
        return false;
    }
    return memory_read8(mmu->memory, paddr, value);
}

bool mmu_read16(mmu_t *mmu, uint32_t vaddr, uint16_t *value) {
    // 简化实现：假设不会跨页边界
    uint32_t paddr;
    if (!mmu_translate(mmu, vaddr, &paddr)) {
        return false;
    }
    return memory_read16(mmu->memory, paddr, value);
}

bool mmu_read32(mmu_t *mmu, uint32_t vaddr, uint32_t *value) {
    // 简化实现：假设不会跨页边界
    uint32_t paddr;
    if (!mmu_translate(mmu, vaddr, &paddr)) {
        return false;
    }
    return memory_read32(mmu->memory, paddr, value);
}

bool mmu_write8(mmu_t *mmu, uint32_t vaddr, uint8_t value) {
    uint32_t paddr;
    if (!mmu_translate(mmu, vaddr, &paddr)) {
        return false;
    }
    
    // 检查写权限
    // TODO: 实现写保护检查
    
    return memory_write8(mmu->memory, paddr, value);
}

bool mmu_write16(mmu_t *mmu, uint32_t vaddr, uint16_t value) {
    uint32_t paddr;
    if (!mmu_translate(mmu, vaddr, &paddr)) {
        return false;
    }
    return memory_write16(mmu->memory, paddr, value);
}

bool mmu_write32(mmu_t *mmu, uint32_t vaddr, uint32_t value) {
    uint32_t paddr;
    if (!mmu_translate(mmu, vaddr, &paddr)) {
        return false;
    }
    
    // 写入数据
    if (!memory_write32(mmu->memory, paddr, value)) {
        return false;
    }
    
    // 设置脏位
    // TODO: 更新页表项的脏位
    
    return true;
}

void mmu_map_page(mmu_t *mmu, uint32_t virtual_addr, uint32_t physical_addr, 
                  uint32_t flags) {
    if (!mmu_is_paging_enabled(mmu)) return;
    
    uint32_t page_dir_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t page_table_index = (virtual_addr >> 12) & 0x3FF;
    
    // 获取或创建页表
    uint32_t pde_addr = mmu->cr3 + page_dir_index * 4;
    uint32_t pde_value;
    memory_read32(mmu->memory, pde_addr, &pde_value);
    
    uint32_t page_table_addr;
    if (!(pde_value & PTE_PRESENT)) {
        // 创建新页表
        page_table_addr = mmu_create_page_table(mmu);
        pde_value = page_table_addr | PTE_PRESENT | PTE_WRITABLE | flags;
        memory_write32(mmu->memory, pde_addr, pde_value);
    } else {
        page_table_addr = pde_value & PTE_FRAME;
    }
    
    // 设置页表项
    uint32_t pte_addr = page_table_addr + page_table_index * 4;
    uint32_t pte_value = (physical_addr & PTE_FRAME) | PTE_PRESENT | flags;
    memory_write32(mmu->memory, pte_addr, pte_value);
    
    // 刷新TLB中的对应条目
    mmu_flush_tlb_entry(mmu, virtual_addr);
}

void mmu_unmap_page(mmu_t *mmu, uint32_t virtual_addr) {
    if (!mmu_is_paging_enabled(mmu)) return;
    
    uint32_t page_dir_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t page_table_index = (virtual_addr >> 12) & 0x3FF;
    
    // 获取页表地址
    uint32_t pde_addr = mmu->cr3 + page_dir_index * 4;
    uint32_t pde_value;
    memory_read32(mmu->memory, pde_addr, &pde_value);
    
    if (!(pde_value & PTE_PRESENT)) {
        return;
    }
    
    uint32_t page_table_addr = pde_value & PTE_FRAME;
    
    // 清除页表项
    uint32_t pte_addr = page_table_addr + page_table_index * 4;
    memory_write32(mmu->memory, pte_addr, 0);
    
    // 刷新TLB
    mmu_flush_tlb_entry(mmu, virtual_addr);
}

void mmu_set_page_flags(mmu_t *mmu, uint32_t virtual_addr, uint32_t flags) {
    if (!mmu_is_paging_enabled(mmu)) return;
    
    uint32_t page_dir_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t page_table_index = (virtual_addr >> 12) & 0x3FF;
    
    uint32_t pde_addr = mmu->cr3 + page_dir_index * 4;
    uint32_t pde_value;
    memory_read32(mmu->memory, pde_addr, &pde_value);
    
    if (!(pde_value & PTE_PRESENT)) return;
    
    uint32_t page_table_addr = pde_value & PTE_FRAME;
    uint32_t pte_addr = page_table_addr + page_table_index * 4;
    uint32_t pte_value;
    memory_read32(mmu->memory, pte_addr, &pte_value);
    
    // 保留帧地址，更新标志
    pte_value = (pte_value & PTE_FRAME) | PTE_PRESENT | flags;
    memory_write32(mmu->memory, pte_addr, pte_value);
    
    mmu_flush_tlb_entry(mmu, virtual_addr);
}

bool mmu_is_page_present(const mmu_t *mmu, uint32_t virtual_addr) {
    uint32_t paddr;
    return mmu_translate(mmu, virtual_addr, &paddr);
}

uint32_t mmu_get_fault_address(const mmu_t *mmu) {
    return mmu->cr2;
}

void mmu_flush_tlb(mmu_t *mmu) {
    for (int i = 0; i < 64; i++) {
        mmu->tlb[i].valid = false;
    }
}

void mmu_flush_tlb_entry(mmu_t *mmu, uint32_t virtual_addr) {
    uint32_t page = virtual_addr & PAGE_MASK;
    
    for (int i = 0; i < 64; i++) {
        if (mmu->tlb[i].valid && mmu->tlb[i].virtual_addr == page) {
            mmu->tlb[i].valid = false;
            return;
        }
    }
}

uint32_t mmu_create_page_table(mmu_t *mmu) {
    // 在内存中分配一个页（4KB）
    // 简化实现：从内存末尾分配
    static uint32_t next_page = 0x100000;  // 从1MB开始分配页表
    
    uint32_t page_addr = next_page;
    next_page += PAGE_SIZE;
    
    // 清零页表
    for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        memory_write8(mmu->memory, page_addr + i, 0);
    }
    
    return page_addr;
}

void mmu_identity_map(mmu_t *mmu, uint32_t start, uint32_t end, uint32_t flags) {
    start &= PAGE_MASK;
    end = (end + PAGE_SIZE - 1) & PAGE_MASK;
    
    for (uint32_t addr = start; addr < end; addr += PAGE_SIZE) {
        mmu_map_page(mmu, addr, addr, flags);
    }
}
