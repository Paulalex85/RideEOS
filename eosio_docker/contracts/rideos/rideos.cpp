#include "rideos.hpp"
using namespace eosio;

// int DELAY_END_ASSIGN = 86400;
// int DELAY_POWER_NEEDED = 86400;
// Only for testing
int DELAY_END_ASSIGN = 5;
int DELAY_POWER_NEEDED = 5;

bool is_equal(const capi_checksum256 &a, const capi_checksum256 &b)
{
    return memcmp((void *)&a, (const void *)&b, sizeof(capi_checksum256)) == 0;
}

bool is_zero(const capi_checksum256 &a)
{
    const uint64_t *p64 = reinterpret_cast<const uint64_t *>(&a);
    return p64[0] == 0 && p64[1] == 0 && p64[2] == 0 && p64[3] == 0;
}

bool is_actor(const name sender, const name buyer, const name seller, const name deliver)
{
    if (sender.value == buyer.value || sender.value == seller.value || sender.value == deliver.value)
    {
        return true;
    }
    return false;
}

void rideos::find_stackpower_and_increase(const name account, const asset &quantity)
{
    auto indexStack = _stackpower.get_index<name("byaccount")>();
    auto iteratorStack = indexStack.find(account.value);

    bool found = false;

    while (iteratorStack != indexStack.end())
    {
        if (iteratorStack->account == account && iteratorStack->endAssignment == time_point_sec(0))
        {
            indexStack.modify(iteratorStack, _self, [&](auto &stackpower) {
                stackpower.balance += quantity;
            });

            found = true;
            break;
        }
        else
        {
            iteratorStack++;
        }
    }

    if (!found)
    {
        _stackpower.emplace(_self, [&](auto &stackpower) {
            stackpower.stackKey = _stackpower.available_primary_key();
            stackpower.account = account;
            stackpower.balance = quantity;
        });
    }
}

void rideos::add_balance(const name account, const asset &quantity)
{
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must withdraw positive quantity");
    eosio_assert(quantity.symbol == eosio::symbol("SYS", 4), "only core token allowed");

    auto iterator = _users.find(account.value);
    eosio_assert(iterator != _users.end(), "Address for account not found");

    _users.modify(iterator, _self, [&](auto &user) {
        user.balance += quantity;
    });
}

void rideos::adduser(const name account, const string &username)
{
    require_auth(account);

    auto iterator = _users.find(account.value);
    eosio_assert(iterator == _users.end(), "Address for account already exists");

    _users.emplace(_self, [&](auto &user) {
        user.account = account;
        user.username = username;
        user.balance = eosio::asset(0, symbol(symbol_code("SYS"), 4));
    });
}

void rideos::updateuser(const name account, const string &username)
{
    require_auth(account);

    auto iterator = _users.find(account.value);
    eosio_assert(iterator != _users.end(), "Address for account not found");

    _users.modify(iterator, _self, [&](auto &user) {
        user.username = username;
    });
}

void rideos::deleteuser(const name account)
{
    require_auth(account);

    auto iterator = _users.find(account.value);
    eosio_assert(iterator != _users.end(), "Address for account not found");

    eosio_assert(iterator->balance.amount == 0, "Balance should be empty");

    auto indexStack = _stackpower.get_index<name("byaccount")>();
    auto iteratorStack = indexStack.find(account.value);
    eosio_assert(iteratorStack == indexStack.end(), "Delete Stackpower first");

    auto indexOrdersBuyer = _orders.get_index<name("bybuyerkey")>();
    auto iteratorOrderBuyer = indexOrdersBuyer.find(account.value);
    eosio_assert(iteratorOrderBuyer == indexOrdersBuyer.end(), "Delete Order first");

    auto indexOrdersSeller = _orders.get_index<name("bysellerkey")>();
    auto iteratorOrderSeller = indexOrdersSeller.find(account.value);
    eosio_assert(iteratorOrderSeller == indexOrdersSeller.end(), "Delete Order first");

    auto indexOrdersDeliver = _orders.get_index<name("bydeliverkey")>();
    auto iteratorOrderDeliver = indexOrdersDeliver.find(account.value);
    eosio_assert(iteratorOrderDeliver == indexOrdersDeliver.end(), "Delete Order first");

    _users.erase(iterator);
}

