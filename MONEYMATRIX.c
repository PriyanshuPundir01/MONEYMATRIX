#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#endif


#define MAX_ACCOUNTS 100
#define MAX_STRING_LEN 50
#define ADHAAR_LEN 12
#define MOBILE_LEN 10
#define ACCOUNT_FILE "accounts.txt"
#define TRANSACTION_FILE "transactions.txt"
#define ADMIN_USERNAME "admin"
#define ADMIN_PASSWORD "admin123"
#define HASH_SALT "banking_system_salt" 


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
    char password_hashed[MAX_STRING_LEN * 2]; 
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

unsigned long hashString(const char *str);
void hashAndStore(char *dest, const char *source); // Still used for passwords
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
#endif

BSTNode* createBSTNode(long long acc_num, int acc_idx);
BSTNode* insertAccountIntoBST(BSTNode* root, long long acc_num, int acc_idx);
int searchAccountInBST(BSTNode* root, long long acc_num);
void freeBST(BSTNode* root);


int main() {
    loadAccounts();
    int choice;
    do {
        clearScreen();
        displayMainMenu();
        setcolor(COLOR_DEFAULT);
        gotoxy(25, 16); // Position for user choice input
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            setcolor(COLOR_RED);
            gotoxy(25, 17); printf("Invalid input. Please enter a number.\n");
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
            default: setcolor(COLOR_RED); gotoxy(25, 17); printf("Invalid choice. Please try again.\n"); pauseExecution();
        }
    } while (choice != 4);
    return 0;
}



BSTNode* createBSTNode(long long acc_num, int acc_idx) {
    BSTNode* newNode = (BSTNode*)malloc(sizeof(BSTNode));
    if (newNode == NULL) { setcolor(COLOR_RED); printf("Memory allocation failed for BST node.\n"); setcolor(COLOR_DEFAULT); exit(EXIT_FAILURE); }
    newNode->account_number = acc_num; newNode->account_index = acc_idx; newNode->left = NULL; newNode->right = NULL;
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
    if (acc_num == root->account_number) { if (accounts[root->account_index].is_active) return root->account_index; else return -1; }
    if (acc_num < root->account_number) return searchAccountInBST(root->left, acc_num);
    else return searchAccountInBST(root->right, acc_num);
}

