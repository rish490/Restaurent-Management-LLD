# Restaurant Management System — Low Level Design

## Problem Statement

Design an in-memory restaurant management system that handles the complete lifecycle of a customer visit: seating, ordering (multiple rounds), kitchen processing, serving, billing, and payment.

## Core Flow

```
Customer arrives → Seat at table (TableSession created)
    → Place order (Round 1) → Kitchen processes → Items served
    → Place order (Round 2) → Kitchen processes → Items served
    → Generate bill (aggregates all orders in session)
    → Process payment → Session closed, table freed
```

## Entities

| Entity | Key Fields | Purpose |
|--------|-----------|---------|
| **MenuItem** | id, name, price, category, available | A dish on the menu |
| **Menu** | map of MenuItems | Holds all available items |
| **OrderItem** | id, menuItemId, quantity, status | One line in an order — references MenuItem, doesn't duplicate it |
| **Order** | id, tableSessionId, list of OrderItems, status | A single round of ordering |
| **Table** | number, capacity, status | Physical table in the restaurant |
| **TableSession** | id, tableNumber, customerId, orderIds, status | Groups everything between seating and leaving — the central lifecycle entity |
| **Customer** | id, name, contactNo | Person visiting |
| **Staff** | id, name, role (enum) | Restaurant employee — role differentiates behavior, no inheritance needed |
| **Bill** | id, tableSessionId, lineItems, subtotal, tax, total | Aggregated bill across all orders in a session |

## Key Relationships

```
TableSession 1 ——— * Order
Order        1 ——— * OrderItem
OrderItem    * ——— 1 MenuItem (reference by ID, not inheritance)
Menu         1 ——— * MenuItem
Bill         * ——— 1 TableSession
```

## Why TableSession Exists

Without it, there's no way to group orders from a single visit. `customerId` alone fails (same customer, different visits). `tableNumber` alone fails (table reused after customer leaves). TableSession is the lifecycle entity created on seating, closed on payment.

## Status State Machines

**OrderItemStatus:**
```
TO_BE_STARTED → PROCESSING → READY_TO_SERVE → SERVED
```

**OrderStatus** (derived from item statuses via `recomputeStatus()`):
```
TO_BE_PICKED → PROCESSING → READY_TO_SERVE → PARTIALLY_SERVED → SERVED → PAYMENT_PENDING → COMPLETED
```
- `PARTIALLY_SERVED`: some items served, some still cooking
- Order status is a precomputed aggregate so the UI can show a summary without scanning all items

**TableSessionStatus:**
```
ACTIVE → BILL_GENERATED → CLOSED
```

## Design Patterns Used

| Pattern | Where | Why |
|---------|-------|-----|
| **Strategy** | `PaymentStrategy` → `CashPayment`, `CardPayment`, `UpiPayment` | Swap payment methods without changing billing logic |
| **Facade** | `Restaurant` class | Single entry point that orchestrates 5 internal services — caller never touches services directly |
| **Composition over Inheritance** | `OrderItem` has `menuItemId` (not extends Item). `Staff` has `role` enum (not Waiter/Manager subclasses) | Inheritance only justified when subclass overrides behavior |

## Services (Single Responsibility)

| Service | Responsibility |
|---------|---------------|
| **TableService** | Find available tables, occupy/free tables |
| **OrderService** | Create orders, track items, update statuses |
| **KitchenService** | Manage processing queue, enforce max concurrent orders |
| **BillingService** | Aggregate all orders in a session into a bill with tax |
| **PaymentService** | Process payment using pluggable strategy |

## Key APIs

```
Restaurant.seatCustomer(customerId, name, partySize) → TableSession
Restaurant.placeOrder(sessionId, [{menuItemId, qty}]) → Order
Restaurant.markItemReady(orderId, itemId)
Restaurant.serveItem(orderId, itemId)
Restaurant.generateBill(sessionId) → Bill
Restaurant.processPayment(sessionId, bill, paymentStrategy) → bool
```

## Non-Functional Requirements

### Concurrency
- `findAvailableTable()` + `occupyTable()` must be atomic (mutex) — otherwise two waiters seat at the same table
- Use `lock_guard<mutex>` wrapping read + update together
- Rule: **check-then-act must be atomic**

### Idempotency
- Single server: mutex on check-and-claim is sufficient
- Multiple servers: database atomic UPDATE (`WHERE status = 'ACTIVE'`) + gateway idempotency key
- Mutex handles in-process concurrency; idempotency keys handle cross-process scenarios (multiple servers, retries, crash recovery)

### Persistence
- In-memory for LLD scope; production would add Repository/DAO layer over a database
- Load active sessions on startup from DB

### Scalability
- `getOrdersForSession()` scans all orders — add `map<sessionId, vector<orderId>>` index for O(1) lookup
- Single restaurant: in-memory maps fine. Chain of restaurants: move to database with indexing



