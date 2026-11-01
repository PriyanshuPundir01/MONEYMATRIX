#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#endif

#define MAX_ACCOUNTS 100
#define MAX_STRING_LEN 50
#define ADHAAR_LEN 12
#define MOBILE_LEN 10
#define ACCOUNT_FILE "accounts.txt"
#define TRANSACTION_FILE "transactions.txt"
#define ENV_FILE ".env"

#define DEFAULT_ADMIN_USER "admin"
#define DEFAULT_ADMIN_PASS "admin123"
#define DEFAULT_HASH_SALT "banking_system_salt"

#define MAX_LOGIN_ATTEMPTS 3
#define LOCKOUT_PENALTY_SEC 3

#ifdef _WIN32
#define COLOR_DEFAULT 15 
#define COLOR_GREEN 10
#define COLOR_RED 12
#define COLOR_YELLOW 14
#define COLOR_CYAN 11
#define COLOR_MAGENTA 13
#define COLOR_BLUE 9
#endif

typedef struct {
    long long account_number;
    char first_name[MAX_STRING_LEN];
    char last_name[MAX_STRING_LEN];
    char father_name[MAX_STRING_LEN];
    char mother_name[MAX_STRING_LEN];
    char aadhar_number[ADHAAR_LEN + 1]; 
    char mobile_number[MOBILE_LEN + 1]; 
    char email[MAX_STRING_LEN];
    int dob_day, dob_month, dob_year;
    char address[MAX_STRING_LEN];
    char account_type[MAX_STRING_LEN];
    char username[MAX_STRING_LEN];
    char password_hashed[128]; 
    double balance;
    bool is_active;
} Account;

typedef struct {
    long long transaction_id;
    char type[20];
    double amount;
    char date_time[MAX_STRING_LEN];
    long long source_account_number;
    long long target_account_number;
} Transaction;

typedef struct BSTNode {
    long long account_number;
    int account_index;
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

Account accounts[MAX_ACCOUNTS];
int num_accounts = 0;
BSTNode *AccountBST_root = NULL;

typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
} SHA256_CTX;

#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

static const uint32_t k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void sha256_transform(SHA256_CTX *ctx, const uint8_t data[]) {
    uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j + 1] << 16) | ((uint32_t)data[j + 2] << 8) | ((uint32_t)data[j + 3]);
    for ( ; i < 64; ++i)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];
    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