void rideos::deposit(const name account, const asset &quantity)
{
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must deposit positive quantity");

    auto iterator = _users.find(account.value);
    eosio_assert(iterator != _users.end(), "Address for account not found");

    action(
        permission_level{account, name("active")},
        name("eosio.token"),
        name("transfer"),
        std::make_tuple(account, _self, quantity, std::string("")))
        .send();

    _users.modify(iterator, _self, [&](auto &user) {
        user.balance += quantity;
    });
}

void rideos::withdraw(const name account, const asset &quantity)
{
    require_auth(account);

    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must withdraw positive quantity");

    auto iterator = _users.find(account.value);
    eosio_assert(iterator != _users.end(), "Address for account not found");

    _users.modify(iterator, _self, [&](auto &user) {
        eosio_assert(user.balance >= quantity, "insufficient balance");
        user.balance -= quantity;
    });

    action(
        permission_level{_self, name("active")},
        name("eosio.token"), name("transfer"),
        std::make_tuple(_self, account, quantity, std::string("")))
        .send();
}

void rideos::pay(const name account, const asset &quantity)
{
    require_auth(account);

    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must withdraw positive quantity");
    eosio_assert(quantity.symbol == eosio::symbol("SYS", 4), "only core token allowed");

    auto iterator = _users.find(account.value);
    eosio_assert(iterator != _users.end(), "Address for account not found");

    eosio_assert(iterator->balance >= quantity, "insufficient balance");

    _users.modify(iterator, _self, [&](auto &user) {
        user.balance -= quantity;
    });
}

void rideos::stackpow(const name account, const asset &quantity)
{
    require_auth(account);

    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must withdraw positive quantity");
    eosio_assert(quantity.symbol == eosio::symbol("SYS", 4), "only core token allowed");

    auto iteratorUser = _users.find(account.value);
    eosio_assert(iteratorUser != _users.end(), "Address for account not found");

    eosio_assert(iteratorUser->balance >= quantity, "Insufficient balance");

    _users.modify(iteratorUser, _self, [&](auto &user) {
        user.balance -= quantity;
    });

    find_stackpower_and_increase(account, quantity);
}

void rideos::unlockpow(const name account, const asset &quantity, const uint64_t stackKey)
{
    require_auth(account);

    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must withdraw positive quantity");
    eosio_assert(quantity.symbol == eosio::symbol("SYS", 4), "only core token allowed");

    auto iteratorUser = _users.find(account.value);
    eosio_assert(iteratorUser != _users.end(), "Address for account not found");

    auto iteratorStackpower = _stackpower.find(stackKey);
    eosio_assert(iteratorStackpower != _stackpower.end(), "Address for stackpower not found");

    eosio_assert(iteratorStackpower->balance >= quantity, "The amount should be less or equal than the balance");
    eosio_assert(iteratorStackpower->account == account, "The user is not the same");

    eosio_assert(iteratorStackpower->endAssignment == time_point_sec(0), "The stack is already unlocked");

    if (iteratorStackpower->balance == quantity)
    {
        _stackpower.modify(iteratorStackpower, _self, [&](auto &stackpower) {
            stackpower.endAssignment = time_point_sec(now() + DELAY_END_ASSIGN);
        });
    }
    else
    {
        asset keepStacked = iteratorStackpower->balance - quantity;
        _stackpower.modify(iteratorStackpower, _self, [&](auto &stackpower) {
            stackpower.endAssignment = time_point_sec(now() + DELAY_END_ASSIGN);
            stackpower.balance = quantity;
        });
        _stackpower.emplace(_self, [&](auto &stackpower) {
            stackpower.stackKey = _stackpower.available_primary_key();
            stackpower.account = account;
            stackpower.balance = keepStacked;
        });
    }
}

void rideos::unstackpow(const name account, const uint64_t stackKey)
{
    require_auth(account);

    auto iteratorStackpower = _stackpower.find(stackKey);
    eosio_assert(iteratorStackpower != _stackpower.end(), "Address for stackpower not found");

    eosio_assert(iteratorStackpower->account == account, "The user is not the same");
    eosio_assert(iteratorStackpower->endAssignment < time_point_sec(now()), "The stack is not ready to unstack");
    eosio_assert(iteratorStackpower->endAssignment != time_point_sec(0), "The stack is already unlocked");

    add_balance(account, iteratorStackpower->balance);

    _stackpower.erase(iteratorStackpower);
}

