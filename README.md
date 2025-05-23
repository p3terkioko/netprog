# netprog
# Banking Application - Monolithic Version Explained

This is a comprehensive analysis of a banking application implemented as a monolithic C program, designed with modularity in mind for future client-server architecture.

## Overview

The application implements a traditional banking system with the following core features:
- Account management (creation and closure)
- Money operations (deposits and withdrawals)
- Balance checking
- Transaction history (statement generation)

All functionality is contained in a single program with clearly separated components that could be split into client and server modules in future versions.

## Architecture Design

The program is structured into several logical components:

1. **User Interface Layer**
   - Menu-driven console interface
   - Input handling and validation
   - Display formatting

2. **Business Logic Layer**
   - Account management
   - Transaction processing
   - Authentication
   - Business rule enforcement

3. **Data Persistence Layer**
   - File-based storage
   - Data locking for concurrent access
   - Transaction logging

4. **Communication Layer**
   - Message creation/parsing (for future client-server communication)
   - Response generation

## Key Features

### Account Management

#### Account Creation
- User provides name, national ID, and account type (savings/checking)
- Requires minimum initial deposit of 1000 units
- System generates unique account number and PIN
- Stores account details in a data file (data.txt)

```c
bool open_account(const char* name, const char* national_id, const char* account_type, 
                  char* out_account_no, char* out_pin, double initial_deposit)
```

#### Account Closure
- Requires account number and PIN verification
- Removes account from the database
- Deletes associated transaction history file

```c
bool close_account(const char* account_no, const char* pin)
```

### Financial Operations

#### Deposits
- Minimum deposit amount is 500 units
- Requires account number and PIN verification
- Updates account balance
- Logs transaction with timestamp

```c
bool deposit_extended(const char* account_no, const char* pin, double amount)
```

#### Withdrawals
- Minimum withdrawal of 500 units
- Requires account number and PIN verification
- Enforces minimum balance of 1000 units after withdrawal
- Updates account balance and logs transaction

```c
bool withdraw_extended(const char* account_no, const char* pin, double amount)
```

### Information Retrieval

#### Balance Checking
- View current account balance
- Doesn't require PIN (basic security implementation)

```c
bool balance(const char* account_no, double* out_balance)
```

#### Account Statement
- Shows last five transactions
- Requires account number and PIN verification
- Displays transaction type, amount, resulting balance, and timestamp

```c
bool statement(const char* account_no, const char* pin, char transactions[5][MAX_LINE_LEN])
```

## Technical Implementation Details

### Data Storage

The program uses two types of files:
- **Main database file** (`data.txt`) - Stores all account information in format:
  ```
  account_number PIN name national_id account_type balance
  ```
- **Transaction log files** (`account_number.txt`) - One per account, stores transaction history:
  ```
  transaction_type amount new_balance timestamp
  ```

### Concurrency Control

File locking mechanisms are implemented for safe concurrent access:
- Uses `flock()` on Unix/Linux systems
- Uses `LockFileEx()` on Windows systems
- Read locks for queries
- Write locks for updates

```c
static void lock_file(FILE* file, bool exclusive) {
    // Platform-specific implementation
}
```

### Security Features

Basic security measures include:
- PIN authentication for sensitive operations
- Random PIN generation (4 digits)
- Transaction logging with timestamps
- Separation of transaction logs per account

### Message Formatting

The program implements message formatting for future client-server architecture:
- **Operations**: `REGISTER`, `DEPOSIT`, `WITHDRAW`, `CHECK_BALANCE`
- **Response codes**: `OK`, `ERROR`, `ACCOUNT_EXISTS`, `ACCOUNT_NOT_FOUND`, etc.

```c
char* create_message(const char* operation, const char* account_no, double amount)
char* create_response(const char* status, double balance)
```

## Business Rules

The application enforces several banking business rules:
- Minimum initial deposit: 1000 units
- Minimum transaction amount: 500 units (deposits and withdrawals)
- Minimum balance after withdrawal: 1000 units
- Valid account types: "savings" or "checking"
- Unique account numbers (auto-incremented)
- Random 4-digit PIN generation

## User Interface

The interface is console-based with a simple menu system:
1. Register New Account
2. Deposit
3. Withdraw
4. Check Balance
5. Statement
6. Close Account
0. Exit

Each option leads to a sub-menu with appropriate prompts and validation.

## Cross-Platform Compatibility

The program includes platform-specific code to ensure compatibility with both Windows and Unix-based systems:
- Different file locking mechanisms
- Different screen clearing commands
- Conditional includes for platform-specific headers


## Error Handling

The application implements various error checks:
- File I/O errors
- Invalid input validation
- Business rule validation
- Memory allocation failures
- Account existence verification

## Conclusion

This banking application demonstrates a well-structured monolithic design with clean separation of concerns. While currently operating as a single program, its modular design and message-passing architecture make it readily convertible to a client-server application with minimal refactoring.

The program balances functionality with security considerations and implements essential banking operations while maintaining data integrity through proper file locking and transaction logging.