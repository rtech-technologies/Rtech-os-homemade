#include <stddef.h>

/* x86_64 UEFI types */
typedef unsigned long long UINT64;
typedef unsigned long long UINTN;
typedef unsigned long long EFI_STATUS;
typedef unsigned short CHAR16;
typedef void *EFI_HANDLE;

#define EFI_SUCCESS            0
#define EFI_LOCATE_BY_PROTOCOL 2
#define EFI_FILE_MODE_READ     1ULL

#define MAX_VOLUMES 16
#define COMMAND_CAPACITY 128
#define FILE_PATH_CAPACITY 256
#define HISTORY_SIZE 16
#define MAX_VARS 32

typedef struct {
    UINTN Signature;
    unsigned int Revision;
    unsigned int HeaderSize;
    unsigned int CRC32;
    unsigned int Reserved;
} EFI_TABLE_HEADER;

typedef struct {
    unsigned int Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} EFI_GUID;

/* ---------- Console protocols ---------- */

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef EFI_STATUS (*EFI_TEXT_STRING)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    CHAR16 *String
);

typedef EFI_STATUS (*EFI_TEXT_CLEAR_SCREEN)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
);

typedef EFI_STATUS (*EFI_TEXT_SET_CURSOR_POSITION)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN Column,
    UINTN Row
);

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void *Reset;
    EFI_TEXT_STRING OutputString;
    void *TestString;
    void *QueryMode;
    void *SetMode;
    void *SetAttribute;
    EFI_TEXT_CLEAR_SCREEN ClearScreen;
    EFI_TEXT_SET_CURSOR_POSITION SetCursorPosition;
    void *Mode;
};