void rideos::needdeliver(const name buyer, const name seller, const asset &priceOrder, const asset &priceDeliver,
                         const std::string &details, const uint64_t delay)
{
    eosio_assert(priceOrder.symbol == eosio::symbol("SYS", 4), "only core token allowed");
    eosio_assert(priceOrder.is_valid(), "invalid bet");
    eosio_assert(priceOrder.amount > 0, "must bet positive quantity");

    eosio_assert(priceDeliver.symbol == eosio::symbol("SYS", 4), "only core token allowed");
    eosio_assert(priceDeliver.is_valid(), "invalid bet");
    eosio_assert(priceDeliver.amount > 0, "must bet positive quantity");

    auto iteratorUser = _users.find(buyer.value);
    eosio_assert(iteratorUser != _users.end(), "Buyer not found");

    iteratorUser = _users.find(seller.value);
    eosio_assert(iteratorUser != _users.end(), "Seller not found");

    _orders.emplace(_self, [&](auto &order) {
        order.orderKey = _orders.available_primary_key();
        order.buyer = buyer;
        order.seller = seller;
        order.state = NEED_DELIVER;
        order.date = time_point_sec(now());
        order.dateDelay = time_point_sec(now());
        order.priceOrder = priceOrder;
        order.priceDeliver = priceDeliver;
        order.validateBuyer = false;
        order.validateSeller = false;
        order.validateDeliver = false;
        order.details = details;
        order.delay = delay;
    });
}

void rideos::deliverfound(const name deliver, const uint64_t orderKey)
{
    auto iteratorOrder = _orders.find(orderKey);
    eosio_assert(iteratorOrder != _orders.end(), "Address for order not found");

    require_auth(iteratorOrder->buyer);

    auto iteratorUser = _users.find(deliver.value);
    eosio_assert(iteratorUser != _users.end(), "Deliver not found");

    eosio_assert(iteratorOrder->state == NEED_DELIVER, "Should be at the state Need deliver");

    _orders.modify(iteratorOrder, _self, [&](auto &order) {
        order.state = INITIALIZATION;
        order.deliver = deliver;
    });

    auto indexOffer = _offers.get_index<name("byorderkey")>();
    auto iteratorOffer = indexOffer.find(orderKey);

    if (iteratorOffer != indexOffer.end() && iteratorOffer->stateOffer == 0)
    {
        action(
            permission_level{iteratorOrder->buyer, name("active")},
            name("rideos"), name("endoffer"),
            std::make_tuple(deliver, iteratorOffer->offerKey))
            .send();
    }
}

void rideos::initialize(const name buyer, const name seller, const name deliver, const asset &priceOrder,
                        const asset &priceDeliver, const string &details, const uint64_t delay)
{
    eosio_assert(priceOrder.symbol == eosio::symbol("SYS", 4), "only core token allowed");
    eosio_assert(priceOrder.is_valid(), "invalid bet");
    eosio_assert(priceOrder.amount > 0, "must bet positive quantity");

    eosio_assert(priceDeliver.symbol == eosio::symbol("SYS", 4), "only core token allowed");
    eosio_assert(priceDeliver.is_valid(), "invalid bet");
    eosio_assert(priceDeliver.amount > 0, "must bet positive quantity");

    auto iteratorUser = _users.find(buyer.value);
    eosio_assert(iteratorUser != _users.end(), "Buyer not found");

    iteratorUser = _users.find(seller.value);
    eosio_assert(iteratorUser != _users.end(), "Seller not found");

    iteratorUser = _users.find(deliver.value);
    eosio_assert(iteratorUser != _users.end(), "Deliver not found");

    _orders.emplace(_self, [&](auto &order) {
        order.orderKey = _orders.available_primary_key();
        order.buyer = buyer;
        order.seller = seller;
        order.deliver = deliver;
        order.state = INITIALIZATION;
        order.date = time_point_sec(now());
        order.dateDelay = time_point_sec(now());
        order.priceOrder = priceOrder;
        order.priceDeliver = priceDeliver;
        order.validateBuyer = false;
        order.validateSeller = false;
        order.validateDeliver = false;
        order.details = details;
        order.delay = delay;
    });
}

