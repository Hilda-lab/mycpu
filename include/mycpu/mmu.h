#ifndef MYCPU_MMU_H
#define MYCPU_MMU_H

#include "mycpu/types.h"
#include "mycpu/memory.h"

// 页大小
#define PAGE_SIZE           4096        // 4KB
#define PAGE_SHIFT          12
#define PAGE_MASK           0xFFFFF000

// 页表项数量
#define PAGE_ENTRIES        1024        // 每个页表1024项

// 页目录数量
#define PAGE_DIR_ENTRIES    1024        // 页目录1024项

// 页表项标志
#define PTE_PRESENT         (1 << 0)    // 存在位
#define PTE_WRITABLE        (1 << 1)    // 读写位
#define PTE_USER            (1 << 2)    // 用户/内核位
#define PTE_WRITE_THROUGH   (1 << 3)    // 写穿透
#define PTE_CACHE_DISABLE   (1 << 4)    // 禁用缓存
#define PTE_ACCESSED        (1 << 5)    // 已访问
#define PTE_DIRTY           (1 << 6)    // 已修改
#define PTE_PAT             (1 << 7)    // 页属性表
#define PTE_GLOBAL          (1 << 8)    // 全局页
#define PTE_FRAME           0xFFFFF000  // 物理帧地址掩码

// 页错误错误码
#define PF_PRESENT          (1 << 0)    // 页存在时错误
#define PF_WRITE            (1 << 1)    // 写操作导致
#define PF_USER             (1 << 2)    // 用户模式导致
#define PF_RESERVED         (1 << 3)    // 保留位被设置
#define PF_INSTRUCTION      (1 << 4)    // 取指导致

// 控制寄存器0（CR0）相关位
#define CR0_PG              (1 << 31)   // 分页使能
#define CR0_WP              (1 << 16)   // 写保护

// 控制寄存器3（CR3）相关
#define CR3_FRAME           0xFFFFF000  // 页目录物理地址
#define CR3_PWT             (1 << 3)    // 页级写穿透
#define CR3_PCD             (1 << 4)    // 页级缓存禁用

// 页表项结构
typedef struct {
    uint32_t value;
} pte_t;

// 页目录项结构
typedef struct {
    uint32_t value;
} pde_t;

// MMU状态
typedef struct {
    // 控制寄存器
    uint32_t cr0;           // 控制寄存器0
    uint32_t cr2;           // 页错误地址
    uint32_t cr3;           // 页目录基址
    uint32_t cr4;           // 控制寄存器4
    
    // 页目录（1024个页目录项）
    // 在实际内存中存储，这里只是指针
    uint32_t page_dir_phys; // 页目录物理地址
    
    // 统计信息
    uint64_t tlb_hits;
    uint64_t tlb_misses;
    uint64_t page_faults;
    
    // TLB（转换后备缓冲区）- 简化实现
    struct {
        uint32_t virtual_addr;
        uint32_t physical_addr;
        bool valid;
    } tlb[64];              // 64项TLB
    
    // 关联的内存
    memory_t *memory;
} mmu_t;

// 初始化MMU
void mmu_init(mmu_t *mmu, memory_t *memory);

// 启用分页
void mmu_enable_paging(mmu_t *mmu);

// 禁用分页
void mmu_disable_paging(mmu_t *mmu);

// 检查分页是否启用
bool mmu_is_paging_enabled(const mmu_t *mmu);

// 设置页目录基址
void mmu_set_page_dir(mmu_t *mmu, uint32_t phys_addr);

// 获取页目录基址
uint32_t mmu_get_page_dir(const mmu_t *mmu);

// 虚拟地址转物理地址
bool mmu_translate(const mmu_t *mmu, uint32_t virtual_addr, uint32_t *physical_addr);

// 读取内存（通过虚拟地址）
bool mmu_read8(mmu_t *mmu, uint32_t vaddr, uint8_t *value);
bool mmu_read16(mmu_t *mmu, uint32_t vaddr, uint16_t *value);
bool mmu_read32(mmu_t *mmu, uint32_t vaddr, uint32_t *value);

// 写入内存（通过虚拟地址）
bool mmu_write8(mmu_t *mmu, uint32_t vaddr, uint8_t value);
bool mmu_write16(mmu_t *mmu, uint32_t vaddr, uint16_t value);
bool mmu_write32(mmu_t *mmu, uint32_t vaddr, uint32_t value);

// 页表操作
void mmu_map_page(mmu_t *mmu, uint32_t virtual_addr, uint32_t physical_addr, 
                  uint32_t flags);
void mmu_unmap_page(mmu_t *mmu, uint32_t virtual_addr);

// 设置页表项属性
void mmu_set_page_flags(mmu_t *mmu, uint32_t virtual_addr, uint32_t flags);

// 检查页是否映射
bool mmu_is_page_present(const mmu_t *mmu, uint32_t virtual_addr);

// 获取页错误地址
uint32_t mmu_get_fault_address(const mmu_t *mmu);

// TLB操作
void mmu_flush_tlb(mmu_t *mmu);
void mmu_flush_tlb_entry(mmu_t *mmu, uint32_t virtual_addr);

// 创建页表（分配内存并初始化）
uint32_t mmu_create_page_table(mmu_t *mmu);

// 恒等映射（将虚拟地址映射到相同的物理地址）
void mmu_identity_map(mmu_t *mmu, uint32_t start, uint32_t end, uint32_t flags);

// 获取PTE值
static inline uint32_t pte_get_frame(pte_t pte) {
    return pte.value & PTE_FRAME;
}

static inline bool pte_is_present(pte_t pte) {
    return (pte.value & PTE_PRESENT) != 0;
}

static inline bool pte_is_writable(pte_t pte) {
    return (pte.value & PTE_WRITABLE) != 0;
}

static inline bool pte_is_user(pte_t pte) {
    return (pte.value & PTE_USER) != 0;
}

static inline bool pte_is_accessed(pte_t pte) {
    return (pte.value & PTE_ACCESSED) != 0;
}

static inline bool pte_is_dirty(pte_t pte) {
    return (pte.value & PTE_DIRTY) != 0;
}

// 设置PTE值
static inline void pte_set_frame(pte_t *pte, uint32_t frame) {
    pte->value = (pte->value & ~PTE_FRAME) | (frame & PTE_FRAME);
}

static inline void pte_set_present(pte_t *pte, bool present) {
    if (present) pte->value |= PTE_PRESENT;
    else pte->value &= ~PTE_PRESENT;
}

// 获取PDE值
static inline uint32_t pde_get_frame(pde_t pde) {
    return pde.value & PTE_FRAME;
}

static inline bool pde_is_present(pde_t pde) {
    return (pde.value & PTE_PRESENT) != 0;
}

// 设置PDE值
static inline void pde_set_frame(pde_t *pde, uint32_t frame) {
    pde->value = (pde->value & ~PTE_FRAME) | (frame & PTE_FRAME);
}

static inline void pde_set_present(pde_t *pde, bool present) {
    if (present) pde->value |= PTE_PRESENT;
    else pde->value &= ~PTE_PRESENT;
}

#endif
