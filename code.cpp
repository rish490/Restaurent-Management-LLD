#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
using namespace std;

// ============================================================
// ENUMS
// ============================================================

enum TableStatus { TABLE_FREE, TABLE_OCCUPIED };
enum OrderItemStatus { ITEM_TO_BE_STARTED, ITEM_PROCESSING, ITEM_READY, ITEM_SERVED };
enum OrderStatus { ORD_TO_BE_PICKED, ORD_PROCESSING, ORD_READY, ORD_PARTIAL_SERVED, ORD_SERVED, ORD_PAYMENT_PENDING, ORD_COMPLETED };
enum SessionStatus { SESSION_ACTIVE, SESSION_BILL_GENERATED, SESSION_CLOSED };
enum StaffRole { WAITER, CHEF, MANAGER };

// ============================================================
// MODELS
// ============================================================

class MenuItem {
public:
    string id;
    string name;
    double price;
    string category;
    bool available;

    MenuItem(string id, string name, double price, string category) {
        this->id = id;
        this->name = name;
        this->price = price;
        this->category = category;
        this->available = true;
    }
};

class Menu {
public:
    unordered_map<string, MenuItem*> items;

    void addItem(MenuItem* item) {
        items[item->id] = item;
    }

    MenuItem* getItem(string itemId) {
        if (items.find(itemId) == items.end()) return NULL;
        return items[itemId];
    }
};

class OrderItem {
public:
    string id;
    string menuItemId;
    int quantity;
    OrderItemStatus status;

    OrderItem(string id, string menuItemId, int quantity) {
        this->id = id;
        this->menuItemId = menuItemId;
        this->quantity = quantity;
        this->status = ITEM_TO_BE_STARTED;
    }
};

class Order {
public:
    string id;
    string tableSessionId;
    vector<OrderItem*> items;
    OrderStatus status;

    Order(string id, string tableSessionId) {
        this->id = id;
        this->tableSessionId = tableSessionId;
        this->status = ORD_TO_BE_PICKED;
    }

    void addItem(OrderItem* item) {
        items.push_back(item);
    }

    void recomputeStatus() {
        if (items.empty()) return;

        bool allServed = true, anyServed = false;
        bool allReady = true, anyProcessing = false;

        for (int i = 0; i < items.size(); i++) {
            OrderItemStatus s = items[i]->status;
            if (s != ITEM_SERVED) allServed = false;
            if (s == ITEM_SERVED) anyServed = true;
            if (s != ITEM_READY && s != ITEM_SERVED) allReady = false;
            if (s == ITEM_PROCESSING) anyProcessing = true;
        }

        if (allServed) status = ORD_SERVED;
        else if (anyServed) status = ORD_PARTIAL_SERVED;
        else if (allReady) status = ORD_READY;
        else if (anyProcessing) status = ORD_PROCESSING;
    }
};

class Table {
public:
    int number;
    int capacity;
    TableStatus status;

    Table(int number, int capacity) {
        this->number = number;
        this->capacity = capacity;
        this->status = TABLE_FREE;
    }
};

class TableSession {
public:
    string id;
    int tableNumber;
    string customerId;
    vector<string> orderIds;
    SessionStatus status;

    TableSession(string id, int tableNumber, string customerId) {
        this->id = id;
        this->tableNumber = tableNumber;
        this->customerId = customerId;
        this->status = SESSION_ACTIVE;
    }
};

class Customer {
public:
    string id;
    string name;
    string contactNo;

    Customer(string id, string name, string contactNo) {
        this->id = id;
        this->name = name;
        this->contactNo = contactNo;
    }
};

class Staff {
public:
    string id;
    string name;
    StaffRole role;

    Staff(string id, string name, StaffRole role) {
        this->id = id;
        this->name = name;
        this->role = role;
    }
};

// ============================================================
// BILL
// ============================================================

struct BillLineItem {
    string menuItemName;
    int quantity;
    double unitPrice;
    double lineTotal;
};

class Bill {
public:
    string id;
    string tableSessionId;
    vector<BillLineItem> lineItems;
    double subtotal;
    double tax;
    double total;