void rideos::validatebuy(const uint64_t orderKey, const capi_checksum256 &hash)
{
    auto iteratorOrder = _orders.find(orderKey);
    eosio_assert(iteratorOrder != _orders.end(), "Address for order not found");

    require_auth(iteratorOrder->buyer);

    eosio_assert(iteratorOrder->state == INITIALIZATION, "The order is not in the state of initialization");

    auto iteratorUser = _users.find(iteratorOrder->buyer.value);
    eosio_assert(iteratorUser != _users.end(), "Buyer not found");

    eosio_assert(iteratorOrder->validateBuyer == false, "Buyer already validate");

    eosio_assert(iteratorUser->balance >= iteratorOrder->priceOrder + iteratorOrder->priceDeliver, "insufficient balance");

    pay(iteratorOrder->buyer, iteratorOrder->priceOrder + iteratorOrder->priceDeliver);

    _orders.modify(iteratorOrder, _self, [&](auto &order) {
        order.validateBuyer = true;
        order.deliveryverification = hash;

        if (iteratorOrder->validateSeller && iteratorOrder->validateDeliver)
        {
            order.state = ORDER_READY;
            order.dateDelay = eosio::time_point_sec(now() + iteratorOrder->delay);
        }
    });
}

void rideos::validatedeli(const uint64_t orderKey, const uint64_t stackKey)
{
    auto iteratorOrder = _orders.find(orderKey);
    eosio_assert(iteratorOrder != _orders.end(), "Address for order not found");

    require_auth(iteratorOrder->deliver);

    eosio_assert(iteratorOrder->state == INITIALIZATION, "The order is not in the state of initialization");

    eosio_assert(iteratorOrder->validateDeliver == false, "Deliver already validate");

    auto iteratorStack = _stackpower.find(stackKey);
    eosio_assert(iteratorStack != _stackpower.end(), "Address for stackpower not found");

    eosio_assert(iteratorStack->account == iteratorOrder->deliver, "The user is not the same of the stackpower");
    eosio_assert(iteratorStack->endAssignment == time_point_sec(0), "The stack is not available");
    eosio_assert(iteratorStack->balance >= iteratorOrder->priceDeliver, "insufficient balance");

    _stackpower.modify(iteratorStack, _self, [&](auto &stackpower) {
        stackpower.balance -= iteratorOrder->priceDeliver;
    });

    _orders.modify(iteratorOrder, _self, [&](auto &order) {
        order.validateDeliver = true;

        if (iteratorOrder->validateSeller && iteratorOrder->validateBuyer)
        {
            order.state = ORDER_READY;
            order.dateDelay = eosio::time_point_sec(now() + iteratorOrder->delay);
        }
    });
}

void rideos::validatesell(const uint64_t orderKey, const capi_checksum256 &hash, const uint64_t stackKey)
{
    auto iteratorOrder = _orders.find(orderKey);
    eosio_assert(iteratorOrder != _orders.end(), "Address for order not found");

    require_auth(iteratorOrder->seller);

    eosio_assert(iteratorOrder->state == INITIALIZATION, "The order is not in the state of initialization");

    eosio_assert(iteratorOrder->validateSeller == false, "Seller already validate");

    auto iteratorStack = _stackpower.find(stackKey);
    eosio_assert(iteratorStack != _stackpower.end(), "Address for stackpower not found");

    eosio_assert(iteratorStack->account == iteratorOrder->seller, "The user is not the same of the stackpower");
    eosio_assert(iteratorStack->endAssignment == time_point_sec(0), "The stack is not available");
    eosio_assert(iteratorStack->balance >= iteratorOrder->priceOrder, "insufficient balance");

    _stackpower.modify(iteratorStack, _self, [&](auto &stackpower) {
        stackpower.balance -= iteratorOrder->priceOrder;
    });

    _orders.modify(iteratorOrder, _self, [&](auto &order) {
        order.validateSeller = true;
        order.takeverification = hash;

        if (iteratorOrder->validateDeliver && iteratorOrder->validateBuyer)
        {
            order.state = ORDER_READY;
            order.dateDelay = eosio::time_point_sec(now() + iteratorOrder->delay);
        }
    });
}

void rideos::orderready(const uint64_t orderKey)
{
    auto iteratorOrder = _orders.find(orderKey);
    eosio_assert(iteratorOrder != _orders.end(), "Address for order not found");

    require_auth(iteratorOrder->seller);

    eosio_assert(iteratorOrder->state == ORDER_READY, "The order is not in the state of product ready");

    _orders.modify(iteratorOrder, _self, [&](auto &order) {
        order.state = ORDER_TAKEN;
    });
}