void freeBST(BSTNode* root) {
    if (root != NULL) { freeBST(root->left); freeBST(root->right); free(root); }
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
    int text_len = strlen(text);
   
    int effective_width = (width == 0) ? (text_len + 4) : width; 

    if (text_len >= effective_width) {
        printf("%s\n", text);
        return;
    }

    int padding_left = (effective_width - text_len) / 2;
    int padding_right = effective_width - text_len - padding_left;
    int i;

    if (border_char != '\0') {
        for (i = 0; i < effective_width; i++) {
            printf("%c", border_char);
        }
        printf("\n");
    }

    for (i = 0; i < padding_left; i++) printf(" ");
    printf("%s", text);
    for (i = 0; i < padding_right; i++) printf(" ");
    printf("\n");

    if (border_char != '\0') {
        for (i = 0; i < effective_width; i++) {
            printf("%c", border_char);
        }
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
    setcolor(COLOR_CYAN); gotoxy(25, 23); printf("Press any key to continue..."); fflush(stdout);
    #ifdef _WIN32
        _getch();
    #else
        getchar();
    #endif
    gotoxy(0, 24); printf("\n"); setcolor(COLOR_DEFAULT);
}

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void getMaskedInput(char *buffer, int max_len) {
    int i = 0; char ch;
    while (i < max_len - 1) {
    #ifdef _WIN32
        ch = _getch();
    #else
        ch = getchar();
    #endif
        if (ch == 13) break;
        if (ch == 8 || ch == 127) { if (i > 0) { i--; printf("\b \b"); } }
        else { buffer[i++] = ch; printf("*"); }
    }
    buffer[i] = '\0'; printf("\n");
}

void toLowerCase(char *str) {
    for (int i = 0; str[i]; i++) str[i] = tolower((unsigned char)str[i]);
}


unsigned long hashString(const char *str) {
    unsigned long hash = 5381; int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;
    return hash;
}

void hashAndStore(char *dest, const char *source) {
    char combined_str[MAX_STRING_LEN + sizeof(HASH_SALT)];
    snprintf(combined_str, sizeof(combined_str), "%s%s", source, HASH_SALT);
    unsigned long h = hashString(combined_str);
    snprintf(dest, MAX_STRING_LEN * 2, "%lx", h);
}

void maskSensitiveData(char *masked_str, const char *original_str, int visible_chars_start, int visible_chars_end, char mask_char) {
    int len = strlen(original_str);
    if (len == 0) { strcpy(masked_str, ""); return; }

   
    if (visible_chars_start < 0) visible_chars_start = 0;
    if (visible_chars_end < 0) visible_chars_end = 0;

    
    if (visible_chars_start + visible_chars_end >= len) {
        strcpy(masked_str, original_str);
        return;
    }

    int i, j = 0;

    
    for (i = 0; i < visible_chars_start; i++) {
        masked_str[j++] = original_str[i];
    }

    for (i = visible_chars_start; i < len - visible_chars_end; i++) {
        masked_str[j++] = mask_char;
    }

    for (i = len - visible_chars_end; i < len; i++) {
        masked_str[j++] = original_str[i];
    }
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
    char line[512];
    while (fgets(line, sizeof(line), file) != NULL && num_accounts < MAX_ACCOUNTS) {
        Account *acc = &accounts[num_accounts];
       
        int scan_count = sscanf(line, "%lld|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%d-%d-%d|%[^|]|%[^|]|%[^|]|%lf|%d\n",
                                &acc->account_number,
                                acc->first_name, acc->last_name,
                                acc->father_name, acc->mother_name,
                                acc->aadhar_number, acc->mobile_number, 
                                acc->email,
                                &acc->dob_day, &acc->dob_month, &acc->dob_year,
                                acc->address, acc->account_type,
                                acc->username, acc->password_hashed,
                                &acc->balance, (int*)&acc->is_active);

        if (scan_count == 16) {
            if (acc->is_active) {
                AccountBST_root = insertAccountIntoBST(AccountBST_root, acc->account_number, num_accounts);
            }
            num_accounts++;
        } else {
            setcolor(COLOR_RED);
            printf("Error reading line from %s: %s (Expected 16 items, got %d)\n", ACCOUNT_FILE, line, scan_count);
            setcolor(COLOR_DEFAULT);
        }
    }
    fclose(file);
}


void saveAccounts() {
    FILE *file = fopen(ACCOUNT_FILE, "w");
    if (file == NULL) {
        setcolor(COLOR_RED);
        perror("Error opening file for saving");
        printf("Could not save account data.\n");
        setcolor(COLOR_DEFAULT);
        return;
    }

    for (int i = 0; i < num_accounts; i++) {
        Account *acc = &accounts[i];
       
        fprintf(file, "%lld|%s|%s|%s|%s|%s|%s|%s|%d-%d-%d|%s|%s|%s|%.2lf|%d\n",
                acc->account_number,
                acc->first_name, acc->last_name,
                acc->father_name, acc->mother_name,
                acc->aadhar_number, acc->mobile_number, 
                acc->email,
                acc->dob_day, acc->dob_month, acc->dob_year,
                acc->address, acc->account_type,
                acc->username, acc->password_hashed,
                acc->balance, (int)acc->is_active);
    }
    fclose(file);
}


void saveTransaction(Transaction t) {
    FILE *file = fopen(TRANSACTION_FILE, "a");
    if (file == NULL) { setcolor(COLOR_RED); perror("Error opening transaction file for saving"); setcolor(COLOR_DEFAULT); return; }
    fprintf(file, "%lld|%s|%.2lf|%s|%lld|%lld\n", t.transaction_id, t.type, t.amount, t.date_time, t.source_account_number, t.target_account_number);
    fclose(file);
}

long long generateAccountNumber() {
    static bool seeded = false;
    if (!seeded) { srand((unsigned int)time(NULL)); seeded = true; }
    long long acc_num; bool unique = false;
    do {
        acc_num = (long long)rand() * rand();
        if (acc_num < 0) acc_num = -acc_num;
        acc_num = acc_num % 9000000000LL + 1000000000LL;
        if (searchAccountInBST(AccountBST_root, acc_num) == -1) unique = true;
    } while (!unique);
    return acc_num;
}

long long generateTransactionId() { return (long long)time(NULL) * 1000 + (rand() % 1000); }

char* generateTimestamp() {
    static char timestamp[MAX_STRING_LEN];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    return timestamp;
}

int findAccountIndex(long long acc_num) { return searchAccountInBST(AccountBST_root, acc_num); }

int findAccountIndexByUsername(const char* username_input) {
    char temp_input_username[MAX_STRING_LEN];
    char temp_stored_username[MAX_STRING_LEN];
    strcpy(temp_input_username, username_input); toLowerCase(temp_input_username);
    for (int i = 0; i < num_accounts; i++) {
        if (accounts[i].is_active) {
            strcpy(temp_stored_username, accounts[i].username); toLowerCase(temp_stored_username);
            if (strcmp(temp_stored_username, temp_input_username) == 0) return i;
        }
    }
    return -1;
}

void createUserAccount() {
    if (num_accounts >= MAX_ACCOUNTS) {
        setcolor(COLOR_RED);
        gotoxy(0, 4); printf("\nMaximum number of accounts reached. Cannot create new account.\n");
        setcolor(COLOR_DEFAULT);
        return;
    }

    Account new_account;

    setcolor(COLOR_YELLOW);
    gotoxy(0, 0); printCentered("Create New Account", 0, '=', COLOR_YELLOW); // Dynamic width
    setcolor(COLOR_DEFAULT);
    printf("\n"); 

    printf("Enter First Name: ");
    fgets(new_account.first_name, MAX_STRING_LEN, stdin);
    new_account.first_name[strcspn(new_account.first_name, "\n")] = 0;

    printf("Enter Last Name: ");
    fgets(new_account.last_name, MAX_STRING_LEN, stdin);
    new_account.last_name[strcspn(new_account.last_name, "\n")] = 0;

    printf("Enter Father's Full Name: ");
    fgets(new_account.father_name, MAX_STRING_LEN, stdin);
    new_account.father_name[strcspn(new_account.father_name, "\n")] = 0;

    printf("Enter Mother's Full Name: ");
    fgets(new_account.mother_name, MAX_STRING_LEN, stdin);
    new_account.mother_name[strcspn(new_account.mother_name, "\n")] = 0;

    char aadhar_raw[ADHAAR_LEN + 5];
    printf("Enter Aadhar Card Number (12 digits): ");
    fgets(aadhar_raw, sizeof(aadhar_raw), stdin);
    aadhar_raw[strcspn(aadhar_raw, "\n")] = 0;
    while (strlen(aadhar_raw) != ADHAAR_LEN || ({bool all_digits = true; for (int i = 0; i < strlen(aadhar_raw); i++) if (!isdigit(aadhar_raw[i])) {all_digits = false; break;} !all_digits;})) {
        setcolor(COLOR_RED); printf("Invalid Aadhar. Must be exactly %d digits and all digits. Re-enter: ", ADHAAR_LEN); setcolor(COLOR_DEFAULT);
        fgets(aadhar_raw, sizeof(aadhar_raw), stdin); aadhar_raw[strcspn(aadhar_raw, "\n")] = 0;
    }
    strcpy(new_account.aadhar_number, aadhar_raw); 
    char mobile_raw[MOBILE_LEN + 5];
    printf("Enter Mobile Number (10 digits): ");
    fgets(mobile_raw, sizeof(mobile_raw), stdin);
    mobile_raw[strcspn(mobile_raw, "\n")] = 0;
    while (strlen(mobile_raw) != MOBILE_LEN || ({bool all_digits = true; for (int i = 0; i < strlen(mobile_raw); i++) if (!isdigit(mobile_raw[i])) {all_digits = false; break;} !all_digits;})) {
        setcolor(COLOR_RED); printf("Invalid Mobile. Must be exactly %d digits and all digits. Re-enter: ", MOBILE_LEN); setcolor(COLOR_DEFAULT);
        fgets(mobile_raw, sizeof(mobile_raw), stdin); mobile_raw[strcspn(mobile_raw, "\n")] = 0;
    }
    strcpy(new_account.mobile_number, mobile_raw); 
    printf("Enter Email Address: ");
    fgets(new_account.email, MAX_STRING_LEN, stdin);
    new_account.email[strcspn(new_account.email, "\n")] = 0;

    printf("Enter Date of Birth (DD MMYYYY): ");
    if (scanf("%d %d %d", &new_account.dob_day, &new_account.dob_month, &new_account.dob_year) != 3 ||
        new_account.dob_day < 1 || new_account.dob_day > 31 || new_account.dob_month < 1 || new_account.dob_month > 12 ||
        new_account.dob_year < 1900 || new_account.dob_year > 2025) {
        setcolor(COLOR_RED); printf("Invalid DOB format or range. Please use DD MMYYYY (e.g., 01 01 2000) and a realistic year.\n"); setcolor(COLOR_DEFAULT); clearInputBuffer(); return;
    }
    clearInputBuffer();

    printf("Enter Address: ");
    fgets(new_account.address, MAX_STRING_LEN, stdin);
    new_account.address[strcspn(new_account.address, "\n")] = 0;

    printf("Enter Account Type (e.g., Savings, Current): ");
    fgets(new_account.account_type, MAX_STRING_LEN, stdin);
    new_account.account_type[strcspn(new_account.account_type, "\n")] = 0;

    printf("Choose a Username: ");
    fgets(new_account.username, MAX_STRING_LEN, stdin);
    new_account.username[strcspn(new_account.username, "\n")] = 0;

    char password_raw[MAX_STRING_LEN];
    printf("Create Password (max %d chars): ", MAX_STRING_LEN - 1);
    getMaskedInput(password_raw, MAX_STRING_LEN);
    hashAndStore(new_account.password_hashed, password_raw);

    printf("Enter initial deposit amount: ");
    if (scanf("%lf", &new_account.balance) != 1 || new_account.balance < 0) {
        setcolor(COLOR_RED); printf("Invalid amount. Initial deposit must be a non-negative number.\n"); setcolor(COLOR_DEFAULT); clearInputBuffer(); return;
    }
    clearInputBuffer();

    new_account.is_active = true;
    new_account.account_number = generateAccountNumber();

    accounts[num_accounts] = new_account;
    AccountBST_root = insertAccountIntoBST(AccountBST_root, new_account.account_number, num_accounts);
    num_accounts++;
    saveAccounts();

    setcolor(COLOR_GREEN); printf("\nAccount created successfully!\n");
    printf("Your Account Number: %lld\n", new_account.account_number);
    setcolor(COLOR_YELLOW); printf("Please remember your Account Number and Password for login.\n"); setcolor(COLOR_DEFAULT);
}

void userLogin() {
    long long acc_num; char username_input_login[MAX_STRING_LEN]; char password_raw[MAX_STRING_LEN];
    char password_hashed_input[MAX_STRING_LEN * 2]; int login_choice; int index = -1;
    setcolor(COLOR_YELLOW); gotoxy(0, 0); printCentered("User Login", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT); printf("\n");

    printf("Login using:\n");
    printf("1. Account Number\n");
    printf("2. Username\n");
    printf("Enter your choice (1 or 2): ");
    if (scanf("%d", &login_choice) != 1) { setcolor(COLOR_RED); printf("Invalid input. Please enter 1 or 2.\n"); setcolor(COLOR_DEFAULT); clearInputBuffer(); return; }
    clearInputBuffer();

    if (login_choice == 1) {
        printf("Enter Account Number: ");
        if (scanf("%lld", &acc_num) != 1) { setcolor(COLOR_RED); printf("Invalid Account Number format.\n"); setcolor(COLOR_DEFAULT); clearInputBuffer(); return; }
        clearInputBuffer();
        index = findAccountIndex(acc_num);
    } else if (login_choice == 2) {
        printf("Enter Username: "); fgets(username_input_login, MAX_STRING_LEN, stdin); username_input_login[strcspn(username_input_login, "\n")] = 0;
        index = findAccountIndexByUsername(username_input_login);
    } else { setcolor(COLOR_RED); printf("Invalid login choice. Please enter 1 or 2.\n"); setcolor(COLOR_DEFAULT); return; }

    if (index != -1) {
        printf("Enter Password: "); getMaskedInput(password_raw, MAX_STRING_LEN);
        hashAndStore(password_hashed_input, password_raw);
        if (strcmp(accounts[index].password_hashed, password_hashed_input) == 0) {
            setcolor(COLOR_GREEN); printf("\nLogin successful! Welcome, %s %s.\n", accounts[index].first_name, accounts[index].last_name); setcolor(COLOR_DEFAULT);
            pauseExecution(); userDashboard(index);
        } else { setcolor(COLOR_RED); printf("\nInvalid Password. Please try again.\n"); setcolor(COLOR_DEFAULT); }
    } else { setcolor(COLOR_RED); printf("\nAccount not found or is inactive.\n"); setcolor(COLOR_DEFAULT); }
}

void userDashboard(int account_index) {
    int choice;
    do {
        clearScreen();
        setcolor(COLOR_GREEN); gotoxy(0, 0); printCentered("User Dashboard", 0, '=', COLOR_GREEN);
        setcolor(COLOR_YELLOW);
        gotoxy(0, 3); printf("Account Holder: %s %s | Acc No: %lld | Balance: %.2lf\n\n",
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
        if (scanf("%d", &choice) != 1) { setcolor(COLOR_RED); printf("\nInvalid input. Please enter a number.\n"); setcolor(COLOR_DEFAULT); clearInputBuffer(); pauseExecution(); continue; }
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
            default: setcolor(COLOR_RED); printf("\nInvalid choice. Please try again.\n"); setcolor(COLOR_DEFAULT); pauseExecution();
        }
    } while (choice != 7);
}

void depositMoney(int account_index) {
    double amount;
    setcolor(COLOR_YELLOW); gotoxy(0, 0); printCentered("Deposit Money", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT); printf("\n");

    printf("Account Holder: %s %s\n", accounts[account_index].first_name, accounts[account_index].last_name);
    printf("Current Balance: %.2lf\n", accounts[account_index].balance);
    printf("Enter amount to deposit: ");
    if (scanf("%lf", &amount) != 1 || amount <= 0) { setcolor(COLOR_RED); printf("Invalid amount. Deposit amount must be a positive number.\n"); setcolor(COLOR_DEFAULT); clearInputBuffer(); return; }
    clearInputBuffer();
    accounts[account_index].balance += amount; saveAccounts();

    Transaction t; t.transaction_id = generateTransactionId(); strcpy(t.type, "Deposit"); t.amount = amount; strcpy(t.date_time, generateTimestamp());
    t.source_account_number = 0; t.target_account_number = accounts[account_index].account_number; saveTransaction(t);
    setcolor(COLOR_GREEN); printf("\nDeposit successful. New balance: %.2lf\n", accounts[account_index].balance); setcolor(COLOR_DEFAULT);
}

void withdrawMoney(int account_index) {
    double amount;
    setcolor(COLOR_YELLOW); gotoxy(0, 0); printCentered("Withdraw Money", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT); printf("\n");

    printf("Account Holder: %s %s\n", accounts[account_index].first_name, accounts[account_index].last_name);
    printf("Current Balance: %.2lf\n", accounts[account_index].balance);
    printf("Enter amount to withdraw: ");
    if (scanf("%lf", &amount) != 1 || amount <= 0) { setcolor(COLOR_RED); printf("Invalid amount. Withdraw amount must be a positive number.\n"); setcolor(COLOR_DEFAULT); clearInputBuffer(); return; }
    clearInputBuffer();

    if (accounts[account_index].balance >= amount) {
        accounts[account_index].balance -= amount; saveAccounts();
        Transaction t; t.transaction_id = generateTransactionId(); strcpy(t.type, "Withdrawal"); t.amount = amount; strcpy(t.date_time, generateTimestamp());
        t.source_account_number = accounts[account_index].account_number; t.target_account_number = 0; saveTransaction(t);
        setcolor(COLOR_GREEN); printf("\nWithdrawal successful. New balance: %.2lf\n", accounts[account_index].balance); setcolor(COLOR_DEFAULT);
    } else { setcolor(COLOR_RED); printf("\nInsufficient balance. Current balance: %.2lf\n", accounts[account_index].balance); setcolor(COLOR_DEFAULT); }
}

void transferMoney(int source_account_index) {
    long long target_acc_num; double amount;
    setcolor(COLOR_YELLOW); gotoxy(0, 0); printCentered("Transfer Money", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT); printf("\n");

    printf("Your Current Balance: %.2lf\n", accounts[source_account_index].balance);
    printf("Enter target Account Number: ");
    if (scanf("%lld", &target_acc_num) != 1) { setcolor(COLOR_RED); printf("Invalid Account Number format.\n"); setcolor(COLOR_DEFAULT); clearInputBuffer(); return; }
    clearInputBuffer();
    if (accounts[source_account_index].account_number == target_acc_num) { setcolor(COLOR_RED); printf("\nCannot transfer money to the same account.\n"); setcolor(COLOR_DEFAULT); return; }
    int target_index = findAccountIndex(target_acc_num);
    if (target_index == -1) { setcolor(COLOR_RED); printf("\nTarget account not found or is inactive.\n"); setcolor(COLOR_DEFAULT); return; }
    printf("Enter amount to transfer: ");
    if (scanf("%lf", &amount) != 1 || amount <= 0) { setcolor(COLOR_RED); printf("Invalid amount. Transfer amount must be a positive number.\n"); setcolor(COLOR_DEFAULT); clearInputBuffer(); return; }
    clearInputBuffer();

    if (accounts[source_account_index].balance >= amount) {
        accounts[source_account_index].balance -= amount; accounts[target_index].balance += amount; saveAccounts();
        Transaction t; t.transaction_id = generateTransactionId(); strcpy(t.type, "Transfer"); t.amount = amount; strcpy(t.date_time, generateTimestamp());
        t.source_account_number = accounts[source_account_index].account_number; t.target_account_number = accounts[target_index].account_number; saveTransaction(t);
        setcolor(COLOR_GREEN); printf("\nTransfer successful. %.2lf transferred to account %lld (%s %s).\n", amount, target_acc_num, accounts[target_index].first_name, accounts[target_index].last_name);
        printf("Your new balance: %.2lf\n", accounts[source_account_index].balance); setcolor(COLOR_DEFAULT);
    } else { setcolor(COLOR_RED); printf("\nInsufficient balance for transfer. Your balance: %.2lf\n", accounts[source_account_index].balance); setcolor(COLOR_DEFAULT); }
}

void checkBalance(int account_index) {
    setcolor(COLOR_YELLOW); gotoxy(0, 0); printCentered("Check Balance", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT); printf("\n");
    setcolor(COLOR_CYAN);
    printf("Account Holder: %s %s\n", accounts[account_index].first_name, accounts[account_index].last_name);
    printf("Account Type: %s\n", accounts[account_index].account_type);
    printf("Account Number: %lld\n", accounts[account_index].account_number);
    setcolor(COLOR_GREEN); printf("Current Balance: %.2lf\n", accounts[account_index].balance); setcolor(COLOR_DEFAULT);
}

void showTransactions(long long account_num) {
    setcolor(COLOR_YELLOW); gotoxy(0, 0); printCentered("Transaction History", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT); printf("\n");

    FILE *file = fopen(TRANSACTION_FILE, "r");
    if (file == NULL) { setcolor(COLOR_RED); printf("No transaction history available.\n"); setcolor(COLOR_DEFAULT); return; }
    Transaction t; char line[512]; bool found_transactions = false;
    int current_y = 6; 

    setcolor(COLOR_CYAN);
    gotoxy(5, current_y++); printf("%-15s %-12s %-10s %-20s %-15s %-15s\n", "Trans ID", "Type", "Amount", "Date/Time", "Source Acc", "Target Acc");
    gotoxy(5, current_y++); printBorder(88, '-', COLOR_CYAN);
    setcolor(COLOR_DEFAULT);
    while (fgets(line, sizeof(line), file) != NULL) {
        int scan_count = sscanf(line, "%lld|%[^|]|%lf|%[^|]|%lld|%lld\n", &t.transaction_id, t.type, &t.amount, t.date_time, &t.source_account_number, &t.target_account_number);
        if (scan_count == 6) {
            if (t.source_account_number == account_num || t.target_account_number == account_num) {
                found_transactions = true;
                gotoxy(5, current_y++); printf("%-15lld %-12s %-10.2lf %-20s %-15lld %-15lld\n", t.transaction_id, t.type, t.amount, t.date_time, t.source_account_number, t.target_account_number);
            }
        }
    }
    fclose(file);
    if (!found_transactions) { setcolor(COLOR_YELLOW); gotoxy(25, current_y++); printf("No transactions found for this account.\n"); setcolor(COLOR_DEFAULT); }
    gotoxy(5, current_y++); printBorder(88, '-', COLOR_CYAN);
}

void showUserDetails(int account_index) {
    Account *acc = &accounts[account_index];
    char masked_aadhar[ADHAAR_LEN + 1];
    char masked_mobile[MOBILE_LEN + 1];
    char masked_email[MAX_STRING_LEN];

    setcolor(COLOR_YELLOW); gotoxy(0, 0); printCentered("Your Account Details", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT); printf("\n");

   
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
    printf("Date of Birth: %02d-%02d-%d\n", acc->dob_day, acc->dob_month, acc->dob_year);
    printf("Address: %s\n", acc->address);
    printf("Account Type: %s\n", acc->account_type);
    setcolor(COLOR_DEFAULT);
}


void adminLogin() {
    char username_input[MAX_STRING_LEN]; char password_input[MAX_STRING_LEN];
    setcolor(COLOR_YELLOW); gotoxy(0, 0); printCentered("Admin Login", 0, '=', COLOR_YELLOW); 
    setcolor(COLOR_DEFAULT); printf("\n");

    printf("Enter Admin Username: ");
    fgets(username_input, MAX_STRING_LEN, stdin);
    username_input[strcspn(username_input, "\n")] = 0;

    printf("Enter Admin Password: ");
    getMaskedInput(password_input, MAX_STRING_LEN);

    if (strcmp(username_input, ADMIN_USERNAME) == 0 && strcmp(password_input, ADMIN_PASSWORD) == 0) {
        setcolor(COLOR_GREEN); printf("\nAdmin login successful!\n"); setcolor(COLOR_DEFAULT); pauseExecution(); adminDashboard();
    } else { setcolor(COLOR_RED); printf("\nInvalid Admin Username or Password.\n"); setcolor(COLOR_DEFAULT); }
}

void adminDashboard() {
    int choice;
    do {
        clearScreen();
        setcolor(COLOR_MAGENTA); gotoxy(0, 0); printCentered("Admin Dashboard", 0, '=', COLOR_MAGENTA); 
        setcolor(COLOR_DEFAULT); printf("\n");

        printf("1. View All User Accounts\n");
        printf("2. Delete User Account\n");
        printf("3. Reset User Password\n");
        printf("4. Logout\n");
        printf("5. Exit Application\n");
        printBorder(28, '-', COLOR_DEFAULT);
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) { setcolor(COLOR_RED); printf("\nInvalid input. Please enter a number.\n"); setcolor(COLOR_DEFAULT); clearInputBuffer(); pauseExecution(); continue; }
        clearInputBuffer();

        switch (choice) {
            case 1: clearScreen(); viewAllAccountsAdmin(); pauseExecution(); break;
            case 2: clearScreen(); deleteUserAccountAdmin(); pauseExecution(); break;
            case 3: clearScreen(); resetUserPasswordAdmin(); pauseExecution(); break;
            case 4: setcolor(COLOR_YELLOW); printf("\nLogging out from Admin...\n"); setcolor(COLOR_DEFAULT); break;
            case 5: exitSystem(); break;
            default: setcolor(COLOR_RED); printf("\nInvalid choice. Please try again.\n"); setcolor(COLOR_DEFAULT); pauseExecution();
        }
    } while (choice != 4);
}

