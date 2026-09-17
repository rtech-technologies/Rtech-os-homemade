#include "ahci.h"
#include <stddef.h>
#include "../../kernel/memory.h"

// Basic PCI I/O ports
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(uint16_t port, uint32_t val) {
    asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

// PCI config reading
static uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

static uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    return (uint16_t)((pci_read_dword(bus, slot, func, offset) >> ((offset & 2) * 8)) & 0xFFFF);
}

// AHCI Controller definitions
#define PCI_CLASS_MASS_STORAGE 0x01
#define PCI_SUBCLASS_SATA      0x06
#define PCI_PROG_IF_AHCI       0x01

#define HBA_PORT_IPM_ACTIVE  1
#define HBA_PORT_DET_PRESENT 3

typedef volatile struct {
    uint32_t clb;       // command list base address, 1K-byte aligned
    uint32_t clbu;      // command list base address upper 32 bits
    uint32_t fb;        // FIS base address, 256-byte aligned
    uint32_t fbu;       // FIS base address upper 32 bits
    uint32_t is;        // interrupt status
    uint32_t ie;        // interrupt enable
    uint32_t cmd;       // command and status
    uint32_t rsv0;
    uint32_t tfd;       // task file data
    uint32_t sig;       // signature
    uint32_t ssts;      // SATA status (SCR0:SStatus)
    uint32_t sctl;      // SATA control (SCR2:SControl)
    uint32_t serr;      // SATA error (SCR1:SError)
    uint32_t sact;      // SATA active (SCR3:SActive)
    uint32_t ci;        // command issue
    uint32_t sntf;      // SATA notification (SCR4:SNotification)
    uint32_t fbs;       // FIS-based switch control
    uint32_t rsv1[11];
    uint32_t vendor[4]; // vendor specific
} hba_port_t;

typedef volatile struct {
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_pts;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;
    uint8_t  rsv[0xA0-0x2C];
    uint8_t  vendor[0x100-0xA0];
    hba_port_t ports[32];
} hba_mem_t;

static hba_mem_t *abar = NULL;
static hba_port_t *active_port = NULL;

// Helper to convert kernel virtual addresses to physical for DMA
static inline uint64_t virt_to_phys(void *ptr) {
    uint64_t virt = (uint64_t)ptr;
    if (virt >= g_kernel_virt_base) {
        return virt - g_kernel_virt_base + g_kernel_phys_base;
    }
    // If it's dynamically allocated by PMM, it uses hhdm
    return virt - g_hhdm_offset;
}

// In a real OS we should dynamically allocate these properly aligned,
// For this bare-metal demo we can statically allocate aligned memory.
__attribute__((aligned(1024)))
static uint8_t clb[1024];

__attribute__((aligned(256)))
static uint8_t fb[256];

__attribute__((aligned(128)))
static uint8_t cmd_table[256];

// PRDT max 1 entry for simplicity
struct hba_prdt_entry {
    uint32_t dba;
    uint32_t dbau;
    uint32_t rsv0;
    uint32_t dbc:22;
    uint32_t rsv1:9;
    uint32_t i:1;
};

struct hba_cmd_table {
    uint8_t cfis[64];
    uint8_t acmd[16];
    uint8_t rsv[48];
    struct hba_prdt_entry prdt_entry[1];
};

struct hba_cmd_header {
    uint8_t cfl:5;
    uint8_t a:1;
    uint8_t c:1;
    uint8_t p:1;
    uint8_t r:1;
    uint8_t b:1;
    uint8_t c_clr:1;
    uint8_t pmp:4;
    uint16_t prdtl;
    uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t rsv1[4];
};

static void port_start(hba_port_t *port) {
    while (port->cmd & 0x8000) ; // Wait until CR (bit 15) is cleared
    port->cmd |= 0x0010; // FRE
    port->cmd |= 0x0001; // ST
}

static void port_stop(hba_port_t *port) {
    port->cmd &= ~0x0001; // Clear ST
    port->cmd &= ~0x0010; // Clear FRE
    while (1) {
        if (port->cmd & 0x4000) continue; // Wait until FR (bit 14) is cleared
        if (port->cmd & 0x8000) continue; // Wait until CR (bit 15) is cleared
        break;
    }
}

static void init_ahci_port(hba_port_t *port) {
    port_stop(port);

    uint64_t phys_clb = virt_to_phys(clb);
    uint64_t phys_fb = virt_to_phys(fb);
    uint64_t phys_cmd_table = virt_to_phys(cmd_table);

    // Set command list base
    port->clb = (uint32_t)phys_clb;
    port->clbu = (uint32_t)(phys_clb >> 32);

    // Set FIS base
    port->fb = (uint32_t)phys_fb;
    port->fbu = (uint32_t)(phys_fb >> 32);

    struct hba_cmd_header *cmdheader = (struct hba_cmd_header *)clb;
    for (int i=0; i<32; i++) {
        cmdheader[i].prdtl = 0; // Clear PRDT length
        cmdheader[i].ctba = 0;
        cmdheader[i].ctbau = 0;
    }

    // Use slot 0
    cmdheader[0].ctba = (uint32_t)phys_cmd_table;
    cmdheader[0].ctbau = (uint32_t)(phys_cmd_table >> 32);

    port_start(port);
}

