#include "fat32.h"
#include "../drivers/storage/ahci.h"

// Basic implementation of FAT32 reading over AHCI
// For simplicity in this demo, we assume the filesystem starts at LBA 0 (no MBR partition table)
// Or we can parse the MBR to find the FAT32 partition.

#pragma pack(push, 1)
typedef struct {
    uint8_t boot_code[3];
    uint8_t oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_ent_cnt;
    uint16_t total_sectors_16;
    uint8_t media;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    // FAT32 extended
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_num;
    uint8_t reserved1;
    uint8_t boot_sig;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t fs_type[8];
} fat32_bpb_t;

typedef struct {
    uint8_t name[11];
    uint8_t attr;
    uint8_t nt_res;
    uint8_t crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t fst_clus_hi;
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t fst_clus_lo;
    uint32_t file_size;
} fat32_dir_entry_t;
#pragma pack(pop)

static uint32_t fat_start_lba;
static uint32_t data_start_lba;
static uint32_t root_cluster;
static uint32_t sectors_per_cluster;

// Shared sector buffer
static uint8_t sector_buf[512] __attribute__((aligned(16)));

static uint32_t partition_start_lba = 0;

static int string_compare(const char *s1, const char *s2, int n) {
    for (int i=0; i<n; i++) {
        if (s1[i] != s2[i]) return 0;
        if (s1[i] == '\0') break;
    }
    return 1;
}

static int string_length(const char *s) {
    int len = 0;
    while(s[len]) len++;
    return len;
}

void init_fat32(void) {
    // Read sector 0
    if (!ahci_read_sectors(0, 1, sector_buf)) return;

    // Very naive MBR check. If it's an MBR, sector_buf[510]==0x55, sector_buf[511]==0xAA
    // Check if it's a FAT32 BPB or MBR.
    // MBR partition 1 starts at offset 0x1BE
    uint8_t part_type = sector_buf[0x1BE + 4];
    if (part_type == 0x0B || part_type == 0x0C) {
        // FAT32 partition
        partition_start_lba = *(uint32_t*)(sector_buf + 0x1BE + 8);
        if (!ahci_read_sectors(partition_start_lba, 1, sector_buf)) return;
    }

    fat32_bpb_t *bpb = (fat32_bpb_t *)sector_buf;
    fat_start_lba = partition_start_lba + bpb->reserved_sectors;
    data_start_lba = fat_start_lba + (bpb->num_fats * bpb->fat_size_32);
    root_cluster = bpb->root_cluster;
    sectors_per_cluster = bpb->sectors_per_cluster;
}

static uint32_t get_cluster_lba(uint32_t cluster) {
    return data_start_lba + ((cluster - 2) * sectors_per_cluster);
}

// Convert "test.txt" to "TEST    TXT"
static void format_fat_name(const char *path, char *fat_name) {
    for(int i=0; i<11; i++) fat_name[i] = ' ';

    int i = 0, j = 0;
    // skip leading slashes
    while (path[i] == '/') i++;

    while (path[i] && path[i] != '.' && j < 8) {
        char c = path[i++];
        if (c >= 'a' && c <= 'z') c -= 32;
        fat_name[j++] = c;
    }
    if (path[i] == '.') {
        i++;
        j = 8;
        while (path[i] && j < 11) {
            char c = path[i++];
            if (c >= 'a' && c <= 'z') c -= 32;
            fat_name[j++] = c;
        }
    }
}

static bool find_file(const char *path, fat32_dir_entry_t *out_entry) {
    char fat_name[11];
    format_fat_name(path, fat_name);

    uint32_t cluster = root_cluster;
    // For this simple demo we only check the first cluster of the root dir
    uint32_t lba = get_cluster_lba(cluster);

    // We will read the whole cluster (assuming e.g. 1 sector/cluster or small enough)
    for (uint32_t s = 0; s < sectors_per_cluster; s++) {
        ahci_read_sectors(lba + s, 1, sector_buf);
        fat32_dir_entry_t *dir = (fat32_dir_entry_t *)sector_buf;

        for (int i = 0; i < 512 / sizeof(fat32_dir_entry_t); i++) {
            if (dir[i].name[0] == 0x00) return false; // End of directory
            if (dir[i].name[0] == 0xE5) continue; // Deleted file
            if (dir[i].attr & 0x08) continue; // Volume label
            if ((dir[i].attr & 0x0F) == 0x0F) continue; // LFN

            if (string_compare((const char*)dir[i].name, fat_name, 11)) {
                *out_entry = dir[i];
                return true;
            }
        }
    }
    return false;
}

bool fs_read_file(const char *path, void *out_buf) {
    fat32_dir_entry_t entry;
    if (!find_file(path, &entry)) return false;

    uint32_t cluster = ((uint32_t)entry.fst_clus_hi << 16) | entry.fst_clus_lo;

    // Very simple read: assume file fits in one contiguous sequence for now
    uint32_t lba = get_cluster_lba(cluster);
    uint32_t sectors_to_read = (entry.file_size + 511) / 512;
    if (sectors_to_read == 0) return true;

    return ahci_read_sectors(lba, sectors_to_read, out_buf);
}

bool fs_write_file(const char *path, const void *in_buf, uint32_t size) {
    fat32_dir_entry_t entry;
    if (!find_file(path, &entry)) {
        // File doesn't exist, we don't support full creation yet in this demo
        return false;
    }

    uint32_t cluster = ((uint32_t)entry.fst_clus_hi << 16) | entry.fst_clus_lo;

    // Write in-place (assumes contiguous and enough space)
    uint32_t lba = get_cluster_lba(cluster);
    uint32_t sectors_to_write = (size + 511) / 512;
    if (sectors_to_write == 0) return true;

    // Ideally we update file_size in the dir entry, but for now we just write sectors
    return ahci_write_sectors(lba, sectors_to_write, (void*)in_buf);
}

void fs_list_dir(const char *path) {
    (void)path; // Stub
}
