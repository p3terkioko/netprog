# Banking System Presentation Guide

## Iterative Connectionless (UDP) Implementation

### Overview

This presentation guide is designed for three presenters to explain the workings of our UDP-based banking system. Each presenter will cover a specific aspect of the system, with a total presentation time of approximately 15-20 minutes.

## Presenter 1: System Overview & Architecture (5-7 minutes)

1. **Project Introduction**

   - Open terminal and run `make`
   - Show project structure:
     ```
     iterative_connectionless/
     ├── server.c
     ├── client.c
     ├── Makefile
     └── bank_utils.c (in parent directory)
     ```
   - Pointer:
     - Highlight the separation of concerns
     - Show how bank_utils.c is shared

2. **UDP Architecture**

   - Start server: `./server`
   - Start client: `./client 127.0.0.1 8080`
   - Pointer:
     - Show socket setup in both server.c and client.c
     - Explain why UDP was chosen (simplicity, speed)
     - Highlight the connectionless nature

3. **Message Protocol**

   - Show message format in code:
     - Request: `OPERATION ACCOUNT_NO PIN AMOUNT`
     - Response: `STATUS MESSAGE`
   - Pointer:
     - Highlight message parsing in server.c
     - Show response creation

4. **Hand-Off**
   - "Now that we've seen the architecture, let's look at the core banking operations."

## Presenter 2: Account Management & Deposits (5-7 minutes)

1. **Account Registration**

   - Press 1 in client menu
   - Enter sample data:
     - Name: "John Doe"
     - National ID: "12345"
     - Type: "savings"
     - Initial deposit: 1000
   - Pointer:
     - Show `open_account()` in bank_utils.c
     - Highlight minimum deposit check
     - Show account number generation

2. **Deposit Operation**

   - Press 2 in client menu
   - Use the new account number and PIN
   - Deposit 500
   - Pointer:
     - Show `deposit_extended()` in bank_utils.c
     - Highlight PIN validation
     - Show balance update and transaction logging

3. **Error Handling**

   - Demonstrate invalid deposit (amount < 500)
   - Show server response
   - Pointer:
     - Highlight error checking in server.c
     - Show error response format

4. **Hand-Off**
   - "Now that we've seen account creation and deposits, let's look at withdrawals and account management."

## Presenter 3: Withdrawals & Account Management (5-7 minutes)

1. **Withdrawal Operation**

   - Press 3 in client menu
   - Use same account and PIN
   - Withdraw 500
   - Pointer:
     - Show `withdraw_extended()` in bank_utils.c
     - Highlight balance checks
     - Show transaction logging

2. **Balance Check**

   - Press 4 in client menu
   - Enter account and PIN
   - Pointer:
     - Show `check_balance_extended()`
     - Highlight balance retrieval

3. **Account Statement**

   - Press 5 in client menu
   - Enter account and PIN
   - Pointer:
     - Show `get_statement_extended()`
     - Highlight transaction log reading

4. **Close Account**
   - Press 6 in client menu
   - Enter account and PIN
   - Pointer:
     - Show `close_account_extended()`
     - Highlight account removal and cleanup

## Demo Flow

### Setup (1 minute)

1. Open two terminals
2. In first terminal: `./server`
3. In second terminal: `./client 127.0.0.1 8080`

### Live Demo (3-5 minutes)

1. Create new account (Presenter 2)
2. Make deposit (Presenter 2)
3. Check balance (Presenter 3)
4. Make withdrawal (Presenter 3)
5. Get statement (Presenter 3)
6. Close account (Presenter 3)

### Q&A (5 minutes)

Common questions to prepare for:

1. Why UDP instead of TCP?
2. How is data persistence handled?
3. What security measures are in place?
4. How are concurrent requests handled?
5. What are the limitations of this implementation?

## Technical Notes

- Server runs on port 8080
- Client connects to localhost by default
- All monetary values are stored as doubles
- Transaction logs are maintained in text files
- Minimum deposit: 1000
- Minimum withdrawal: 500
- Minimum balance: 1000

## Presentation Flow

### Introduction (1 minute)

- Brief introduction of the team
- Project overview
- Presentation structure

### Main Presentation (15-17 minutes)

1. Presenter 1: System Overview & Architecture
2. Presenter 2: Account Management & Deposits
3. Presenter 3: Withdrawals & Account Management

### Live Demo (3-5 minutes)

- Start the server
- Run the client
- Demonstrate key operations:
  1. Account registration
  2. Deposit
  3. Withdrawal
  4. Balance check

### Q&A (5 minutes)

- Open floor for questions
- Each presenter handles questions in their area of expertise

## Technical Requirements for Demo

- Two terminal windows
- Server running on one terminal
- Client running on another terminal
- Sample account data ready

## Tips for Presenters

1. **Presenter 1**

   - Focus on high-level architecture
   - Explain design decisions
   - Use diagrams if possible

2. **Presenter 2**

   - Show key code snippets
   - Explain server-side validation
   - Demonstrate error handling

3. **Presenter 3**
   - Focus on user experience
   - Show practical usage
   - Demonstrate error scenarios

## Common Questions to Prepare For

1. Why UDP instead of TCP?
2. How is data persistence handled?
3. What security measures are in place?
4. How are concurrent requests handled?
5. What are the limitations of this implementation?

## Conclusion

- Summarize key points
- Highlight main features
- Discuss potential improvements
- Thank the audience