static void find_ahci_controller(void) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint16_t vendor = pci_read_word(bus, slot, 0, 0);
            if (vendor == 0xFFFF) continue; // Device doesn't exist

            uint8_t class = pci_read_word(bus, slot, 0, 0x0A) >> 8;
            uint8_t subclass = pci_read_word(bus, slot, 0, 0x0A) & 0xFF;
            uint8_t prog_if = pci_read_word(bus, slot, 0, 0x08) >> 8;

            if (class == PCI_CLASS_MASS_STORAGE && subclass == PCI_SUBCLASS_SATA && prog_if == PCI_PROG_IF_AHCI) {
                // Found AHCI controller!
                uint32_t bar5 = pci_read_dword(bus, slot, 0, 0x24);
                uint64_t phys_abar = bar5 & 0xFFFFFFF0;
                abar = (hba_mem_t *)(phys_abar + g_hhdm_offset); // Map physical MMIO to virtual using HHDM

                // We must enable bus mastering in the command register
                uint16_t cmd = pci_read_word(bus, slot, 0, 0x04);
                if (!(cmd & 0x04)) {
                    outl(PCI_CONFIG_ADDRESS, (uint32_t)((bus << 16) | (slot << 11) | (0 << 8) | 0x04 | 0x80000000));
                    outl(PCI_CONFIG_DATA, cmd | 0x04);
                }
                return;
            }
        }
    }
}

void init_storage(void) {
    find_ahci_controller();
    if (!abar) return; // No AHCI controller found

    abar->ghc |= 0x80000000; // Enable AHCI

    for (int i = 0; i < 32; i++) {
        if (abar->pi & (1 << i)) {
            hba_port_t *port = &abar->ports[i];

            // Check status
            uint32_t ssts = port->ssts;
            uint8_t ipm = (ssts >> 8) & 0x0F;
            uint8_t det = ssts & 0x0F;

            // Signature for SATA drive is 0x00000101
            if (det == HBA_PORT_DET_PRESENT && ipm == HBA_PORT_IPM_ACTIVE && port->sig == 0x00000101) {
                active_port = port;
                init_ahci_port(active_port);
                return; // Just use the first one
            }
        }
    }
}

static bool ahci_do_cmd(uint64_t lba, uint32_t count, void *buf, bool write) {
    if (!active_port) return false;

    active_port->is = (uint32_t)-1; // Clear pending interrupts

    struct hba_cmd_header *cmdheader = (struct hba_cmd_header *)clb;
    // Note: the original layout was slightly offset in bits.
    // The direction bit 'w' was missing. We use raw assignment to be safer.
    uint16_t *flags = (uint16_t *)&cmdheader[0];
    *flags = 5; // FIS length in DWORDS
    if (write) *flags |= (1 << 6); // Write flag is bit 6 of word 0
    cmdheader[0].prdtl = 1; // 1 PRDT entry

    uint64_t phys_buf = virt_to_phys(buf);

    struct hba_cmd_table *cmdtbl = (struct hba_cmd_table *)cmd_table;
    cmdtbl->prdt_entry[0].dba = (uint32_t)phys_buf;
    cmdtbl->prdt_entry[0].dbau = (uint32_t)(phys_buf >> 32);
    cmdtbl->prdt_entry[0].dbc = (count * 512) - 1; // Count in bytes minus 1
    cmdtbl->prdt_entry[0].i = 0; // No interrupt

    // Setup command FIS
    uint8_t *fis = (uint8_t *)cmdtbl->cfis;
    for(int i=0; i<64; i++) fis[i]=0;

    fis[0] = 0x27; // Register H2D
    fis[1] = 1 << 7; // Command
    fis[2] = write ? 0x35 : 0x25; // Write DMA Ext / Read DMA Ext
    fis[4] = (uint8_t)lba;
    fis[5] = (uint8_t)(lba >> 8);
    fis[6] = (uint8_t)(lba >> 16);
    fis[7] = 1 << 6; // LBA mode
    fis[8] = (uint8_t)(lba >> 24);
    fis[9] = (uint8_t)(lba >> 32);
    fis[10]= (uint8_t)(lba >> 40);
    fis[12] = (uint8_t)count;
    fis[13] = (uint8_t)(count >> 8);

    active_port->ci = 1; // Issue command (slot 0)

    while (1) {
        if ((active_port->ci & 1) == 0)
            break;
        if (active_port->is & (1 << 30)) { // Task file error
            return false;
        }
    }

    return true;
}

bool ahci_read_sectors(uint64_t lba, uint32_t count, void *buf) {
    return ahci_do_cmd(lba, count, buf, false);
}

bool ahci_write_sectors(uint64_t lba, uint32_t count, void *buf) {
    return ahci_do_cmd(lba, count, buf, true);
}

void init_nvme_queue_stub(void) {
    // Stub for phase 2
}