typedef struct {
    unsigned short ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

#define SCAN_UP    0x01
#define SCAN_DOWN  0x02

typedef EFI_STATUS (*EFI_INPUT_READ_KEY)(
    void *This,
    EFI_INPUT_KEY *Key
);

typedef struct {
    void *Reset;
    EFI_INPUT_READ_KEY ReadKeyStroke;
    void *WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

/* ---------- File protocols ---------- */

typedef struct _EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;

typedef EFI_STATUS (*EFI_FILE_OPEN)(
    EFI_FILE_PROTOCOL *This,
    EFI_FILE_PROTOCOL **NewHandle,
    CHAR16 *FileName,
    UINT64 OpenMode,
    UINT64 Attributes
);

typedef EFI_STATUS (*EFI_FILE_CLOSE)(
    EFI_FILE_PROTOCOL *This
);

typedef EFI_STATUS (*EFI_FILE_READ)(
    EFI_FILE_PROTOCOL *This,
    UINTN *BufferSize,
    void *Buffer
);

typedef EFI_STATUS (*EFI_FILE_WRITE)(
    EFI_FILE_PROTOCOL *This,
    UINTN *BufferSize,
    void *Buffer
);

typedef EFI_STATUS (*EFI_FILE_DELETE)(
    EFI_FILE_PROTOCOL *This
);

struct _EFI_FILE_PROTOCOL {
    UINT64 Revision;
    EFI_FILE_OPEN Open;
    EFI_FILE_CLOSE Close;
    EFI_FILE_DELETE Delete;
    EFI_FILE_READ Read;
    EFI_FILE_WRITE Write;
    void *GetPosition;
    void *SetPosition;
    void *GetInfo;
    void *SetInfo;
    void *Flush;
};

typedef struct {
    UINT64 Revision;
    EFI_STATUS (*OpenVolume)(
        void *This,
        EFI_FILE_PROTOCOL **Root
    );
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

#define EFI_FILE_MODE_WRITE   2ULL
#define EFI_FILE_MODE_CREATE  0x8000000000000000ULL
#define EFI_FILE_DIRECTORY    0x0000000000000010ULL

typedef EFI_STATUS (*EFI_FILE_GET_INFO)(
    EFI_FILE_PROTOCOL *This,
    EFI_GUID *InformationType,
    UINTN *BufferSize,
    void *Buffer
);

typedef struct {
    unsigned short Year;
    unsigned char Month;
    unsigned char Day;
    unsigned char Hour;
    unsigned char Minute;
    unsigned char Second;
    unsigned char Pad1;
    unsigned int Nanosecond;
    short TimeZone;
    unsigned char Daylight;
    unsigned char Pad2;
} EFI_TIME;

typedef struct {
    UINTN Size;
    UINTN FileSize;
    UINTN PhysicalSize;
    EFI_TIME CreateTime;
    EFI_TIME LastAccessTime;
    EFI_TIME ModificationTime;
    UINTN Attribute;
    CHAR16 FileName[1];
} EFI_FILE_INFO;

/* ---------- Boot Services ---------- */

typedef struct {
    EFI_TABLE_HEADER Hdr;

    void *Slots0To5[6];
    EFI_STATUS (*FreePool)(void *Buffer); /* slot 6 */

    void *Slots7To15[9];
    EFI_STATUS (*HandleProtocol)(
        EFI_HANDLE Handle,
        EFI_GUID *Protocol,
        void **Interface
    ); /* slot 16 */

    void *Slots17To35[19];
    EFI_STATUS (*LocateHandleBuffer)(
        unsigned int SearchType,
        EFI_GUID *Protocol,
        void *SearchKey,
        UINTN *NoHandles,
        EFI_HANDLE **Buffer
    ); /* slot 36 */
} EFI_BOOT_SERVICES;

/* ---------- System Table ---------- */

typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;
    unsigned int FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    void *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    UINTN NumberOfTableEntries;
    void *ConfigurationTable;
} EFI_SYSTEM_TABLE;

/* ---------- App state & Variables ---------- */

typedef struct {
    char name[32];
    int value;
} Variable;

typedef struct {
    char command[COMMAND_CAPACITY];
    int command_length;
    int row;
    int line_dirty;
    int exit_requested;
    char cwd[FILE_PATH_CAPACITY];
} AppState;

static EFI_SYSTEM_TABLE *g_st;
static AppState app;

static EFI_FILE_PROTOCOL *g_volume_roots[MAX_VOLUMES];
static UINTN g_volume_count;
static UINTN g_selected_volume;

/* Command History & Variables State */
static char history[HISTORY_SIZE][COMMAND_CAPACITY];
static int history_count = 0;
static int history_index = -1;

static Variable g_vars[MAX_VARS];
static int g_var_count = 0;

typedef struct {
    char alias[32];
    char target[32];
} CommandAlias;

#define MAX_ALIASES 16
static CommandAlias g_aliases[MAX_ALIASES];
static int g_alias_count = 0;

static const EFI_GUID EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID = {
    0x964e5b22, 0x6459, 0x11d2,
    {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}
};

/* Forward declarations */
static void execute_command(char *cmd_str);
static void enumerate_volumes(void);

/* ---------- Utility and drawing functions ---------- */

static void to_efi_str(const char *src, CHAR16 *dest, UINTN capacity) {
    UINTN i = 0;
    if (capacity == 0) return;
    while (i + 1 < capacity && src[i] != '\0') {
        dest[i] = (CHAR16)(unsigned char)src[i];
        i++;
    }
    dest[i] = 0;
}

static int string_compare(const char *a, const char *b) {
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

static int starts_with(const char *text, const char *prefix) {
    while (*prefix != '\0') {
        if (*text++ != *prefix++) {
            return 0;
        }
    }
    return 1;
}

static void room_clear(void) {
    if (g_st != NULL && g_st->ConOut != NULL) {
        g_st->ConOut->ClearScreen(g_st->ConOut);
    }
}

static void draw_text(int x, int y, const char *str) {
    CHAR16 buffer[256];
    if (g_st == NULL || g_st->ConOut == NULL || str == NULL || x < 0 || y < 0) {
        return;
    }
    to_efi_str(str, buffer, sizeof(buffer) / sizeof(buffer[0]));
    g_st->ConOut->SetCursorPosition(g_st->ConOut, (UINTN)x, (UINTN)y);
    g_st->ConOut->OutputString(g_st->ConOut, buffer);
}

static int get_input_key(EFI_INPUT_KEY *key) {
    if (g_st == NULL || g_st->ConIn == NULL || g_st->ConIn->ReadKeyStroke == NULL) {
        return 0;
    }
    if (g_st->ConIn->ReadKeyStroke(g_st->ConIn, key) != EFI_SUCCESS) {
        return 0;
    }
    return 1;
}

static void sleep_ms(unsigned int ms) {
    volatile unsigned long long i;
    for (unsigned int m = 0; m < ms; m++) {
        for (i = 0; i < 50000; i++) {
            __asm__ __volatile__("nop");
        }
    }
}

static void draw_header(void) {
    draw_text(2, 1, "========================================");
    draw_text(2, 2, "            RTECH os*2 v2.0.1           ");
    draw_text(2, 3, "========================================");
}

/* ---------- Math & Variable Helpers ---------- */

static int get_var(const char *name) {
    for (int i = 0; i < g_var_count; i++) {
        if (string_compare(g_vars[i].name, name) == 0) return g_vars[i].value;
    }
    return 0;
}

static void set_var(const char *name, int val) {
    for (int i = 0; i < g_var_count; i++) {
        if (string_compare(g_vars[i].name, name) == 0) {
            g_vars[i].value = val;
            return;
        }
    }
    if (g_var_count < MAX_VARS) {
        int i = 0;
        while (name[i] != '\0' && i < 31) { g_vars[g_var_count].name[i] = name[i]; i++; }
        g_vars[g_var_count].name[i] = '\0';
        g_vars[g_var_count].value = val;
        g_var_count++;
    }
}

static int parse_val_or_var(const char *str) {
    int val = 0;
    int neg = 0;
    const char *p = str;
    if (*p == '-') { neg = 1; p++; }
    int is_num = 1;
    while (*p != '\0') {
        if (*p < '0' || *p > '9') { is_num = 0; break; }
        val = val * 10 + (*p - '0');
        p++;
    }
    if (is_num) return neg ? -val : val;
    return get_var(str);
}

static int eval_expression(const char *expr) {
    char op = 0;
    const char *op_ptr = NULL;
    for (const char *p = expr; *p != '\0'; p++) {
        if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
            op = *p;
            op_ptr = p;
            break;
        }
    }
    if (!op_ptr) {
        return parse_val_or_var(expr);
    }

    char left[64], right[64];
    int l_len = 0, r_len = 0;

    const char *p = expr;
    while (p < op_ptr && l_len < 63) {
        if (*p != ' ') left[l_len++] = *p;
        p++;
    }
    left[l_len] = '\0';

    p = op_ptr + 1;
    while (*p != '\0' && r_len < 63) {
        if (*p != ' ') right[r_len++] = *p;
        p++;
    }
    right[r_len] = '\0';

    int l_val = parse_val_or_var(left);
    int r_val = parse_val_or_var(right);

    if (op == '+') return l_val + r_val;
    if (op == '-') return l_val - r_val;
    if (op == '*') return l_val * r_val;
    if (op == '/') return r_val != 0 ? l_val / r_val : 0;
    return 0;
}

static void int_to_str(int n, char *buf) {
    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    int i = 0, neg = 0;
    if (n < 0) { neg = 1; n = -n; }
    while (n > 0) {
        buf[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    if (neg) buf[i++] = '-';
    buf[i] = '\0';
    int start = 0, end = i - 1;
    while (start < end) {
        char tmp = buf[start];
        buf[start] = buf[end];
        buf[end] = tmp;
        start++; end--;
    }
}

/* ---------- Path Resolver Helper ---------- */

static EFI_FILE_PROTOCOL *resolve_path(const char *input_path, CHAR16 *out_efi_path, UINTN capacity) {
    UINTN target_vol = g_selected_volume;
    const char *p = input_path;

    if (p[0] == 'D' && p[1] >= '0' && p[1] <= '9' && p[2] == ':') {
        target_vol = (UINTN)(p[1] - '0');
        p += 3;
    }

    if (target_vol >= g_volume_count) {
        target_vol = 0;
    }

    char combined[FILE_PATH_CAPACITY];
    int c_idx = 0;

    if (p[0] == '/' || p[0] == '\\') {
        combined[c_idx++] = '\\';
        p++;
        while (*p != '\0' && c_idx < FILE_PATH_CAPACITY - 1) {
            combined[c_idx++] = (*p == '/') ? '\\' : *p;
            p++;
        }
    } else {
        if (app.cwd[0] != '\0') {
            int k = 0;
            while (app.cwd[k] != '\0' && c_idx < FILE_PATH_CAPACITY - 1) {
                combined[c_idx++] = (app.cwd[k] == '/') ? '\\' : app.cwd[k];
                k++;
            }
            if (c_idx > 0 && combined[c_idx - 1] != '\\' && c_idx < FILE_PATH_CAPACITY - 1) {
                combined[c_idx++] = '\\';
            }
        } else {
            combined[c_idx++] = '\\';
        }

        while (*p != '\0' && c_idx < FILE_PATH_CAPACITY - 1) {
            combined[c_idx++] = (*p == '/') ? '\\' : *p;
            p++;
        }
    }
    combined[c_idx] = '\0';

    to_efi_str(combined, out_efi_path, capacity);
    return g_volume_roots[target_vol];
}

/* ---------- Disk/volume commands & Startup execution ---------- */

static void command_chell(const char *path);

static void load_chell_cfg(void) {
    if (g_volume_count == 0) return;

    CHAR16 efi_path[FILE_PATH_CAPACITY];
    EFI_FILE_PROTOCOL *root = g_volume_roots[0];
    EFI_FILE_PROTOCOL *file = NULL;

    to_efi_str("\\chell_ap.cfg", efi_path, sizeof(efi_path) / sizeof(CHAR16));
    EFI_STATUS status = root->Open(root, &file, efi_path, EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS || file == NULL) return;

    unsigned char buffer[512];
    char line[128];
    int line_len = 0;

    for (;;) {
        UINTN size = sizeof(buffer);
        status = file->Read(file, &size, buffer);
        if (status != EFI_SUCCESS || size == 0) break;

        for (UINTN j = 0; j < size; j++) {
            unsigned char c = buffer[j];
            if (c == '\r') continue;
            if (c == '\n') {
                line[line_len] = '\0';
                if (line_len > 0 && starts_with(line, "alias ") && g_alias_count < MAX_ALIASES) {
                    int k = 6;
                    int a_idx = 0;
                    while (line[k] != ' ' && line[k] != '\0' && a_idx < 31) {
                        g_aliases[g_alias_count].alias[a_idx++] = line[k++];
                    }
                    g_aliases[g_alias_count].alias[a_idx] = '\0';
                    if (line[k] == ' ') k++;

                    int t_idx = 0;
                    while (line[k] != '\0' && t_idx < 31) {
                        g_aliases[g_alias_count].target[t_idx++] = line[k++];
                    }
                    g_aliases[g_alias_count].target[t_idx] = '\0';
                    g_alias_count++;
                }
                line_len = 0;
            } else {
                if (line_len < 127) line[line_len++] = (char)c;
            }
        }
    }
    file->Close(file);
}

static void check_startup_script(void) {
    if (g_volume_count == 0) return;

    load_chell_cfg();

    CHAR16 efi_path[FILE_PATH_CAPACITY];
    EFI_FILE_PROTOCOL *root = g_volume_roots[0];
    EFI_FILE_PROTOCOL *script_file = NULL;

    to_efi_str("\\startup.rsl", efi_path, sizeof(efi_path) / sizeof(CHAR16));
    EFI_STATUS status = root->Open(root, &script_file, efi_path, EFI_FILE_MODE_READ, 0);
    if (status == EFI_SUCCESS && script_file != NULL) {
        script_file->Close(script_file);
        draw_text(2, app.row++, "Running startup.rsl...");
        command_chell("startup.rsl");
    }
}

static void enumerate_volumes(void) {
    EFI_HANDLE *handles = NULL;
    UINTN handle_count = 0;
    g_volume_count = 0;

    if (g_st == NULL || g_st->BootServices == NULL) {
        return;
    }

    EFI_STATUS status = g_st->BootServices->LocateHandleBuffer(
        EFI_LOCATE_BY_PROTOCOL,
        (EFI_GUID *)&EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID,
                                                               NULL,
                                                               &handle_count,
                                                               &handles
    );

    if (status != EFI_SUCCESS || handles == NULL) {
        return;
    }

    for (UINTN i = 0; i < handle_count && g_volume_count < MAX_VOLUMES; i++) {
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = NULL;
        status = g_st->BootServices->HandleProtocol(
            handles[i],
            (EFI_GUID *)&EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID,
                                                    (void **)&fs
        );

        if (status != EFI_SUCCESS || fs == NULL || fs->OpenVolume == NULL) {
            continue;
        }

        EFI_FILE_PROTOCOL *root = NULL;
        status = fs->OpenVolume(fs, &root);
        if (status == EFI_SUCCESS && root != NULL) {
            g_volume_roots[g_volume_count++] = root;
        }
    }

    g_st->BootServices->FreePool(handles);

    if (g_volume_count > 0) {
        check_startup_script();
    }
}

static void select_volume(const char *arg) {
    if (arg[0] != 'D' || arg[1] < '0' || arg[1] > '9' || arg[2] != ':' || arg[3] != '\0') {
        draw_text(2, app.row++, "Usage: mount D0: or ldsk D0:");
        return;
    }
    UINTN index = (UINTN)(arg[1] - '0');
    if (index >= g_volume_count) {
        draw_text(2, app.row++, "Volume not found.");
        return;
    }
    g_selected_volume = index;
    draw_text(2, app.row++, "Volume selected.");
}

/* ---------- Command Implementations ---------- */

static void command_help(void) {
    draw_text(2, app.row++, "Available Commands:");
    draw_text(2, app.row++, "  help, about, uname, pwd, cd, ls");
    draw_text(2, app.row++, "  mkdir, rmdir, touch, mount, ldsk");
    draw_text(2, app.row++, "  chell, let, calc, clear, echo, cat, exit, q");

}

static void command_pwd(void) {
    if (app.cwd[0] == '\0') {
        draw_text(2, app.row++, "/");
    } else {
        draw_text(2, app.row++, app.cwd);
    }
}

static void command_uname(void) {
    draw_text(2, app.row++, "MyOS x86_64 Freestanding UEFI v1.0");
}

static void command_mount(const char *arg) {
    if (*arg == '\0') {
        if (g_volume_count == 0) {
            enumerate_volumes();
        }
        draw_text(2, app.row++, "Mounted volumes:");
        if (g_volume_count == 0) {
            draw_text(4, app.row++, "None found.");
            return;
        }
        for (UINTN i = 0; i < g_volume_count; i++) {
            char line[32];
            line[0] = ' ';
            line[1] = 'D';
            line[2] = (char)('0' + i);
            line[3] = ':';
            line[4] = ' ';
            line[5] = (i == g_selected_volume) ? '*' : ' ';
            line[6] = '\0';
            draw_text(2, app.row, line);
            draw_text(10, app.row++, (i == g_selected_volume) ? "[Active]" : "[Available]");
        }
    } else {
        select_volume(arg);
    }
}

static void command_cd(const char *path) {
    if (*path == '\0') {
        draw_text(2, app.row++, "Usage: cd <path>");
        return;
    }

    if (g_volume_count == 0) {
        draw_text(2, app.row++, "No volumes mounted. Run 'mount' or 'ldsk' first.");
        return;
    }

    if (string_compare(path, "/") == 0 || string_compare(path, "\\") == 0) {
        app.cwd[0] = '\0';
        draw_text(2, app.row++, "Directory changed to root (/)");
        return;
    }

    CHAR16 efi_path[FILE_PATH_CAPACITY];
    EFI_FILE_PROTOCOL *root = resolve_path(path, efi_path, sizeof(efi_path) / sizeof(CHAR16));
    EFI_FILE_PROTOCOL *target_dir = NULL;

    EFI_STATUS status = root->Open(root, &target_dir, efi_path, EFI_FILE_MODE_READ, EFI_FILE_DIRECTORY);
    if (status == EFI_SUCCESS && target_dir != NULL) {
        target_dir->Close(target_dir);

        UINTN j = 0;
        while (path[j] != '\0' && j < FILE_PATH_CAPACITY - 1) {
            app.cwd[j] = path[j];
            j++;
        }
        app.cwd[j] = '\0';
        draw_text(2, app.row++, "Directory changed.");
    } else {
        draw_text(2, app.row++, "Directory not found.");
    }
}

static void command_ls(void) {
    if (g_volume_count == 0) {
        draw_text(2, app.row++, "No volumes mounted. Run 'mount' or 'ldsk' first.");
        return;
    }

    CHAR16 efi_path[FILE_PATH_CAPACITY];
    EFI_FILE_PROTOCOL *root = resolve_path("", efi_path, sizeof(efi_path) / sizeof(CHAR16));
    EFI_FILE_PROTOCOL *dir = NULL;

    EFI_STATUS status = root->Open(root, &dir, efi_path, EFI_FILE_MODE_READ, EFI_FILE_DIRECTORY);
    if (status != EFI_SUCCESS || dir == NULL) {
        draw_text(2, app.row++, "Failed to open directory.");
        return;
    }

    unsigned char info_buffer[512];
    for (;;) {
        UINTN buffer_size = sizeof(info_buffer);
        status = dir->Read(dir, &buffer_size, info_buffer);
        if (status != EFI_SUCCESS || buffer_size == 0) {
            break;
        }

        EFI_FILE_INFO *info = (EFI_FILE_INFO *)info_buffer;
        char name_buf[128];
        UINTN k = 0;
        while (info->FileName[k] != 0 && k < sizeof(name_buf) - 1) {
            name_buf[k] = (char)info->FileName[k];
            k++;
        }
        name_buf[k] = '\0';

        if (info->Attribute & EFI_FILE_DIRECTORY) {
            draw_text(2, app.row, name_buf);
            draw_text(2 + (int)k, app.row, "/");
            app.row++;
        } else {
            draw_text(2, app.row++, name_buf);
        }

        if (app.row > 22) {
            draw_text(2, app.row++, "-- More --");
            break;
        }
    }
    dir->Close(dir);
}

static void command_mkdir(const char *path) {
    if (*path == '\0') {
        draw_text(2, app.row++, "Usage: mkdir <dirname>");
        return;
    }

    if (g_volume_count == 0) {
        draw_text(2, app.row++, "No volumes mounted. Run 'mount' or 'ldsk' first.");
        return;
    }

    CHAR16 efi_path[FILE_PATH_CAPACITY];
    EFI_FILE_PROTOCOL *root = resolve_path(path, efi_path, sizeof(efi_path) / sizeof(CHAR16));
    EFI_FILE_PROTOCOL *new_dir = NULL;

    EFI_STATUS status = root->Open(
        root,
        &new_dir,
        efi_path,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
        EFI_FILE_DIRECTORY
    );

    if (status == EFI_SUCCESS && new_dir != NULL) {
        new_dir->Close(new_dir);
        draw_text(2, app.row++, "Directory created.");
    } else {
        draw_text(2, app.row++, "Failed to create directory.");
    }
}

static void command_rmdir(const char *path) {
    if (*path == '\0') {
        draw_text(2, app.row++, "Usage: rmdir <path>");
        return;
    }

    if (g_volume_count == 0) {
        draw_text(2, app.row++, "No volumes mounted. Run 'mount' or 'ldsk' first.");
        return;
    }

    CHAR16 efi_path[FILE_PATH_CAPACITY];
    EFI_FILE_PROTOCOL *root = resolve_path(path, efi_path, sizeof(efi_path) / sizeof(CHAR16));
    EFI_FILE_PROTOCOL *target = NULL;

    EFI_STATUS status = root->Open(root, &target, efi_path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    if (status == EFI_SUCCESS && target != NULL) {
        status = target->Delete(target);
        if (status == EFI_SUCCESS) {
            draw_text(2, app.row++, "Directory/File removed.");
        } else {
            target->Close(target);
            draw_text(2, app.row++, "Failed to remove (might not be empty).");
        }
    } else {
        draw_text(2, app.row++, "Directory not found.");
    }
}

static void command_touch(const char *args) {
    if (*args == '\0') {
        draw_text(2, app.row++, "Usage: touch <filename> [content]");
        return;
    }

    if (g_volume_count == 0) {
        draw_text(2, app.row++, "No volumes mounted. Run 'mount' or 'ldsk' first.");
        return;
    }

    char filename[128];
    const char *content = NULL;
    int f_idx = 0;

    while (args[f_idx] != '\0' && args[f_idx] != ' ' && f_idx < sizeof(filename) - 1) {
        filename[f_idx] = args[f_idx];
        f_idx++;
    }
    filename[f_idx] = '\0';

    if (args[f_idx] == ' ') {
        content = &args[f_idx + 1];
    }

    CHAR16 efi_path[FILE_PATH_CAPACITY];
    EFI_FILE_PROTOCOL *root = resolve_path(filename, efi_path, sizeof(efi_path) / sizeof(CHAR16));
    EFI_FILE_PROTOCOL *file = NULL;

    EFI_STATUS status = root->Open(
        root,
        &file,
        efi_path,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
        0
    );

    if (status == EFI_SUCCESS && file != NULL) {
        if (content != NULL) {
            char write_buf[256];
            UINTN w_len = 0;
            while (content[w_len] != '\0' && w_len < sizeof(write_buf) - 2) {
                write_buf[w_len] = content[w_len];
                w_len++;
            }
            write_buf[w_len++] = '\n';
            write_buf[w_len] = '\0';

            status = file->Write(file, &w_len, write_buf);
        }
        file->Close(file);
        draw_text(2, app.row++, "File created/written.");
    } else {
        draw_text(2, app.row++, "Failed to create file.");
    }
}

static void command_cat(const char *path) {
    if (*path == '\0') {
        draw_text(2, app.row++, "Usage: cat <path>");
        return;
    }

    if (g_volume_count == 0) {
        draw_text(2, app.row++, "No volumes mounted. Run 'mount' or 'ldsk' first.");
        return;
    }

    CHAR16 efi_path[FILE_PATH_CAPACITY];
    EFI_FILE_PROTOCOL *root = resolve_path(path, efi_path, sizeof(efi_path) / sizeof(CHAR16));
    EFI_FILE_PROTOCOL *file = NULL;

    EFI_STATUS status = root->Open(root, &file, efi_path, EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS || file == NULL) {
        draw_text(2, app.row++, "Could not open file.");
        return;
    }

    unsigned char buffer[256];
    for (;;) {
        UINTN size = sizeof(buffer);
        status = file->Read(file, &size, buffer);
        if (status != EFI_SUCCESS || size == 0) {
            break;
        }

        char line[257];
        UINTN line_length = 0;
        for (UINTN j = 0; j < size; j++) {
            unsigned char c = buffer[j];
            if (c == '\r') continue;
            if (c == '\n') {
                line[line_length] = '\0';
                draw_text(2, app.row++, line);
                line_length = 0;
                if (app.row > 22) {
                    draw_text(2, app.row++, "Output stopped at screen limit.");
                    file->Close(file);
                    return;
                }
            } else {
                line[line_length++] = (c >= 32 && c <= 126) ? (char)c : '.';
            }
        }
        if (app.row > 22) {
            draw_text(2, app.row++, "Output stopped at screen limit.");
            file->Close(file);
            return;
        }
        if (line_length > 0) {
            line[line_length] = '\0';
            draw_text(2, app.row++, line);
            if (app.row > 22) {
                draw_text(2, app.row++, "Output stopped at screen limit.");
                file->Close(file);
                return;
            }
        }
    }
    file->Close(file);
}

static void command_let(const char *args) {
    if (*args == '\0') {
        draw_text(2, app.row++, "Usage: let <var_name> = <expression>");
        return;
    }

    char var_name[32];
    int v_idx = 0;
    while (args[v_idx] != '\0' && args[v_idx] != ' ' && args[v_idx] != '=' && v_idx < sizeof(var_name) - 1) {
        var_name[v_idx] = args[v_idx];
        v_idx++;
    }
    var_name[v_idx] = '\0';

    const char *p = &args[v_idx];
    while (*p == ' ' || *p == '=') p++;

    if (*p == '\0') {
        draw_text(2, app.row++, "Usage: let <var_name> = <expression>");
        return;
    }

    int val = eval_expression(p);
    set_var(var_name, val);

    char msg[64];
    msg[0] = 'S'; msg[1] = 'e'; msg[2] = 't'; msg[3] = ' ';
    int m_idx = 4;
    int k = 0;
    while (var_name[k] != '\0' && m_idx < 50) msg[m_idx++] = var_name[k++];
    msg[m_idx++] = ' '; msg[m_idx++] = '='; msg[m_idx++] = ' ';
    msg[m_idx] = '\0';

    char num_buf[32];
    int_to_str(val, num_buf);
    k = 0;
    while (num_buf[k] != '\0' && m_idx < 62) msg[m_idx++] = num_buf[k++];
    msg[m_idx] = '\0';

    draw_text(2, app.row++, msg);
}
static void autocomplete_command(void) {
    static const char *commands[] = {
        "help", "about", "uname", "pwd", "cd", "ls",
        "mkdir", "rmdir", "touch", "mount", "ldsk",
        "chell", "let", "calc", "clear", "echo",
        "cat", "exit", "q"
    };

    /* Check whether this command starts with the text already typed. */
    for (int i = 0; i < app.command_length; i++) {
        if (app.command[i] == ' ' || app.command[i] == '\t') {
            return;
        }
    }

    int match = -1;
    int match_count = 0;

    for (int c = 0; c < (int)(sizeof(commands) / sizeof(commands[0])); c++) {
        int i = 0;

        while (i < app.command_length &&
            commands[c][i] != '\0' &&
            app.command[i] == commands[c][i]) {
            i++;
            }

            if (i == app.command_length) {
                match = c;
                match_count++;
            }
    }

    if (match_count != 1) {
        return; /* No match, or ambiguous match. */
    }

    int i = 0;
    while (commands[match][i] != '\0' &&
        i < (int)sizeof(app.command) - 1) {
        app.command[i] = commands[match][i];
    i++;
        }

        app.command[i] = '\0';
        app.command_length = i;
}


static void command_calc(const char *expr) {
    if (*expr == '\0') {
        draw_text(2, app.row++, "Usage: calc <expression>");
        return;
    }
    int res = eval_expression(expr);
    char buf[32];
    int_to_str(res, buf);
    draw_text(2, app.row++, buf);
}

/* ---------- RSL Script Engine (`chell`) ---------- */

static void command_chell(const char *path) {
    if (*path == '\0') {
        draw_text(2, app.row++, "Usage: chell <script.rsl>");
        return;
    }

    int len = 0;
    while (path[len] != '\0') len++;
    if (len < 4 || path[len-4] != '.' ||
        (path[len-3] != 'r' && path[len-3] != 'R') ||
        (path[len-2] != 's' && path[len-2] != 'S') ||
        (path[len-1] != 'l' && path[len-1] != 'L')) {
        draw_text(2, app.row++, "Error: Only .rsl script files can be executed.");
    return;
        }

        if (g_volume_count == 0) {
            draw_text(2, app.row++, "No volumes mounted. Run 'mount' or 'ldsk' first.");
            return;
        }

        CHAR16 efi_path[FILE_PATH_CAPACITY];
        EFI_FILE_PROTOCOL *root = resolve_path(path, efi_path, sizeof(efi_path) / sizeof(CHAR16));
        EFI_FILE_PROTOCOL *file = NULL;

        EFI_STATUS status = root->Open(root, &file, efi_path, EFI_FILE_MODE_READ, 0);
        if (status != EFI_SUCCESS || file == NULL) {
            draw_text(2, app.row++, "Could not open RSL script file.");
            return;
        }

        unsigned char buffer[256];
        char script_line[COMMAND_CAPACITY];
        int line_len = 0;

        for (;;) {
            UINTN size = sizeof(buffer);
            status = file->Read(file, &size, buffer);
            if (status != EFI_SUCCESS || size == 0) {
                break;
            }

            for (UINTN j = 0; j < size; j++) {
                unsigned char c = buffer[j];
                if (c == '\r') continue;
                if (c == '\n') {
                    script_line[line_len] = '\0';
                    if (line_len > 0) {
                        execute_command(script_line);
                    }
                    line_len = 0;
                } else {
                    if (line_len < COMMAND_CAPACITY - 1) {
                        script_line[line_len++] = (char)c;
                    }
                }
            }
        }

        if (line_len > 0) {
            script_line[line_len] = '\0';
            execute_command(script_line);
        }

        file->Close(file);
}

/* ---------- CLI lifecycle ---------- */

static void game_create(void) {
    app.command_length = 0;
    app.command[0] = '\0';
    app.row = 7;
    app.line_dirty = 1;
    app.exit_requested = 0;
    app.cwd[0] = '\0';

    g_volume_count = 0;
    g_selected_volume = 0;
    history_count = 0;
    history_index = -1;
    g_var_count = 0;

    room_clear();
    draw_header();
    draw_text(2, 5, "Type 'help' for a list of commands.");

    enumerate_volumes();
}

static void execute_command(char *cmd_str) {
    if (cmd_str[0] != '\0') {
        int h_idx = history_count % HISTORY_SIZE;
        int i = 0;
        while (cmd_str[i] != '\0' && i < COMMAND_CAPACITY - 1) {
            history[h_idx][i] = cmd_str[i];
            i++;
        }
        history[h_idx][i] = '\0';
        history_count++;
    }
    history_index = history_count;

    char backup_cmd[COMMAND_CAPACITY];
    int backup_len = app.command_length;
    int i = 0;
    while (cmd_str[i] != '\0' && i < COMMAND_CAPACITY - 1) {
        backup_cmd[i] = app.command[i];
        app.command[i] = cmd_str[i];
        i++;
    }
    app.command[i] = '\0';
    app.command_length = i;

    app.row++;

    char resolved_cmd[COMMAND_CAPACITY];
    int r_idx = 0;
    while(app.command[r_idx] != '\0') {
        resolved_cmd[r_idx] = app.command[r_idx];
        r_idx++;
    }
    resolved_cmd[r_idx] = '\0';

    // Basic alias replacement for the first word
    char first_word[32];
    int w_idx = 0;
    while (resolved_cmd[w_idx] != ' ' && resolved_cmd[w_idx] != '\0' && w_idx < 31) {
        first_word[w_idx] = resolved_cmd[w_idx];
        w_idx++;
    }
    first_word[w_idx] = '\0';

    for (int a = 0; a < g_alias_count; a++) {
        if (string_compare(first_word, g_aliases[a].alias) == 0) {
            // Reconstruct the command string with the target
            char new_cmd[COMMAND_CAPACITY];
            int n_idx = 0;
            int t_idx = 0;
            while(g_aliases[a].target[t_idx] != '\0' && n_idx < COMMAND_CAPACITY - 1) {
                new_cmd[n_idx++] = g_aliases[a].target[t_idx++];
            }
            if (resolved_cmd[w_idx] != '\0') {
                while(resolved_cmd[w_idx] != '\0' && n_idx < COMMAND_CAPACITY - 1) {
                    new_cmd[n_idx++] = resolved_cmd[w_idx++];
                }
            }
            new_cmd[n_idx] = '\0';
            r_idx = 0;
            while(new_cmd[r_idx] != '\0') {
                resolved_cmd[r_idx] = new_cmd[r_idx];
                r_idx++;
            }
            resolved_cmd[r_idx] = '\0';
            break;
        }
    }

    if (string_compare(resolved_cmd, "help") == 0) {
        command_help();
    } else if (string_compare(resolved_cmd, "about") == 0) {
        draw_text(2, app.row++, "UEFI CLI with a GML-style text wrapper.");
    } else if (string_compare(resolved_cmd, "uname") == 0) {
        command_uname();
    } else if (string_compare(resolved_cmd, "pwd") == 0) {
        command_pwd();
    } else if (string_compare(resolved_cmd, "cd") == 0) {
        command_cd("");
    } else if (starts_with(resolved_cmd, "cd ")) {
        command_cd(&resolved_cmd[3]);
    } else if (string_compare(resolved_cmd, "ls") == 0) {
        command_ls();
    } else if (string_compare(resolved_cmd, "mkdir") == 0) {
        command_mkdir("");
    } else if (starts_with(resolved_cmd, "mkdir ")) {
        command_mkdir(&resolved_cmd[6]);
    } else if (string_compare(resolved_cmd, "rmdir") == 0) {
        command_rmdir("");
    } else if (starts_with(resolved_cmd, "rmdir ")) {
        command_rmdir(&resolved_cmd[6]);
    } else if (string_compare(resolved_cmd, "touch") == 0) {
        command_touch("");
    } else if (starts_with(resolved_cmd, "touch ")) {
        command_touch(&resolved_cmd[6]);
    } else if (string_compare(resolved_cmd, "let") == 0) {
        command_let("");
    } else if (starts_with(resolved_cmd, "let ")) {
        command_let(&resolved_cmd[4]);
    } else if (string_compare(resolved_cmd, "calc") == 0) {
        command_calc("");
    } else if (starts_with(resolved_cmd, "calc ")) {
        command_calc(&resolved_cmd[5]);
    } else if (string_compare(resolved_cmd, "mount") == 0 || string_compare(resolved_cmd, "ldsk") == 0) {
        command_mount("");
    } else if (starts_with(resolved_cmd, "mount ")) {
        command_mount(&resolved_cmd[6]);
    } else if (starts_with(resolved_cmd, "ldsk ")) {
        command_mount(&resolved_cmd[5]);
    } else if (string_compare(resolved_cmd, "chell") == 0) {
        command_chell("");
    } else if (starts_with(resolved_cmd, "chell ")) {
        command_chell(&resolved_cmd[6]);
    } else if (string_compare(resolved_cmd, "clear") == 0) {
        room_clear();
        draw_header();
        app.row = 5;
    } else if (string_compare(resolved_cmd, "echo") == 0) {
        draw_text(2, app.row++, "Usage: echo <text>");
    } else if (starts_with(resolved_cmd, "echo ")) {
        draw_text(2, app.row++, &resolved_cmd[5]);
    } else if (string_compare(resolved_cmd, "cat") == 0) {
        command_cat("");
    } else if (starts_with(resolved_cmd, "cat ")) {
        command_cat(&resolved_cmd[4]);
    } else if (string_compare(resolved_cmd, "exit") == 0 || string_compare(resolved_cmd, "q") == 0) {
        app.exit_requested = 1;
    } else if (app.command_length > 0) {
        command_help();
    }

    for (i = 0; i < backup_len; i++) {
        app.command[i] = backup_cmd[i];
    }
    app.command[backup_len] = '\0';
    app.command_length = backup_len;

    if (app.row > 22) {
        room_clear();
        draw_header();
        app.row = 5;
    }
}

static void run_command(void) {
    app.command[app.command_length] = '\0';
    execute_command(app.command);
    app.command_length = 0;
    app.command[0] = '\0';
    history_index = history_count;
    app.line_dirty = 1;
}

static void game_step(void) {
    EFI_INPUT_KEY key = {0};
    if (!get_input_key(&key)) {
        return;
    }

    if (key.ScanCode == SCAN_UP) {
        if (history_count > 0 && history_index > 0) {
            history_index--;
            int h_idx = history_index % HISTORY_SIZE;
            int i = 0;
            while (history[h_idx][i] != '\0' && i < COMMAND_CAPACITY - 1) {
                app.command[i] = history[h_idx][i];
                i++;
            }
            app.command[i] = '\0';
            app.command_length = i;
            app.line_dirty = 1;
        }
    } else if (key.ScanCode == SCAN_DOWN) {
        if (history_count > 0 && history_index < history_count) {
            history_index++;
            if (history_index == history_count) {
                app.command[0] = '\0';
                app.command_length = 0;
            } else {
                int h_idx = history_index % HISTORY_SIZE;
                int i = 0;
                while (history[h_idx][i] != '\0' && i < COMMAND_CAPACITY - 1) {
                    app.command[i] = history[h_idx][i];
                    i++;
                }
                app.command[i] = '\0';
                app.command_length = i;
            }
            app.line_dirty = 1;
        }
    } else if (key.UnicodeChar == '\t') {
        autocomplete_command();  // Must update app.command and app.command_length.
        app.line_dirty = 1;
    } else if (key.UnicodeChar == '\r' || key.UnicodeChar == '\n') {
        run_command();
    } else if ((key.UnicodeChar == 8 || key.UnicodeChar == 127) &&
        app.command_length > 0) {
        app.command_length--;
    app.command[app.command_length] = '\0';
    app.line_dirty = 1;
        } else if (key.UnicodeChar >= 32 && key.UnicodeChar <= 126 &&
            app.command_length < (int)sizeof(app.command) - 1) {
            app.command[app.command_length++] = (char)key.UnicodeChar;
        app.command[app.command_length] = '\0';
        app.line_dirty = 1;
            }
}


static void game_draw(void) {
    if (!app.line_dirty) return;

    char prompt_buf[128];
    prompt_buf[0] = 'O';
    prompt_buf[1] = 'S';
    prompt_buf[2] = ':';
    int p_len = 3;

    prompt_buf[p_len++] = 'D';
    prompt_buf[p_len++] = (char)('0' + g_selected_volume);
    prompt_buf[p_len++] = ':';

    if (app.cwd[0] == '\0') {
        prompt_buf[p_len++] = '/';
    } else {
        int k = 0;
        while (app.cwd[k] != '\0' && p_len < (int)sizeof(prompt_buf) - 3) {
            prompt_buf[p_len++] = app.cwd[k++];
        }
    }

    prompt_buf[p_len++] = '>';
    prompt_buf[p_len++] = ' ';
    prompt_buf[p_len] = '\0';

    /*
     * Clear the whole input row first. Adjust INPUT_WIDTH if your console
     * has a different width. draw_text() must treat spaces as blank cells.
     */
    for (int x = 0; x < 78; x++) {
        draw_text(2 + x, app.row, " ");
    }

    draw_text(2, app.row, prompt_buf);
    draw_text(2 + p_len, app.row, app.command);

    app.line_dirty = 0;
}


static void game_cleanup(void) {}

static void game_destroy(void) {
    draw_text(2, app.row + 1, "Returning to firmware...");
}

static void app_main(void) {
    game_create();
    while (!app.exit_requested) {
        game_step();
        game_draw();
        game_cleanup();
        sleep_ms(10);
    }
    game_destroy();
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    (void)ImageHandle;
    if (SystemTable == NULL) return 1;
    g_st = SystemTable;
    app_main();
    return EFI_SUCCESS;
}

void __main(void) {}