    Bill(string id, string tableSessionId) {
        this->id = id;
        this->tableSessionId = tableSessionId;
        this->subtotal = 0;
        this->tax = 0;
        this->total = 0;
    }

    void addLineItem(BillLineItem item) {
        lineItems.push_back(item);
        subtotal += item.lineTotal;
        tax = subtotal * 0.18;
        total = subtotal + tax;
    }

    void print() {
        cout << "\n========== BILL ==========" << endl;
        for (int i = 0; i < lineItems.size(); i++) {
            cout << lineItems[i].menuItemName << " x" << lineItems[i].quantity
                 << " @ " << lineItems[i].unitPrice << " = " << lineItems[i].lineTotal << endl;
        }
        cout << "--------------------------" << endl;
        cout << "Subtotal: " << subtotal << endl;
        cout << "Tax:      " << tax << endl;
        cout << "TOTAL:    " << total << endl;
        cout << "==========================" << endl;
    }
};

// ============================================================
// PAYMENT - Strategy Pattern
// ============================================================

class PaymentStrategy {
public:
    virtual bool pay(double amount) = 0;
    virtual string getName() = 0;
};

class CashPayment : public PaymentStrategy {
public:
    bool pay(double amount) {
        cout << "[Cash] Received " << amount << endl;
        return true;
    }
    string getName() { return "CASH"; }
};

class CardPayment : public PaymentStrategy {
public:
    string cardNumber;
    CardPayment(string cardNumber) { this->cardNumber = cardNumber; }

    bool pay(double amount) {
        cout << "[Card] Charged " << amount << endl;
        return true;
    }
    string getName() { return "CARD"; }
};

class UpiPayment : public PaymentStrategy {
public:
    string upiId;
    UpiPayment(string upiId) { this->upiId = upiId; }

    bool pay(double amount) {
        cout << "[UPI] Payment of " << amount << " to " << upiId << endl;
        return true;
    }
    string getName() { return "UPI"; }
};

// ============================================================
// SERVICES
// ============================================================

class TableService {
public:
    unordered_map<int, Table*> tables;

    void addTable(int number, int capacity) {
        tables[number] = new Table(number, capacity);
    }

    Table* findAvailableTable(int partySize) {
        for (auto& p : tables) {
            if (p.second->status == TABLE_FREE && p.second->capacity >= partySize)
                return p.second;
        }
        return NULL;
    }

    void occupyTable(int number) { tables[number]->status = TABLE_OCCUPIED; }
    void freeTable(int number) { tables[number]->status = TABLE_FREE; }
};

class OrderService {
public:
    unordered_map<string, Order*> orders;
    int orderCounter = 0;
    int itemCounter = 0;

    Order* createOrder(string sessionId, vector<pair<string, int>> items, Menu* menu) {
        string orderId = "ORD-" + to_string(++orderCounter);
        Order* order = new Order(orderId, sessionId);

        for (int i = 0; i < items.size(); i++) {
            MenuItem* mi = menu->getItem(items[i].first);
            if (!mi || !mi->available) {
                cout << "Item not available: " << items[i].first << endl;
                continue;
            }
            string itemId = "OI-" + to_string(++itemCounter);
            order->addItem(new OrderItem(itemId, items[i].first, items[i].second));
        }

        orders[orderId] = order;
        return order;
    }

    Order* getOrder(string orderId) {
        if (orders.find(orderId) == orders.end()) return NULL;
        return orders[orderId];
    }

    void updateItemStatus(string orderId, string itemId, OrderItemStatus newStatus) {
        Order* order = getOrder(orderId);
        for (int i = 0; i < order->items.size(); i++) {
            if (order->items[i]->id == itemId) {
                order->items[i]->status = newStatus;
                order->recomputeStatus();
                return;
            }
        }
    }

    vector<Order*> getOrdersForSession(string sessionId) {
        vector<Order*> result;
        for (auto& p : orders) {
            if (p.second->tableSessionId == sessionId)
                result.push_back(p.second);
        }
        return result;
    }
};

class KitchenService {
    queue<string> pendingOrders;
    int activeCount = 0;
    int maxConcurrent;
    OrderService* orderService;

public:
    KitchenService(OrderService* orderService, int maxConcurrent) {
        this->orderService = orderService;
        this->maxConcurrent = maxConcurrent;
        this->activeCount = 0;
    }