static void sha256_init(SHA256_CTX *ctx) {
    ctx->datalen = 0;
    ctx->bitlen = 0;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len) {
    for (size_t i = 0; i < len; ++i) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(SHA256_CTX *ctx, uint8_t hash[]) {
    uint32_t i = ctx->datalen;
    if (ctx->datalen < 56) {
        ctx->data[i++] = 0x80;
        while (i < 56) ctx->data[i++] = 0x00;
    } else {
        ctx->data[i++] = 0x80;
        while (i < 64) ctx->data[i++] = 0x00;
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }
    ctx->bitlen += (uint64_t)ctx->datalen * 8;
    ctx->data[63] = (uint8_t)(ctx->bitlen);
    ctx->data[62] = (uint8_t)(ctx->bitlen >> 8);
    ctx->data[61] = (uint8_t)(ctx->bitlen >> 16);
    ctx->data[60] = (uint8_t)(ctx->bitlen >> 24);
    ctx->data[59] = (uint8_t)(ctx->bitlen >> 32);
    ctx->data[58] = (uint8_t)(ctx->bitlen >> 40);
    ctx->data[57] = (uint8_t)(ctx->bitlen >> 48);
    ctx->data[56] = (uint8_t)(ctx->bitlen >> 56);
    sha256_transform(ctx, ctx->data);
    for (i = 0; i < 4; ++i) {
        hash[i]      = (uint8_t)((ctx->state[0] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 4]  = (uint8_t)((ctx->state[1] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 8]  = (uint8_t)((ctx->state[2] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 12] = (uint8_t)((ctx->state[3] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 16] = (uint8_t)((ctx->state[4] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 20] = (uint8_t)((ctx->state[5] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 24] = (uint8_t)((ctx->state[6] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 28] = (uint8_t)((ctx->state[7] >> (24 - i * 8)) & 0x000000ff);
    }
}

static void sha256_hash_string(const char *str, char *output_hex) {
    SHA256_CTX ctx;
    uint8_t hash[32];
    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t*)str, strlen(str));
    sha256_final(&ctx, hash);
    for (int i = 0; i < 32; i++) {
        sprintf(output_hex + (i * 2), "%02x", hash[i]);
    }
    output_hex[64] = '\0';
}

static unsigned long legacyDJB2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;
    return hash;
}

static void loadEnvFile(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) return;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '#' || line[0] == '\0') continue;
        char *eq = strchr(line, '=');
        if (eq) {
#ifdef _WIN32
            _putenv(line);
#else
            putenv(line);
#endif
        }
    }
    fclose(file);
}

static const char* getAdminUsername() {
    const char *env_user = getenv("MONEYMATRIX_ADMIN_USER");
    return (env_user && strlen(env_user) > 0) ? env_user : DEFAULT_ADMIN_USER;
}

static const char* getAdminPassword() {
    const char *env_pass = getenv("MONEYMATRIX_ADMIN_PASS");
    return (env_pass && strlen(env_pass) > 0) ? env_pass : DEFAULT_ADMIN_PASS;
}

static const char* getSecretSalt() {
    const char *env_salt = getenv("MONEYMATRIX_SECRET_KEY");
    return (env_salt && strlen(env_salt) > 0) ? env_salt : DEFAULT_HASH_SALT;
}

static int constantTimeCompare(const char *a, const char *b) {
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    int result = (int)(len_a ^ len_b);
    size_t max_len = len_a > len_b ? len_a : len_b;
    for (size_t i = 0; i < max_len; i++) {
        char ca = (i < len_a) ? a[i] : 0;
        char cb = (i < len_b) ? b[i] : 0;
        result |= (ca ^ cb);
    }
    return result == 0;
}

static void secureSleep(int seconds) {
#ifdef _WIN32
    Sleep(seconds * 1000);
#else
    sleep(seconds);
#endif
}

void sanitizeInput(char *str, size_t max_len) {
    if (!str) return;
    str[strcspn(str, "\r\n")] = '\0';
    size_t len = strlen(str);
    size_t write_idx = 0;
    for (size_t i = 0; i < len && write_idx < max_len - 1; i++) {
        unsigned char c = (unsigned char)str[i];
        if (c == '|' || c == '\r' || c == '\n') continue;
        if (c < 32 || c > 126) continue;
        str[write_idx++] = (char)c;
    }
    str[write_idx] = '\0';
    while (write_idx > 0 && isspace((unsigned char)str[write_idx - 1])) {
        str[--write_idx] = '\0';
    }
    size_t start = 0;
    while (str[start] && isspace((unsigned char)str[start])) start++;
    if (start > 0) {
        memmove(str, str + start, strlen(str + start) + 1);
    }
}

static bool isValidDigits(const char *str, size_t expected_len) {
    if (!str || strlen(str) != expected_len) return false;
    for (size_t i = 0; i < expected_len; i++) {
        if (!isdigit((unsigned char)str[i])) return false;
    }
    return true;
}

static bool isValidEmail(const char *email) {
    if (!email) return false;
    size_t len = strlen(email);
    if (len < 5) return false;
    const char *at = strchr(email, '@');
    if (!at || at == email) return false;
    const char *dot = strrchr(at, '.');
    if (!dot || dot == at + 1 || *(dot + 1) == '\0') return false;
    return true;
}

void hashAndStore(char *dest, const char *password) {
    char combined[MAX_STRING_LEN + 128];
    const char *salt = getSecretSalt();
    snprintf(combined, sizeof(combined), "%s%s", password, salt);
    sha256_hash_string(combined, dest);
}

static bool verifyPassword(const char *stored_hash, const char *raw_password, Account *acc_to_migrate) {
    if (!stored_hash || !raw_password) return false;
    char combined[MAX_STRING_LEN + 128];
    const char *salt = getSecretSalt();
    snprintf(combined, sizeof(combined), "%s%s", raw_password, salt);

    if (strlen(stored_hash) == 64) {
        char candidate_hash[65];
        sha256_hash_string(combined, candidate_hash);
        return constantTimeCompare(stored_hash, candidate_hash);
    }

    unsigned long legacy_h = legacyDJB2(combined);
    char legacy_hex[32];
    snprintf(legacy_hex, sizeof(legacy_hex), "%lx", legacy_h);
    if (constantTimeCompare(stored_hash, legacy_hex)) {
        if (acc_to_migrate) {
            hashAndStore(acc_to_migrate->password_hashed, raw_password);
        }
        return true;
    }

    if (constantTimeCompare(stored_hash, raw_password)) {
        if (acc_to_migrate) {
            hashAndStore(acc_to_migrate->password_hashed, raw_password);
        }
        return true;
    }

    return false;
}

void displayMainMenu();
void createUserAccount();
void userLogin();
void adminLogin();
void exitSystem();

void userDashboard(int account_index);
void depositMoney(int account_index);
void withdrawMoney(int account_index);
void transferMoney(int source_account_index);
void checkBalance(int account_index);
void showTransactions(long long account_num);
void showUserDetails(int account_index);

void adminDashboard();
void viewAllAccountsAdmin();
void deleteUserAccountAdmin();
void resetUserPasswordAdmin();

void loadAccounts();
void saveAccounts();
void saveTransaction(Transaction t);
long long generateAccountNumber();
long long generateTransactionId();
int findAccountIndex(long long acc_num);
int findAccountIndexByUsername(const char* username);
char* generateTimestamp();

void maskSensitiveData(char *masked_str, const char *original_str, int visible_chars_start, int visible_chars_end, char mask_char);

void clearInputBuffer();
void printCentered(const char *text, int width, char border_char, int color);
void pauseExecution();
void clearScreen();
void printBorder(int width, char border_char, int color);
void getMaskedInput(char *buffer, int max_len);
void toLowerCase(char *str);

#ifdef _WIN32
void gotoxy(int x, int y);
void setcolor(int color);
#else
void gotoxy(int x, int y) { (void)x; (void)y; }
void setcolor(int color) { (void)color; }
#endif

BSTNode* createBSTNode(long long acc_num, int acc_idx);
BSTNode* insertAccountIntoBST(BSTNode* root, long long acc_num, int acc_idx);
int searchAccountInBST(BSTNode* root, long long acc_num);
void freeBST(BSTNode* root);

int main() {
    loadEnvFile(ENV_FILE);
    loadAccounts();
    int choice;
    do {
        clearScreen();
        displayMainMenu();
        setcolor(COLOR_DEFAULT);
        gotoxy(25, 16);
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            setcolor(COLOR_RED);
            gotoxy(25, 17);
            printf("Invalid input. Please enter a number.\n");
            clearInputBuffer();
            pauseExecution();
            continue;
        }
        clearInputBuffer();
        switch (choice) {
            case 1: clearScreen(); createUserAccount(); pauseExecution(); break;
            case 2: clearScreen(); userLogin(); pauseExecution(); break;
            case 3: clearScreen(); adminLogin(); pauseExecution(); break;
            case 4: exitSystem(); break;
            default:
                setcolor(COLOR_RED);
                gotoxy(25, 17);
                printf("Invalid choice. Please try again.\n");
                pauseExecution();
                break;
        }
    } while (choice != 4);
    return 0;
}

BSTNode* createBSTNode(long long acc_num, int acc_idx) {
    BSTNode* newNode = (BSTNode*)malloc(sizeof(BSTNode));
    if (newNode == NULL) {
        setcolor(COLOR_RED);
        printf("Memory allocation failed for BST node.\n");
        setcolor(COLOR_DEFAULT);
        exit(EXIT_FAILURE);
    }
    newNode->account_number = acc_num;
    newNode->account_index = acc_idx;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

BSTNode* insertAccountIntoBST(BSTNode* root, long long acc_num, int acc_idx) {
    if (root == NULL) return createBSTNode(acc_num, acc_idx);
    if (acc_num < root->account_number) root->left = insertAccountIntoBST(root->left, acc_num, acc_idx);
    else if (acc_num > root->account_number) root->right = insertAccountIntoBST(root->right, acc_num, acc_idx);
    return root;
}

int searchAccountInBST(BSTNode* root, long long acc_num) {
    if (root == NULL) return -1;
    if (acc_num == root->account_number) {
        if (accounts[root->account_index].is_active) return root->account_index;
        else return -1;
    }
    if (acc_num < root->account_number) return searchAccountInBST(root->left, acc_num);
    else return searchAccountInBST(root->right, acc_num);
}

void freeBST(BSTNode* root) {
    if (root != NULL) {
        freeBST(root->left);
        freeBST(root->right);
        free(root);
    }
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void printCentered(const char *text, int width, char border_char, int color) {
    setcolor(color);
    int text_len = (int)strlen(text);
    int effective_width = (width == 0) ? (text_len + 4) : width; 

    if (text_len >= effective_width) {
        printf("%s\n", text);
        setcolor(COLOR_DEFAULT);
        return;
    }

    int padding_left = (effective_width - text_len) / 2;
    int padding_right = effective_width - text_len - padding_left;
    int i;

    if (border_char != '\0') {
        for (i = 0; i < effective_width; i++) printf("%c", border_char);
        printf("\n");
    }

    for (i = 0; i < padding_left; i++) printf(" ");
    printf("%s", text);
    for (i = 0; i < padding_right; i++) printf(" ");
    printf("\n");

    if (border_char != '\0') {
        for (i = 0; i < effective_width; i++) printf("%c", border_char);
        printf("\n");
    }
    setcolor(COLOR_DEFAULT);
}

void printBorder(int width, char border_char, int color) {
    setcolor(color);
    for (int i = 0; i < width; i++) printf("%c", border_char);
    printf("\n");
    setcolor(COLOR_DEFAULT);
}

void pauseExecution() {
    setcolor(COLOR_CYAN);
    gotoxy(25, 23);
    printf("Press any key to continue...");
    fflush(stdout);
#ifdef _WIN32
    _getch();
#else
    getchar();
#endif
    gotoxy(0, 24);
    printf("\n");
    setcolor(COLOR_DEFAULT);
}

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void getMaskedInput(char *buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
#ifdef _WIN32
        char ch = _getch();
#else
        char ch = (char)getchar();
#endif
        if (ch == 13 || ch == 10) break;
        if (ch == 8 || ch == 127) {
            if (i > 0) {
                i--;
                printf("\b \b");
            }
        } else if ((unsigned char)ch >= 32 && (unsigned char)ch <= 126) {
            buffer[i++] = ch;
            printf("*");
        }
    }
    buffer[i] = '\0';
    printf("\n");
}

void toLowerCase(char *str) {
    for (int i = 0; str[i]; i++) str[i] = (char)tolower((unsigned char)str[i]);
}

void maskSensitiveData(char *masked_str, const char *original_str, int visible_chars_start, int visible_chars_end, char mask_char) {
    int len = (int)strlen(original_str);
    if (len == 0) {
        strcpy(masked_str, "");
        return;
    }

    if (visible_chars_start < 0) visible_chars_start = 0;
    if (visible_chars_end < 0) visible_chars_end = 0;

    if (visible_chars_start + visible_chars_end >= len) {
        strncpy(masked_str, original_str, MAX_STRING_LEN - 1);
        masked_str[MAX_STRING_LEN - 1] = '\0';
        return;
    }

    int j = 0;
    for (int i = 0; i < visible_chars_start; i++) masked_str[j++] = original_str[i];
    for (int i = visible_chars_start; i < len - visible_chars_end; i++) masked_str[j++] = mask_char;
    for (int i = len - visible_chars_end; i < len; i++) masked_str[j++] = original_str[i];
    masked_str[j] = '\0';
}

void displayMainMenu() {
    setcolor(COLOR_YELLOW);
    gotoxy(0, 0); printCentered("WELCOME TO MONEYMATRIX", 0, '=', COLOR_YELLOW);
    setcolor(COLOR_CYAN);
    gotoxy(0, 3); printCentered("DEVELOPER: PRIYANSHU PUNDIR", 0, '-', COLOR_CYAN); 
    gotoxy(0, 6); printf("\n");
    setcolor(COLOR_GREEN);
    gotoxy(0, 7); printCentered("Main Menu", 0, '*', COLOR_GREEN); 
    setcolor(COLOR_DEFAULT);
    gotoxy(25, 10); printf("1... CREATE NEW ACCOUNT");
    gotoxy(25, 11); printf("2... SIGN In as USER");
    gotoxy(25, 12); printf("3... ADMIN LOGIN");
    gotoxy(25, 13); printf("4... Exit");
    gotoxy(25, 14); printBorder(30, '-', COLOR_DEFAULT);
    gotoxy(25, 16);
}

void loadAccounts() {
    FILE *file = fopen(ACCOUNT_FILE, "r");
    if (file == NULL) {
        num_accounts = 0;
        return;
    }

    num_accounts = 0;
    char line[1024];
    while (fgets(line, sizeof(line), file) != NULL && num_accounts < MAX_ACCOUNTS) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strlen(line) == 0) continue;

        char copy[1024];
        strncpy(copy, line, sizeof(copy) - 1);
        copy[sizeof(copy) - 1] = '\0';

        char *tokens[32];
        int token_count = 0;
        char *p = copy;
        tokens[token_count++] = p;
        while (*p) {
            if (*p == '|') {
                *p = '\0';
                if (token_count < 32) {
                    tokens[token_count++] = p + 1;
                }
            }
            p++;
        }

        if (token_count < 14) continue;

        Account *acc = &accounts[num_accounts];
        memset(acc, 0, sizeof(Account));

        acc->account_number = atoll(tokens[0]);
        strncpy(acc->first_name, tokens[1], MAX_STRING_LEN - 1);
        strncpy(acc->last_name, tokens[2], MAX_STRING_LEN - 1);
        strncpy(acc->father_name, tokens[3], MAX_STRING_LEN - 1);
        strncpy(acc->mother_name, tokens[4], MAX_STRING_LEN - 1);
        strncpy(acc->aadhar_number, tokens[5], ADHAAR_LEN);
        strncpy(acc->mobile_number, tokens[6], MOBILE_LEN);
        strncpy(acc->email, tokens[7], MAX_STRING_LEN - 1);

        int dob_d = 1, dob_m = 1, dob_y = 2000;
        sscanf(tokens[8], "%d-%d-%d", &dob_d, &dob_m, &dob_y);
        acc->dob_day = dob_d;
        acc->dob_month = dob_m;
        acc->dob_year = dob_y;

        strncpy(acc->address, tokens[9], MAX_STRING_LEN - 1);
        strncpy(acc->account_type, tokens[10], MAX_STRING_LEN - 1);
        strncpy(acc->username, tokens[11], MAX_STRING_LEN - 1);

        if (token_count == 15) {
            strncpy(acc->password_hashed, tokens[12], sizeof(acc->password_hashed) - 1);
            acc->balance = atof(tokens[13]);
            acc->is_active = (atoi(tokens[14]) != 0);
        } else if (token_count >= 16) {
            if (strlen(tokens[12]) > 0 && strlen(tokens[13]) > 0 && strchr(tokens[12], '.') == NULL) {
                snprintf(acc->password_hashed, sizeof(acc->password_hashed), "%s%s", tokens[12], tokens[13]);
            } else if (strlen(tokens[12]) > 0 && strchr(tokens[12], '.') == NULL) {
                strncpy(acc->password_hashed, tokens[12], sizeof(acc->password_hashed) - 1);
            } else {
                strncpy(acc->password_hashed, "", sizeof(acc->password_hashed) - 1);
            }
            acc->balance = atof(tokens[14]);
            acc->is_active = (atoi(tokens[15]) != 0);
        }

        sanitizeInput(acc->first_name, MAX_STRING_LEN);
        sanitizeInput(acc->last_name, MAX_STRING_LEN);
        sanitizeInput(acc->father_name, MAX_STRING_LEN);
        sanitizeInput(acc->mother_name, MAX_STRING_LEN);
        sanitizeInput(acc->email, MAX_STRING_LEN);
        sanitizeInput(acc->address, MAX_STRING_LEN);
        sanitizeInput(acc->account_type, MAX_STRING_LEN);
        sanitizeInput(acc->username, MAX_STRING_LEN);

        if (acc->is_active) {
            AccountBST_root = insertAccountIntoBST(AccountBST_root, acc->account_number, num_accounts);
        }
        num_accounts++;
    }
    fclose(file);
}