void rideos::ordertaken(const uint64_t orderKey, const capi_checksum256 &source)
{
    auto iteratorOrder = _orders.find(orderKey);
    eosio_assert(iteratorOrder != _orders.end(), "Address for order not found");

    require_auth(iteratorOrder->deliver);

    assert_sha256((char *)&source, sizeof(source), (const capi_checksum256 *)&iteratorOrder->takeverification);

    eosio_assert(iteratorOrder->state == ORDER_TAKEN, "The order is not in the state of waiting deliver");

    _orders.modify(iteratorOrder, _self, [&](auto &order) {
        order.state = ORDER_DELIVERED;
    });
}

void rideos::orderdelive(const uint64_t orderKey, const capi_checksum256 &source)
{
    auto iteratorOrder = _orders.find(orderKey);
    eosio_assert(iteratorOrder != _orders.end(), "Address for order not found");

    require_auth(iteratorOrder->deliver);

    assert_sha256((char *)&source, sizeof(source), (const capi_checksum256 *)&iteratorOrder->deliveryverification);

    eosio_assert(iteratorOrder->state == ORDER_DELIVERED, "The order is not in the state delivery");

    _orders.modify(iteratorOrder, _self, [&](auto &order) {
        order.state = ORDER_END;
    });

    add_balance(iteratorOrder->seller, iteratorOrder->priceOrder);
    add_balance(iteratorOrder->deliver, iteratorOrder->priceDeliver);

    find_stackpower_and_increase(iteratorOrder->seller, iteratorOrder->priceOrder);
    find_stackpower_and_increase(iteratorOrder->deliver, iteratorOrder->priceDeliver);
}

void rideos::initcancel(const uint64_t orderKey, const name account)
{
    auto iteratorOrder = _orders.find(orderKey);
    eosio_assert(iteratorOrder != _orders.end(), "Address for order not found");

    require_auth(account);

    eosio_assert(is_actor(account, iteratorOrder->buyer, iteratorOrder->seller, iteratorOrder->deliver), "The sender is not in the contract");

    bool isState = iteratorOrder->state == INITIALIZATION || iteratorOrder->state == NEED_DELIVER;

    eosio_assert(isState, "The order is not in the state of initialization or need deliver");

    if (iteratorOrder->validateBuyer)
    {
        add_balance(iteratorOrder->buyer, iteratorOrder->priceOrder + iteratorOrder->priceDeliver);
    }

    if (iteratorOrder->validateSeller)
    {
        find_stackpower_and_increase(iteratorOrder->seller, iteratorOrder->priceOrder);
    }

    if (iteratorOrder->validateDeliver)
    {
        find_stackpower_and_increase(iteratorOrder->deliver, iteratorOrder->priceDeliver);
    }

    _orders.modify(iteratorOrder, _self, [&](auto &order) {
        order.state = INIT_CANCEL;
    });
}

void rideos::delaycancel(const uint64_t orderKey)
{
    auto iteratorOrder = _orders.find(orderKey);
    eosio_assert(iteratorOrder != _orders.end(), "Address for order not found");

    require_auth(iteratorOrder->buyer);

    eosio_assert(iteratorOrder->state > INITIALIZATION, "The order is in the state of initialization");
    eosio_assert(iteratorOrder->state < ORDER_END, "The order is finish");

    eosio_assert(iteratorOrder->dateDelay < eosio::time_point_sec(now()), "The delay for cancel the order is not passed");

    add_balance(iteratorOrder->buyer, iteratorOrder->priceOrder + iteratorOrder->priceDeliver);

    find_stackpower_and_increase(iteratorOrder->seller, iteratorOrder->priceOrder);
    find_stackpower_and_increase(iteratorOrder->deliver, iteratorOrder->priceDeliver);

    _orders.modify(iteratorOrder, _self, [&](auto &order) {
        order.state = ORDER_CANCEL;
    });
}

void rideos::deleteorder(const uint64_t orderKey)
{
    auto iteratorOrder = _orders.find(orderKey);
    eosio_assert(iteratorOrder != _orders.end(), "Address for order not found");

    auto indexOffer = _offers.get_index<name("byorderkey")>();
    auto iteratorOffer = indexOffer.find(orderKey);

    eosio_assert(iteratorOffer == indexOffer.end(), "Delete offers first");

    if (iteratorOrder->state == 5 || iteratorOrder->state == 98 || iteratorOrder->state == 99)
    {
        _orders.erase(iteratorOrder);
    }
}