void viewAllAccountsAdmin() {
    setcolor(COLOR_MAGENTA); gotoxy(0, 0); printCentered("All Active User Accounts", 0, '=', COLOR_MAGENTA); 
    setcolor(COLOR_DEFAULT); printf("\n");
    if (num_accounts == 0) { setcolor(COLOR_YELLOW); printf("No accounts registered yet.\n"); setcolor(COLOR_DEFAULT); return; }

    bool found_active = false;
    int current_y = 6;

    setcolor(COLOR_CYAN);
    gotoxy(5, current_y++); printf("%-15s %-25s %-10s %-15s %s\n", "Account No.", "Name", "Balance", "Type", "Status");
    gotoxy(5, current_y++); printBorder(80, '-', COLOR_CYAN);
    setcolor(COLOR_DEFAULT);
    for (int i = 0; i < num_accounts; i++) {
        if (accounts[i].is_active) {
            found_active = true;
            gotoxy(5, current_y++); printf("%-15lld %-25s %-10.2lf %-15s %s\n",
                   accounts[i].account_number,
                   accounts[i].first_name,
                   accounts[i].balance,
                   accounts[i].account_type,
                   accounts[i].is_active ? "Active" : "Inactive");
        }
    }
    if (!found_active) { setcolor(COLOR_YELLOW); gotoxy(25, current_y++); printf("No active accounts found.\n"); setcolor(COLOR_DEFAULT); }
    gotoxy(5, current_y++); printBorder(80, '-', COLOR_CYAN);
}