    void enqueueOrder(string orderId) {
        pendingOrders.push(orderId);
        cout << "[Kitchen] Order " << orderId << " queued" << endl;
        tryProcessNext();
    }

    void tryProcessNext() {
        while (activeCount < maxConcurrent && !pendingOrders.empty()) {
            string orderId = pendingOrders.front();
            pendingOrders.pop();

            Order* order = orderService->getOrder(orderId);
            activeCount++;
            for (int i = 0; i < order->items.size(); i++) {
                orderService->updateItemStatus(orderId, order->items[i]->id, ITEM_PROCESSING);
            }
            cout << "[Kitchen] Processing " << orderId << " (active: " << activeCount << ")" << endl;
        }
    }

    void markItemReady(string orderId, string itemId) {
        orderService->updateItemStatus(orderId, itemId, ITEM_READY);

        Order* order = orderService->getOrder(orderId);
        if (order->status == ORD_READY) {
            activeCount--;
            cout << "[Kitchen] " << orderId << " fully ready" << endl;
            tryProcessNext();
        }
    }

    void markItemServed(string orderId, string itemId) {
        orderService->updateItemStatus(orderId, itemId, ITEM_SERVED);
    }
};

class BillingService {
    int billCounter = 0;

public:
    Bill* generateBill(string sessionId, OrderService* orderService, Menu* menu) {
        vector<Order*> orders = orderService->getOrdersForSession(sessionId);
        string billId = "BILL-" + to_string(++billCounter);
        Bill* bill = new Bill(billId, sessionId);

        unordered_map<string, pair<int, double>> aggregated;
        unordered_map<string, string> names;

        for (int i = 0; i < orders.size(); i++) {
            for (int j = 0; j < orders[i]->items.size(); j++) {
                OrderItem* oi = orders[i]->items[j];
                MenuItem* mi = menu->getItem(oi->menuItemId);
                if (!mi) continue;
                aggregated[oi->menuItemId].first += oi->quantity;
                aggregated[oi->menuItemId].second = mi->price;
                names[oi->menuItemId] = mi->name;
            }
        }

        for (auto& p : aggregated) {
            BillLineItem li;
            li.menuItemName = names[p.first];
            li.quantity = p.second.first;
            li.unitPrice = p.second.second;
            li.lineTotal = li.quantity * li.unitPrice;
            bill->addLineItem(li);
        }

        for (int i = 0; i < orders.size(); i++) {
            orders[i]->status = ORD_PAYMENT_PENDING;
        }

        return bill;
    }
};

class PaymentService {
public:
    bool processPayment(Bill* bill, PaymentStrategy* strategy,
                        OrderService* orderService, string sessionId) {
        bool success = strategy->pay(bill->total);
        if (success) {
            vector<Order*> orders = orderService->getOrdersForSession(sessionId);
            for (int i = 0; i < orders.size(); i++) {
                orders[i]->status = ORD_COMPLETED;
            }
            cout << "[Payment] " << strategy->getName() << " payment of " << bill->total << " done" << endl;
        }
        return success;
    }
};

// ============================================================
// RESTAURANT FACADE
// ============================================================

class Restaurant {
    string name;
    Menu* menu;
    TableService* tableService;
    OrderService* orderService;
    KitchenService* kitchenService;
    BillingService* billingService;
    PaymentService* paymentService;

    unordered_map<string, TableSession*> sessions;
    unordered_map<string, Customer*> customers;
    int sessionCounter = 0;

public:
    Restaurant(string name, int maxKitchenOrders) {
        this->name = name;
        menu = new Menu();
        tableService = new TableService();
        orderService = new OrderService();
        kitchenService = new KitchenService(orderService, maxKitchenOrders);
        billingService = new BillingService();
        paymentService = new PaymentService();
    }

    Menu* getMenu() { return menu; }

    void setupTable(int number, int capacity) {
        tableService->addTable(number, capacity);
    }