void rideos::addoffer(const uint64_t orderKey)
{
    auto iteratorOrder = _orders.find(orderKey);
    eosio_assert(iteratorOrder != _orders.end(), "Order not found");

    require_auth(iteratorOrder->buyer);

    eosio_assert(iteratorOrder->state == 0, "The state of the order don't need a deliver");

    _offers.emplace(_self, [&](auto &offer) {
        offer.offerKey = _offers.available_primary_key();
        offer.orderKey = orderKey;
        offer.stateOffer = OPEN;
    });
}

void rideos::endoffer(const name deliver, const uint64_t offerKey)
{
    auto iteratorOffer = _offers.find(offerKey);
    eosio_assert(iteratorOffer != _offers.end(), "Offer not found");

    eosio_assert(iteratorOffer->stateOffer == OPEN, "The state of the offer is not open");

    auto iteratorOrder = _orders.find(iteratorOffer->orderKey);
    eosio_assert(iteratorOrder != _orders.end(), "Order not found");

    auto iteratorUser = _users.find(deliver.value);
    eosio_assert(iteratorUser != _users.end(), "Deliver not found");

    require_auth(iteratorOrder->buyer);

    _offers.modify(iteratorOffer, _self, [&](auto &offer) {
        offer.stateOffer = FOUNDED;
    });

    if (iteratorOrder->state == 0)
    {
        action(
            permission_level{iteratorOrder->buyer, name("active")},
            name("rideor"), name("deliverfound"),
            std::make_tuple(deliver, iteratorOffer->orderKey))
            .send();
    }
}

void rideos::canceloffer(const uint64_t offerKey)
{
    auto iteratorOffer = _offers.find(offerKey);
    eosio_assert(iteratorOffer != _offers.end(), "Offer not found");

    eosio_assert(iteratorOffer->stateOffer == OPEN, "The state of the offer is not open");

    auto iteratorOrder = _orders.find(iteratorOffer->orderKey);
    eosio_assert(iteratorOrder != _orders.end(), "Order not found");

    require_auth(iteratorOrder->buyer);

    _offers.modify(iteratorOffer, _self, [&](auto &offer) {
        offer.stateOffer = CLOSED;
    });
}

void rideos::deleteoffer(const uint64_t offerKey)
{
    auto iteratorOffer = _offers.find(offerKey);
    eosio_assert(iteratorOffer != _offers.end(), "Offer not found");

    auto indexApply = _applies.get_index<name("byoffer")>();
    auto iteratorApply = indexApply.find(offerKey);

    eosio_assert(iteratorApply == indexApply.end(), "Delete applies first");

    if (iteratorOffer->stateOffer == 1 || iteratorOffer->stateOffer == 2)
    {
        _offers.erase(iteratorOffer);
    }
}

void rideos::addapply(const name account, const uint64_t offerKey)
{
    require_auth(account);

    auto iteratorUser = _users.find(account.value);
    eosio_assert(iteratorUser != _users.end(), "User not found");

    auto iteratorOffer = _offers.find(offerKey);
    eosio_assert(iteratorOffer != _offers.end(), "Offer not found");

    eosio_assert(iteratorOffer->stateOffer == OPEN, "The offer is not open");

    _applies.emplace(_self, [&](auto &apply) {
        apply.applyKey = _applies.available_primary_key();
        apply.deliver = account;
        apply.offerKey = offerKey;
    });
}

void rideos::cancelapply(const uint64_t applyKey)
{
    auto iteratorApply = _applies.find(applyKey);
    eosio_assert(iteratorApply != _applies.end(), "Apply not found");

    auto iteratorOffer = _offers.find(iteratorApply->offerKey);
    eosio_assert(iteratorOffer != _offers.end(), "Offer not found");
    if (iteratorOffer->stateOffer == 0)
    {
        require_auth(iteratorApply->deliver);
    }

    _applies.erase(iteratorApply);
}

EOSIO_DISPATCH(rideos, (adduser)(updateuser)(deleteuser)(deposit)(withdraw)(pay)(stackpow)(unlockpow)(unstackpow)(needdeliver)(deliverfound)(initialize)(validatebuy)(validatedeli)(validatesell)(orderready)(ordertaken)(orderdelive)(initcancel)(delaycancel)(deleteorder)(addoffer)(endoffer)(canceloffer)(deleteoffer)(addapply)(cancelapply))