void saveAccounts() {
    const char *tmp_filename = "accounts.tmp";
    FILE *file = fopen(tmp_filename, "w");
    if (file == NULL) {
        setcolor(COLOR_RED);
        printf("Error opening temporary file for secure saving.\n");
        setcolor(COLOR_DEFAULT);
        return;
    }

    for (int i = 0; i < num_accounts; i++) {
        Account *acc = &accounts[i];
        fprintf(file, "%lld|%s|%s|%s|%s|%s|%s|%s|%02d-%02d-%04d|%s|%s|%s|%s|%.2lf|%d\n",
                acc->account_number,
                acc->first_name, acc->last_name,
                acc->father_name, acc->mother_name,
                acc->aadhar_number, acc->mobile_number, 
                acc->email,
                acc->dob_day, acc->dob_month, acc->dob_year,
                acc->address, acc->account_type,
                acc->username, acc->password_hashed,
                acc->balance, acc->is_active ? 1 : 0);
    }
    fflush(file);
    fclose(file);

    remove(ACCOUNT_FILE);
    if (rename(tmp_filename, ACCOUNT_FILE) != 0) {
        setcolor(COLOR_RED);
        printf("Atomic database update failed.\n");
        setcolor(COLOR_DEFAULT);
    }
}

void saveTransaction(Transaction t) {
    FILE *file = fopen(TRANSACTION_FILE, "a");
    if (file == NULL) {
        setcolor(COLOR_RED);
        printf("Error opening transaction file for saving.\n");
        setcolor(COLOR_DEFAULT);
        return;
    }
    fprintf(file, "%lld|%s|%.2lf|%s|%lld|%lld\n",
            t.transaction_id, t.type, t.amount, t.date_time,
            t.source_account_number, t.target_account_number);
    fflush(file);
    fclose(file);
}