void deleteUserAccountAdmin() {
    long long acc_num_to_delete;
    setcolor(COLOR_MAGENTA); gotoxy(0, 0); printCentered("Delete User Account", 0, '=', COLOR_MAGENTA); 
    setcolor(COLOR_DEFAULT); printf("\n");

    printf("Enter Account Number to delete: ");
    if (scanf("%lld", &acc_num_to_delete) != 1) { setcolor(COLOR_RED); printf("Invalid Account Number format.\n"); setcolor(COLOR_DEFAULT); clearInputBuffer(); return; }
    clearInputBuffer();

    int index_to_delete = findAccountIndex(acc_num_to_delete);

    if (index_to_delete != -1) {
        char confirm_char;
        setcolor(COLOR_YELLOW); printf("Are you sure you want to delete account %lld (Holder: %s %s)? (y/n): ", accounts[index_to_delete].account_number, accounts[index_to_delete].first_name, accounts[index_to_delete].last_name); setcolor(COLOR_DEFAULT);
        if (scanf(" %c", &confirm_char) != 1) { setcolor(COLOR_RED); printf("Invalid input.\n"); setcolor(COLOR_DEFAULT); clearInputBuffer(); return; }
        clearInputBuffer();

        if (tolower(confirm_char) == 'y') {
            accounts[index_to_delete].is_active = false; saveAccounts();
            setcolor(COLOR_GREEN); printf("\nAccount %lld deleted (marked inactive) successfully.\n", acc_num_to_delete); setcolor(COLOR_DEFAULT);
        } else { setcolor(COLOR_YELLOW); printf("\nAccount deletion cancelled.\n"); setcolor(COLOR_DEFAULT); }
    } else { setcolor(COLOR_RED); printf("\nAccount %lld not found or already inactive.\n", acc_num_to_delete); setcolor(COLOR_DEFAULT); }
}