    TableSession* seatCustomer(string customerId, string customerName, int partySize) {
        Table* table = tableService->findAvailableTable(partySize);
        if (!table) {
            cout << "No table available!" << endl;
            return NULL;
        }

        if (customers.find(customerId) == customers.end()) {
            customers[customerId] = new Customer(customerId, customerName, customerId);
        }

        tableService->occupyTable(table->number);

        string sessionId = "SESSION-" + to_string(++sessionCounter);
        TableSession* session = new TableSession(sessionId, table->number, customerId);
        sessions[sessionId] = session;

        cout << "[Restaurant] Seated " << customerName << " at Table " << table->number
             << " | Session: " << sessionId << endl;
        return session;
    }

    Order* placeOrder(string sessionId, vector<pair<string, int>> items) {
        TableSession* session = sessions[sessionId];
        if (!session || session->status != SESSION_ACTIVE) {
            cout << "Invalid session!" << endl;
            return NULL;
        }

        Order* order = orderService->createOrder(sessionId, items, menu);
        session->orderIds.push_back(order->id);
        kitchenService->enqueueOrder(order->id);

        cout << "[Restaurant] Order " << order->id << " placed" << endl;
        return order;
    }

    void markItemReady(string orderId, string itemId) {
        kitchenService->markItemReady(orderId, itemId);
    }

    void serveItem(string orderId, string itemId) {
        kitchenService->markItemServed(orderId, itemId);
        cout << "[Restaurant] Item " << itemId << " served" << endl;
    }

    Bill* generateBill(string sessionId) {
        TableSession* session = sessions[sessionId];
        Bill* bill = billingService->generateBill(sessionId, orderService, menu);
        session->status = SESSION_BILL_GENERATED;
        bill->print();
        return bill;
    }

    bool processPayment(string sessionId, Bill* bill, PaymentStrategy* strategy) {
        TableSession* session = sessions[sessionId];
        bool success = paymentService->processPayment(bill, strategy, orderService, sessionId);
        if (success) {
            session->status = SESSION_CLOSED;
            tableService->freeTable(session->tableNumber);
            cout << "[Restaurant] Session closed. Table " << session->tableNumber << " free." << endl;
        }
        return success;
    }
};

// ============================================================
// DEMO
// ============================================================

int main() {
    Restaurant restaurant("The Code Kitchen", 3);

    restaurant.setupTable(1, 2);
    restaurant.setupTable(2, 4);
    restaurant.setupTable(3, 6);

    Menu* menu = restaurant.getMenu();
    menu->addItem(new MenuItem("M1", "Butter Chicken", 350, "Main"));
    menu->addItem(new MenuItem("M2", "Naan", 60, "Bread"));
    menu->addItem(new MenuItem("M3", "Dal Makhani", 250, "Main"));
    menu->addItem(new MenuItem("M4", "Gulab Jamun", 120, "Dessert"));
    menu->addItem(new MenuItem("M5", "Lassi", 80, "Beverage"));

    cout << "=== Customer arrives ===" << endl;
    TableSession* session = restaurant.seatCustomer("C1", "Rishabh", 3);

    cout << "\n=== Round 1: Main course ===" << endl;
    Order* order1 = restaurant.placeOrder(session->id, {{"M1", 2}, {"M2", 4}, {"M3", 1}});

    cout << "\n=== Kitchen prepares ===" << endl;
    for (int i = 0; i < order1->items.size(); i++)
        restaurant.markItemReady(order1->id, order1->items[i]->id);

    cout << "\n=== Waiter serves ===" << endl;
    for (int i = 0; i < order1->items.size(); i++)
        restaurant.serveItem(order1->id, order1->items[i]->id);

    cout << "\n=== Round 2: Dessert ===" << endl;
    Order* order2 = restaurant.placeOrder(session->id, {{"M4", 3}, {"M5", 3}});

    for (int i = 0; i < order2->items.size(); i++)
        restaurant.markItemReady(order2->id, order2->items[i]->id);
    for (int i = 0; i < order2->items.size(); i++)
        restaurant.serveItem(order2->id, order2->items[i]->id);

    cout << "\n=== Bill ===" << endl;
    Bill* bill = restaurant.generateBill(session->id);

    cout << "\n=== Payment ===" << endl;
    UpiPayment upi("rishabh@upi");
    restaurant.processPayment(session->id, bill, &upi);

    return 0;
}