long long generateAccountNumber() {
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }
    long long acc_num;
    bool unique = false;
    do {
        acc_num = (long long)rand() * rand();
        if (acc_num < 0) acc_num = -acc_num;
        acc_num = acc_num % 9000000000LL + 1000000000LL;
        if (searchAccountInBST(AccountBST_root, acc_num) == -1) unique = true;
    } while (!unique);
    return acc_num;
}

long long generateTransactionId() {
    return (long long)time(NULL) * 1000 + (rand() % 1000);
}

char* generateTimestamp() {
    static char timestamp[MAX_STRING_LEN];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    return timestamp;
}

int findAccountIndex(long long acc_num) {
    return searchAccountInBST(AccountBST_root, acc_num);
}

int findAccountIndexByUsername(const char* username_input) {
    char temp_input_username[MAX_STRING_LEN];
    char temp_stored_username[MAX_STRING_LEN];
    strncpy(temp_input_username, username_input, MAX_STRING_LEN - 1);
    temp_input_username[MAX_STRING_LEN - 1] = '\0';
    toLowerCase(temp_input_username);

    for (int i = 0; i < num_accounts; i++) {
        if (accounts[i].is_active) {
            strncpy(temp_stored_username, accounts[i].username, MAX_STRING_LEN - 1);
            temp_stored_username[MAX_STRING_LEN - 1] = '\0';
            toLowerCase(temp_stored_username);
            if (strcmp(temp_stored_username, temp_input_username) == 0) return i;
        }
    }
    return -1;
}