void resetUserPasswordAdmin() {
    long long acc_num_to_reset;
    setcolor(COLOR_MAGENTA); gotoxy(0, 0); printCentered("Reset User Password", 0, '=', COLOR_MAGENTA); 
    setcolor(COLOR_DEFAULT); printf("\n");

    printf("Enter Account Number whose password you want to reset: ");
    if (scanf("%lld", &acc_num_to_reset) != 1) { setcolor(COLOR_RED); printf("Invalid Account Number format.\n"); setcolor(COLOR_DEFAULT); clearInputBuffer(); return; }
    clearInputBuffer();

    int index_to_reset = findAccountIndex(acc_num_to_reset);

    if (index_to_reset != -1) {
        char new_password_raw[MAX_STRING_LEN];
        setcolor(COLOR_YELLOW); printf("Account found: %s %s (Account %lld).\n", accounts[index_to_reset].first_name, accounts[index_to_reset].last_name, acc_num_to_reset); setcolor(COLOR_DEFAULT);
        printf("Enter new password (max %d chars): ", MAX_STRING_LEN - 1); getMaskedInput(new_password_raw, MAX_STRING_LEN);
        hashAndStore(accounts[index_to_reset].password_hashed, new_password_raw); saveAccounts();
        setcolor(COLOR_GREEN); printf("\nPassword for account %lld reset successfully.\n", acc_num_to_reset); setcolor(COLOR_DEFAULT);
    } else { setcolor(COLOR_RED); printf("\nAccount %lld not found or is inactive.\n", acc_num_to_reset); setcolor(COLOR_DEFAULT); }
}


void exitSystem() {
    clearScreen();
    setcolor(COLOR_YELLOW); gotoxy(0, 5); printCentered("Thank You for Using MONEYMATRIX!", 0, '*', COLOR_YELLOW); 
    setcolor(COLOR_GREEN); gotoxy(0, 8); printCentered("Goodbye!", 0, ' ', COLOR_GREEN); 
    setcolor(COLOR_DEFAULT);
    gotoxy(0, 10); printf("\n\n");
    freeBST(AccountBST_root);
    exit(0);
}

#ifdef _WIN32
void gotoxy(int x, int y) { COORD c; c.X = x; c.Y = y; SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c); }
void setcolor(int color) { HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); SetConsoleTextAttribute(hConsole, color); }
#else
void gotoxy(int x, int y) { /* Do nothing */ }
void setcolor(int color) 
#endif