void createUserAccount() {
    if (num_accounts >= MAX_ACCOUNTS) {
        setcolor(COLOR_RED);
        gotoxy(0, 4);
        printf("\nMaximum number of accounts reached. Cannot create new account.\n");
        setcolor(COLOR_DEFAULT);
        return;
    }

    Account new_account;
    memset(&new_account, 0, sizeof(Account));

    setcolor(COLOR_YELLOW);
    gotoxy(0, 0);
    printCentered("Create New Account", 0, '=', COLOR_YELLOW);
    setcolor(COLOR_DEFAULT);
    printf("\n"); 

    printf("Enter First Name: ");
    fgets(new_account.first_name, MAX_STRING_LEN, stdin);
    sanitizeInput(new_account.first_name, MAX_STRING_LEN);

    printf("Enter Last Name: ");
    fgets(new_account.last_name, MAX_STRING_LEN, stdin);
    sanitizeInput(new_account.last_name, MAX_STRING_LEN);

    printf("Enter Father's Full Name: ");
    fgets(new_account.father_name, MAX_STRING_LEN, stdin);
    sanitizeInput(new_account.father_name, MAX_STRING_LEN);

    printf("Enter Mother's Full Name: ");
    fgets(new_account.mother_name, MAX_STRING_LEN, stdin);
    sanitizeInput(new_account.mother_name, MAX_STRING_LEN);

    char aadhar_raw[ADHAAR_LEN + 16];
    printf("Enter Aadhar Card Number (12 digits): ");
    fgets(aadhar_raw, sizeof(aadhar_raw), stdin);
    sanitizeInput(aadhar_raw, sizeof(aadhar_raw));
    while (!isValidDigits(aadhar_raw, ADHAAR_LEN)) {
        setcolor(COLOR_RED);
        printf("Invalid Aadhar. Must be exactly %d digits. Re-enter: ", ADHAAR_LEN);
        setcolor(COLOR_DEFAULT);
        fgets(aadhar_raw, sizeof(aadhar_raw), stdin);
        sanitizeInput(aadhar_raw, sizeof(aadhar_raw));
    }
    strncpy(new_account.aadhar_number, aadhar_raw, ADHAAR_LEN);
    new_account.aadhar_number[ADHAAR_LEN] = '\0';

    char mobile_raw[MOBILE_LEN + 16];
    printf("Enter Mobile Number (10 digits): ");
    fgets(mobile_raw, sizeof(mobile_raw), stdin);
    sanitizeInput(mobile_raw, sizeof(mobile_raw));
    while (!isValidDigits(mobile_raw, MOBILE_LEN)) {
        setcolor(COLOR_RED);
        printf("Invalid Mobile. Must be exactly %d digits. Re-enter: ", MOBILE_LEN);
        setcolor(COLOR_DEFAULT);
        fgets(mobile_raw, sizeof(mobile_raw), stdin);
        sanitizeInput(mobile_raw, sizeof(mobile_raw));
    }
    strncpy(new_account.mobile_number, mobile_raw, MOBILE_LEN);
    new_account.mobile_number[MOBILE_LEN] = '\0';

    printf("Enter Email Address: ");
    fgets(new_account.email, MAX_STRING_LEN, stdin);
    sanitizeInput(new_account.email, MAX_STRING_LEN);
    while (!isValidEmail(new_account.email)) {
        setcolor(COLOR_RED);
        printf("Invalid Email format (must contain @ and valid domain). Re-enter: ");
        setcolor(COLOR_DEFAULT);
        fgets(new_account.email, MAX_STRING_LEN, stdin);
        sanitizeInput(new_account.email, MAX_STRING_LEN);
    }

    printf("Enter Date of Birth (DD MM YYYY): ");
    if (scanf("%d %d %d", &new_account.dob_day, &new_account.dob_month, &new_account.dob_year) != 3 ||
        new_account.dob_day < 1 || new_account.dob_day > 31 ||
        new_account.dob_month < 1 || new_account.dob_month > 12 ||
        new_account.dob_year < 1900 || new_account.dob_year > 2026) {
        setcolor(COLOR_RED);
        printf("Invalid DOB format or range. Please use DD MM YYYY and a realistic year.\n");
        setcolor(COLOR_DEFAULT);
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    printf("Enter Address: ");
    fgets(new_account.address, MAX_STRING_LEN, stdin);
    sanitizeInput(new_account.address, MAX_STRING_LEN);

    printf("Enter Account Type (e.g., Savings, Current): ");
    fgets(new_account.account_type, MAX_STRING_LEN, stdin);
    sanitizeInput(new_account.account_type, MAX_STRING_LEN);

    printf("Choose a Username: ");
    fgets(new_account.username, MAX_STRING_LEN, stdin);
    sanitizeInput(new_account.username, MAX_STRING_LEN);
    while (strlen(new_account.username) < 3 || findAccountIndexByUsername(new_account.username) != -1) {
        setcolor(COLOR_RED);
        if (strlen(new_account.username) < 3)
            printf("Username too short (min 3 chars). Re-enter: ");
        else
            printf("Username already taken. Please choose another: ");
        setcolor(COLOR_DEFAULT);
        fgets(new_account.username, MAX_STRING_LEN, stdin);
        sanitizeInput(new_account.username, MAX_STRING_LEN);
    }

    char password_raw[MAX_STRING_LEN];
    printf("Create Password (max %d chars): ", MAX_STRING_LEN - 1);
    getMaskedInput(password_raw, MAX_STRING_LEN);
    sanitizeInput(password_raw, MAX_STRING_LEN);
    while (strlen(password_raw) < 4) {
        setcolor(COLOR_RED);
        printf("Password too short (minimum 4 characters). Re-enter: ");
        setcolor(COLOR_DEFAULT);
        getMaskedInput(password_raw, MAX_STRING_LEN);
        sanitizeInput(password_raw, MAX_STRING_LEN);
    }
    hashAndStore(new_account.password_hashed, password_raw);

    printf("Enter initial deposit amount: ");
    if (scanf("%lf", &new_account.balance) != 1 || new_account.balance < 0 || isnan(new_account.balance) || isinf(new_account.balance)) {
        setcolor(COLOR_RED);
        printf("Invalid amount. Initial deposit must be a valid non-negative number.\n");
        setcolor(COLOR_DEFAULT);
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    new_account.is_active = true;
    new_account.account_number = generateAccountNumber();

    accounts[num_accounts] = new_account;
    AccountBST_root = insertAccountIntoBST(AccountBST_root, new_account.account_number, num_accounts);
    num_accounts++;
    saveAccounts();

    setcolor(COLOR_GREEN);
    printf("\nAccount created successfully!\n");
    printf("Your Account Number: %lld\n", new_account.account_number);
    setcolor(COLOR_YELLOW);
    printf("Please remember your Account Number and Password for login.\n");
    setcolor(COLOR_DEFAULT);
}

void userLogin() {
    static int failed_attempts = 0;
    if (failed_attempts >= MAX_LOGIN_ATTEMPTS) {
        setcolor(COLOR_RED);
        printf("\nRate limit triggered: Too many failed attempts. Security cooldown active...\n");
        setcolor(COLOR_DEFAULT);
        secureSleep(LOCKOUT_PENALTY_SEC);
        failed_attempts = 0;
    }

    long long acc_num;
    char username_input_login[MAX_STRING_LEN];
    char password_raw[MAX_STRING_LEN];
    int login_choice;
    int index = -1;

    setcolor(COLOR_YELLOW);
    gotoxy(0, 0);
    printCentered("User Login", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT);
    printf("\n");

    printf("Login using:\n");
    printf("1. Account Number\n");
    printf("2. Username\n");
    printf("Enter your choice (1 or 2): ");
    if (scanf("%d", &login_choice) != 1) {
        setcolor(COLOR_RED);
        printf("Invalid input. Please enter 1 or 2.\n");
        setcolor(COLOR_DEFAULT);
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (login_choice == 1) {
        printf("Enter Account Number: ");
        if (scanf("%lld", &acc_num) != 1) {
            setcolor(COLOR_RED);
            printf("Invalid Account Number format.\n");
            setcolor(COLOR_DEFAULT);
            clearInputBuffer();
            return;
        }
        clearInputBuffer();
        index = findAccountIndex(acc_num);
    } else if (login_choice == 2) {
        printf("Enter Username: ");
        fgets(username_input_login, MAX_STRING_LEN, stdin);
        sanitizeInput(username_input_login, MAX_STRING_LEN);
        index = findAccountIndexByUsername(username_input_login);
    } else {
        setcolor(COLOR_RED);
        printf("Invalid login choice. Please enter 1 or 2.\n");
        setcolor(COLOR_DEFAULT);
        return;
    }

    if (index != -1 && accounts[index].is_active) {
        printf("Enter Password: ");
        getMaskedInput(password_raw, MAX_STRING_LEN);
        sanitizeInput(password_raw, MAX_STRING_LEN);

        if (verifyPassword(accounts[index].password_hashed, password_raw, &accounts[index])) {
            failed_attempts = 0;
            saveAccounts();
            setcolor(COLOR_GREEN);
            printf("\nLogin successful! Welcome, %s %s.\n", accounts[index].first_name, accounts[index].last_name);
            setcolor(COLOR_DEFAULT);
            pauseExecution();
            userDashboard(index);
        } else {
            failed_attempts++;
            setcolor(COLOR_RED);
            printf("\nInvalid credentials. Please try again. (Attempts remaining: %d)\n", MAX_LOGIN_ATTEMPTS - failed_attempts);
            setcolor(COLOR_DEFAULT);
        }
    } else {
        failed_attempts++;
        setcolor(COLOR_RED);
        printf("\nInvalid credentials or account inactive. (Attempts remaining: %d)\n", MAX_LOGIN_ATTEMPTS - failed_attempts);
        setcolor(COLOR_DEFAULT);
    }
}

void userDashboard(int account_index) {
    int choice;
    do {
        clearScreen();
        setcolor(COLOR_GREEN);
        gotoxy(0, 0);
        printCentered("User Dashboard", 0, '=', COLOR_GREEN);
        setcolor(COLOR_YELLOW);
        gotoxy(0, 3);
        printf("Account Holder: %s %s | Acc No: %lld | Balance: %.2lf\n\n",
               accounts[account_index].first_name, accounts[account_index].last_name,
               accounts[account_index].account_number, accounts[account_index].balance);
        setcolor(COLOR_DEFAULT);

        printf("1. Deposit Money\n");
        printf("2. Withdraw Money\n");
        printf("3. Transfer Money\n");
        printf("4. Check Balance\n");
        printf("5. Show Transactions\n");
        printf("6. Show User Details\n");
        printf("7. Logout\n");
        printf("8. Exit Application\n");
        printBorder(25, '-', COLOR_DEFAULT);
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            setcolor(COLOR_RED);
            printf("\nInvalid input. Please enter a number.\n");
            setcolor(COLOR_DEFAULT);
            clearInputBuffer();
            pauseExecution();
            continue;
        }
        clearInputBuffer();

        switch (choice) {
            case 1: clearScreen(); depositMoney(account_index); pauseExecution(); break;
            case 2: clearScreen(); withdrawMoney(account_index); pauseExecution(); break;
            case 3: clearScreen(); transferMoney(account_index); pauseExecution(); break;
            case 4: clearScreen(); checkBalance(account_index); pauseExecution(); break;
            case 5: clearScreen(); showTransactions(accounts[account_index].account_number); pauseExecution(); break;
            case 6: clearScreen(); showUserDetails(account_index); pauseExecution(); break;
            case 7: setcolor(COLOR_YELLOW); printf("\nLogging out...\n"); setcolor(COLOR_DEFAULT); break;
            case 8: exitSystem(); break;
            default:
                setcolor(COLOR_RED);
                printf("\nInvalid choice. Please try again.\n");
                setcolor(COLOR_DEFAULT);
                pauseExecution();
                break;
        }
    } while (choice != 7);
}

void depositMoney(int account_index) {
    double amount;
    setcolor(COLOR_YELLOW);
    gotoxy(0, 0);
    printCentered("Deposit Money", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT);
    printf("\n");

    printf("Account Holder: %s %s\n", accounts[account_index].first_name, accounts[account_index].last_name);
    printf("Current Balance: %.2lf\n", accounts[account_index].balance);
    printf("Enter amount to deposit: ");
    if (scanf("%lf", &amount) != 1 || amount <= 0 || isnan(amount) || isinf(amount)) {
        setcolor(COLOR_RED);
        printf("Invalid amount. Deposit amount must be a finite positive number.\n");
        setcolor(COLOR_DEFAULT);
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    accounts[account_index].balance += amount;
    saveAccounts();

    Transaction t;
    t.transaction_id = generateTransactionId();
    snprintf(t.type, sizeof(t.type), "Deposit");
    t.amount = amount;
    snprintf(t.date_time, sizeof(t.date_time), "%s", generateTimestamp());
    t.source_account_number = 0;
    t.target_account_number = accounts[account_index].account_number;
    saveTransaction(t);

    setcolor(COLOR_GREEN);
    printf("\nDeposit successful. New balance: %.2lf\n", accounts[account_index].balance);
    setcolor(COLOR_DEFAULT);
}

void withdrawMoney(int account_index) {
    double amount;
    setcolor(COLOR_YELLOW);
    gotoxy(0, 0);
    printCentered("Withdraw Money", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT);
    printf("\n");

    printf("Account Holder: %s %s\n", accounts[account_index].first_name, accounts[account_index].last_name);
    printf("Current Balance: %.2lf\n", accounts[account_index].balance);
    printf("Enter amount to withdraw: ");
    if (scanf("%lf", &amount) != 1 || amount <= 0 || isnan(amount) || isinf(amount)) {
        setcolor(COLOR_RED);
        printf("Invalid amount. Withdraw amount must be a finite positive number.\n");
        setcolor(COLOR_DEFAULT);
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (accounts[account_index].balance >= amount) {
        accounts[account_index].balance -= amount;
        saveAccounts();
        Transaction t;
        t.transaction_id = generateTransactionId();
        snprintf(t.type, sizeof(t.type), "Withdrawal");
        t.amount = amount;
        snprintf(t.date_time, sizeof(t.date_time), "%s", generateTimestamp());
        t.source_account_number = accounts[account_index].account_number;
        t.target_account_number = 0;
        saveTransaction(t);
        setcolor(COLOR_GREEN);
        printf("\nWithdrawal successful. New balance: %.2lf\n", accounts[account_index].balance);
        setcolor(COLOR_DEFAULT);
    } else {
        setcolor(COLOR_RED);
        printf("\nInsufficient balance. Current balance: %.2lf\n", accounts[account_index].balance);
        setcolor(COLOR_DEFAULT);
    }
}

void transferMoney(int source_account_index) {
    long long target_acc_num;
    double amount;
    setcolor(COLOR_YELLOW);
    gotoxy(0, 0);
    printCentered("Transfer Money", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT);
    printf("\n");

    printf("Your Current Balance: %.2lf\n", accounts[source_account_index].balance);
    printf("Enter target Account Number: ");
    if (scanf("%lld", &target_acc_num) != 1) {
        setcolor(COLOR_RED);
        printf("Invalid Account Number format.\n");
        setcolor(COLOR_DEFAULT);
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (accounts[source_account_index].account_number == target_acc_num) {
        setcolor(COLOR_RED);
        printf("\nCannot transfer money to the same account.\n");
        setcolor(COLOR_DEFAULT);
        return;
    }

    int target_index = findAccountIndex(target_acc_num);
    if (target_index == -1 || !accounts[target_index].is_active) {
        setcolor(COLOR_RED);
        printf("\nTarget account not found or is inactive.\n");
        setcolor(COLOR_DEFAULT);
        return;
    }

    printf("Enter amount to transfer: ");
    if (scanf("%lf", &amount) != 1 || amount <= 0 || isnan(amount) || isinf(amount)) {
        setcolor(COLOR_RED);
        printf("Invalid amount. Transfer amount must be a finite positive number.\n");
        setcolor(COLOR_DEFAULT);
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (accounts[source_account_index].balance >= amount) {
        accounts[source_account_index].balance -= amount;
        accounts[target_index].balance += amount;
        saveAccounts();

        Transaction t;
        t.transaction_id = generateTransactionId();
        snprintf(t.type, sizeof(t.type), "Transfer");
        t.amount = amount;
        snprintf(t.date_time, sizeof(t.date_time), "%s", generateTimestamp());
        t.source_account_number = accounts[source_account_index].account_number;
        t.target_account_number = accounts[target_index].account_number;
        saveTransaction(t);

        setcolor(COLOR_GREEN);
        printf("\nTransfer successful. %.2lf transferred to account %lld (%s %s).\n",
               amount, target_acc_num, accounts[target_index].first_name, accounts[target_index].last_name);
        printf("Your new balance: %.2lf\n", accounts[source_account_index].balance);
        setcolor(COLOR_DEFAULT);
    } else {
        setcolor(COLOR_RED);
        printf("\nInsufficient balance for transfer. Your balance: %.2lf\n", accounts[source_account_index].balance);
        setcolor(COLOR_DEFAULT);
    }
}

void checkBalance(int account_index) {
    setcolor(COLOR_YELLOW);
    gotoxy(0, 0);
    printCentered("Check Balance", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT);
    printf("\n");
    setcolor(COLOR_CYAN);
    printf("Account Holder: %s %s\n", accounts[account_index].first_name, accounts[account_index].last_name);
    printf("Account Type: %s\n", accounts[account_index].account_type);
    printf("Account Number: %lld\n", accounts[account_index].account_number);
    setcolor(COLOR_GREEN);
    printf("Current Balance: %.2lf\n", accounts[account_index].balance);
    setcolor(COLOR_DEFAULT);
}

void showTransactions(long long account_num) {
    setcolor(COLOR_YELLOW);
    gotoxy(0, 0);
    printCentered("Transaction History", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT);
    printf("\n");

    FILE *file = fopen(TRANSACTION_FILE, "r");
    if (file == NULL) {
        setcolor(COLOR_RED);
        printf("No transaction history available.\n");
        setcolor(COLOR_DEFAULT);
        return;
    }

    Transaction t;
    char line[512];
    bool found_transactions = false;
    int current_y = 6; 

    setcolor(COLOR_CYAN);
    gotoxy(5, current_y++);
    printf("%-15s %-12s %-10s %-20s %-15s %-15s\n", "Trans ID", "Type", "Amount", "Date/Time", "Source Acc", "Target Acc");
    gotoxy(5, current_y++);
    printBorder(88, '-', COLOR_CYAN);
    setcolor(COLOR_DEFAULT);

    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';
        char copy[512];
        strncpy(copy, line, sizeof(copy) - 1);
        copy[sizeof(copy) - 1] = '\0';

        char *tokens[10];
        int count = 0;
        char *p = copy;
        tokens[count++] = p;
        while (*p) {
            if (*p == '|') {
                *p = '\0';
                if (count < 10) tokens[count++] = p + 1;
            }
            p++;
        }

        if (count >= 6) {
            t.transaction_id = atoll(tokens[0]);
            strncpy(t.type, tokens[1], sizeof(t.type) - 1);
            t.amount = atof(tokens[2]);
            strncpy(t.date_time, tokens[3], sizeof(t.date_time) - 1);
            t.source_account_number = atoll(tokens[4]);
            t.target_account_number = atoll(tokens[5]);

            if (t.source_account_number == account_num || t.target_account_number == account_num) {
                found_transactions = true;
                gotoxy(5, current_y++);
                printf("%-15lld %-12s %-10.2lf %-20s %-15lld %-15lld\n",
                       t.transaction_id, t.type, t.amount, t.date_time,
                       t.source_account_number, t.target_account_number);
            }
        }
    }
    fclose(file);

    if (!found_transactions) {
        setcolor(COLOR_YELLOW);
        gotoxy(25, current_y++);
        printf("No transactions found for this account.\n");
        setcolor(COLOR_DEFAULT);
    }
    gotoxy(5, current_y++);
    printBorder(88, '-', COLOR_CYAN);
}

void showUserDetails(int account_index) {
    Account *acc = &accounts[account_index];
    char masked_aadhar[ADHAAR_LEN + 1];
    char masked_mobile[MOBILE_LEN + 1];
    char masked_email[MAX_STRING_LEN];

    setcolor(COLOR_YELLOW);
    gotoxy(0, 0);
    printCentered("Your Account Details", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT);
    printf("\n");

    maskSensitiveData(masked_aadhar, acc->aadhar_number, 0, 4, '*');
    maskSensitiveData(masked_mobile, acc->mobile_number, 0, 4, '*');
    maskSensitiveData(masked_email, acc->email, 0, 14, '*');

    setcolor(COLOR_CYAN);
    printf("Account Holder: %s %s\n", acc->first_name, acc->last_name);
    printf("Username: %s\n", acc->username);
    printf("Account Number: %lld\n", acc->account_number);
    printf("Father's Name: %s\n", acc->father_name);
    printf("Mother's Name: %s\n", acc->mother_name);
    printf("Aadhar Card Number: %s\n", masked_aadhar);
    printf("Mobile Number: %s\n", masked_mobile);
    printf("Email Address: %s\n", masked_email);
    printf("Date of Birth: %02d-%02d-%04d\n", acc->dob_day, acc->dob_month, acc->dob_year);
    printf("Address: %s\n", acc->address);
    printf("Account Type: %s\n", acc->account_type);
    setcolor(COLOR_DEFAULT);
}

void adminLogin() {
    static int admin_failed_attempts = 0;
    if (admin_failed_attempts >= MAX_LOGIN_ATTEMPTS) {
        setcolor(COLOR_RED);
        printf("\nAdmin rate limit triggered: Lockout cooldown active...\n");
        setcolor(COLOR_DEFAULT);
        secureSleep(LOCKOUT_PENALTY_SEC);
        admin_failed_attempts = 0;
    }

    char username_input[MAX_STRING_LEN];
    char password_input[MAX_STRING_LEN];

    setcolor(COLOR_YELLOW);
    gotoxy(0, 0);
    printCentered("Admin Login", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT);
    printf("\n");

    printf("Enter Admin Username: ");
    fgets(username_input, MAX_STRING_LEN, stdin);
    sanitizeInput(username_input, MAX_STRING_LEN);

    printf("Enter Admin Password: ");
    getMaskedInput(password_input, MAX_STRING_LEN);
    sanitizeInput(password_input, MAX_STRING_LEN);

    const char *expected_user = getAdminUsername();
    const char *expected_pass = getAdminPassword();

    bool user_match = constantTimeCompare(username_input, expected_user);
    bool pass_match = constantTimeCompare(password_input, expected_pass);

    if (user_match && pass_match) {
        admin_failed_attempts = 0;
        setcolor(COLOR_GREEN);
        printf("\nAdmin login successful!\n");
        setcolor(COLOR_DEFAULT);
        pauseExecution();
        adminDashboard();
    } else {
        admin_failed_attempts++;
        setcolor(COLOR_RED);
        printf("\nInvalid Admin credentials. (Attempts remaining: %d)\n", MAX_LOGIN_ATTEMPTS - admin_failed_attempts);
        setcolor(COLOR_DEFAULT);
    }
}

void adminDashboard() {
    int choice;
    do {
        clearScreen();
        setcolor(COLOR_MAGENTA);
        gotoxy(0, 0);
        printCentered("Admin Dashboard", 0, '=', COLOR_MAGENTA); 
        setcolor(COLOR_DEFAULT);
        printf("\n");

        printf("1. View All User Accounts\n");
        printf("2. Delete User Account\n");
        printf("3. Reset User Password\n");
        printf("4. Logout\n");
        printf("5. Exit Application\n");
        printBorder(28, '-', COLOR_DEFAULT);
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            setcolor(COLOR_RED);
            printf("\nInvalid input. Please enter a number.\n");
            setcolor(COLOR_DEFAULT);
            clearInputBuffer();
            pauseExecution();
            continue;
        }
        clearInputBuffer();

        switch (choice) {
            case 1: clearScreen(); viewAllAccountsAdmin(); pauseExecution(); break;
            case 2: clearScreen(); deleteUserAccountAdmin(); pauseExecution(); break;
            case 3: clearScreen(); resetUserPasswordAdmin(); pauseExecution(); break;
            case 4: setcolor(COLOR_YELLOW); printf("\nLogging out from Admin...\n"); setcolor(COLOR_DEFAULT); break;
            case 5: exitSystem(); break;
            default:
                setcolor(COLOR_RED);
                printf("\nInvalid choice. Please try again.\n");
                setcolor(COLOR_DEFAULT);
                pauseExecution();
                break;
        }
    } while (choice != 4);
}

void viewAllAccountsAdmin() {
    setcolor(COLOR_MAGENTA);
    gotoxy(0, 0);
    printCentered("All Active User Accounts", 0, '=', COLOR_MAGENTA); 
    setcolor(COLOR_DEFAULT);
    printf("\n");
    if (num_accounts == 0) {
        setcolor(COLOR_YELLOW);
        printf("No accounts registered yet.\n");
        setcolor(COLOR_DEFAULT);
        return;
    }

    bool found_active = false;
    int current_y = 6;

    setcolor(COLOR_CYAN);
    gotoxy(5, current_y++);
    printf("%-15s %-25s %-10s %-15s %s\n", "Account No.", "Name", "Balance", "Type", "Status");
    gotoxy(5, current_y++);
    printBorder(80, '-', COLOR_CYAN);
    setcolor(COLOR_DEFAULT);

    for (int i = 0; i < num_accounts; i++) {
        if (accounts[i].is_active) {
            found_active = true;
            gotoxy(5, current_y++);
            printf("%-15lld %-25s %-10.2lf %-15s %s\n",
                   accounts[i].account_number,
                   accounts[i].first_name,
                   accounts[i].balance,
                   accounts[i].account_type,
                   accounts[i].is_active ? "Active" : "Inactive");
        }
    }
    if (!found_active) {
        setcolor(COLOR_YELLOW);
        gotoxy(25, current_y++);
        printf("No active accounts found.\n");
        setcolor(COLOR_DEFAULT);
    }
    gotoxy(5, current_y++);
    printBorder(80, '-', COLOR_CYAN);
}

void deleteUserAccountAdmin() {
    long long acc_num_to_delete;
    setcolor(COLOR_MAGENTA);
    gotoxy(0, 0);
    printCentered("Delete User Account", 0, '=', COLOR_MAGENTA); 
    setcolor(COLOR_DEFAULT);
    printf("\n");

    printf("Enter Account Number to delete: ");
    if (scanf("%lld", &acc_num_to_delete) != 1) {
        setcolor(COLOR_RED);
        printf("Invalid Account Number format.\n");
        setcolor(COLOR_DEFAULT);
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    int index_to_delete = findAccountIndex(acc_num_to_delete);

    if (index_to_delete != -1) {
        char confirm_char;
        setcolor(COLOR_YELLOW);
        printf("Are you sure you want to delete account %lld (Holder: %s %s)? (y/n): ",
               accounts[index_to_delete].account_number,
               accounts[index_to_delete].first_name,
               accounts[index_to_delete].last_name);
        setcolor(COLOR_DEFAULT);
        if (scanf(" %c", &confirm_char) != 1) {
            setcolor(COLOR_RED);
            printf("Invalid input.\n");
            setcolor(COLOR_DEFAULT);
            clearInputBuffer();
            return;
        }
        clearInputBuffer();

        if (tolower((unsigned char)confirm_char) == 'y') {
            accounts[index_to_delete].is_active = false;
            saveAccounts();
            setcolor(COLOR_GREEN);
            printf("\nAccount %lld deleted (marked inactive) successfully.\n", acc_num_to_delete);
            setcolor(COLOR_DEFAULT);
        } else {
            setcolor(COLOR_YELLOW);
            printf("\nAccount deletion cancelled.\n");
            setcolor(COLOR_DEFAULT);
        }
    } else {
        setcolor(COLOR_RED);
        printf("\nAccount %lld not found or already inactive.\n", acc_num_to_delete);
        setcolor(COLOR_DEFAULT);
    }
}

void resetUserPasswordAdmin() {
    long long acc_num_to_reset;
    setcolor(COLOR_MAGENTA);
    gotoxy(0, 0);
    printCentered("Reset User Password", 0, '=', COLOR_MAGENTA); 
    setcolor(COLOR_DEFAULT);
    printf("\n");

    printf("Enter Account Number whose password you want to reset: ");
    if (scanf("%lld", &acc_num_to_reset) != 1) {
        setcolor(COLOR_RED);
        printf("Invalid Account Number format.\n");
        setcolor(COLOR_DEFAULT);
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    int index_to_reset = findAccountIndex(acc_num_to_reset);

    if (index_to_reset != -1) {
        char new_password_raw[MAX_STRING_LEN];
        setcolor(COLOR_YELLOW);
        printf("Account found: %s %s (Account %lld).\n",
               accounts[index_to_reset].first_name, accounts[index_to_reset].last_name, acc_num_to_reset);
        setcolor(COLOR_DEFAULT);
        printf("Enter new password (max %d chars): ", MAX_STRING_LEN - 1);
        getMaskedInput(new_password_raw, MAX_STRING_LEN);
        sanitizeInput(new_password_raw, MAX_STRING_LEN);
        while (strlen(new_password_raw) < 4) {
            setcolor(COLOR_RED);
            printf("Password must be at least 4 characters. Re-enter: ");
            setcolor(COLOR_DEFAULT);
            getMaskedInput(new_password_raw, MAX_STRING_LEN);
            sanitizeInput(new_password_raw, MAX_STRING_LEN);
        }
        hashAndStore(accounts[index_to_reset].password_hashed, new_password_raw);
        saveAccounts();
        setcolor(COLOR_GREEN);
        printf("\nPassword for account %lld reset successfully.\n", acc_num_to_reset);
        setcolor(COLOR_DEFAULT);
    } else {
        setcolor(COLOR_RED);
        printf("\nAccount %lld not found or is inactive.\n", acc_num_to_reset);
        setcolor(COLOR_DEFAULT);
    }
}

void exitSystem() {
    clearScreen();
    setcolor(COLOR_YELLOW);
    gotoxy(0, 5);
    printCentered("Thank You for Using MONEYMATRIX!", 0, '*', COLOR_YELLOW); 
    setcolor(COLOR_GREEN);
    gotoxy(0, 8);
    printCentered("Goodbye!", 0, ' ', COLOR_GREEN); 
    setcolor(COLOR_DEFAULT);
    gotoxy(0, 10);
    printf("\n\n");
    freeBST(AccountBST_root);
    exit(0);
}

#ifdef _WIN32
void gotoxy(int x, int y) {
    COORD c;
    c.X = (SHORT)x;
    c.Y = (SHORT)y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void setcolor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, (WORD)color);
}
#endif
