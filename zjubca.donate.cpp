#include "zjubca.donate.hpp"

void Donate::start()
{
    //确保只有本合约才有权力执行start这个action
    require_auth(_self);

    name recipient_account = name("donate");
    name foundation_account = name("zjubca");

    //查找recipient表中是否已经存在donate这个字段
    auto existing1 = recipient.find(recipient_account.value);
    //查找foundation表中是否已经存在zjubca这个字段
    auto existing2 = foundation.find(foundation_account.value);
    if (existing1 == recipient.end())
    {
        //如果没找到，插入初始化数据
        recipient.emplace(_self, [&](auto &new_recipient) {
            new_recipient.recipient_name = recipient_account;
            new_recipient.ZJUBCA_amount = asset(0, symbol("ZJUBCA", 4));
            new_recipient.EOS_amount = asset(0, symbol("EOS", 3));
        });
    }
    if (existing2 == foundation.end())
    {
        //如果没有找到，插入初始化数据
        foundation.emplace(_self, [&](auto &new_foundation) {
            new_foundation.foundation_name = foundation_account;
            new_foundation.ZJUBCA_amount = asset(0, symbol("ZJUBCA", 4));
            new_foundation.EOS_amount = asset(0, symbol("EOS", 3));
        });
    }
}

void Donate::donatezjubca(name from, name to, asset quantity, string memo)
{
    //建议先看apply函数(代码最后)中的注释代码

    //检查memo是否是"donatezjubca"，防止其他转账操作而进入本action
    if (memo == "donatezjubca")
    {
        //进行一些assert操作
        eosio_assert(from != to, "cannot transfer to self");
        require_auth(from);
        //接收ZJUBCA的地址只能是本合约
        eosio_assert(to == _self, "only can donate to the contract");

        auto sym = quantity.symbol;
        eosio_assert(sym.is_valid(), "invalid symbol name");
        eosio_assert(sym == symbol("ZJUBCA", 4), "invalid symbol name");

        eosio_assert(quantity.is_valid(), "invalid quantity");
        eosio_assert(quantity.amount > 0, "must transfer positive quantity");

        eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

        //先从donators表中查询捐赠者
        auto existing1 = donator.find(from.value);
        if (existing1 == donator.end())
        {
            //不存在就在新增一个用户
            donator.emplace(_self, [&](auto &new_donator) {
                new_donator.donator_name = from;
                new_donator.ZJUBCA_amount = quantity;
                symbol sym = symbol("EOS", 3);
                auto zero = asset(0, sym);
                new_donator.EOS_amount = zero;
            });
        }
        else
        {
            //若已经存在，则修改用户数据
            donator.modify(existing1, same_payer, [&](auto &content) {
                content.ZJUBCA_amount += quantity;
            });
        }

        auto existing2 = recipient.find(to.value);
        if (existing2 != recipient.end())
        {
            //修改recipient表中数据
            recipient.modify(existing2, same_payer, [&](auto &content) {
                content.ZJUBCA_amount += quantity;
            });
        }
    }
}

void Donate::donateeos(name from, name to, asset quantity, string memo)
{
    //和doantezjubca逻辑类似
    if (memo == "donateeos")
    {
        eosio_assert(from != to, "cannot transfer to self");
        require_auth(from);
        eosio_assert(to == _self, "only can donate to the contract");

        auto sym = quantity.symbol;
        eosio_assert(sym.is_valid(), "invalid symbol name");
        eosio_assert(sym == symbol("EOS", 3), "invalid symbol name");

        eosio_assert(quantity.is_valid(), "invalid quantity");
        eosio_assert(quantity.amount > 0, "must transfer positive quantity");

        eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

        auto existing1 = donator.find(from.value);
        if (existing1 == donator.end())
        {
            donator.emplace(_self, [&](auto &new_donator) {
                new_donator.donator_name = from;
                new_donator.EOS_amount = quantity;
                symbol sym = symbol("ZJUBCA", 4);
                auto zero = asset(0, sym);
                new_donator.ZJUBCA_amount = zero;
            });
        }
        else
        {
            donator.modify(existing1, same_payer, [&](auto &content) {
                content.EOS_amount += quantity;
            });
        }

        auto existing2 = recipient.find(to.value);
        if (existing2 != recipient.end())
        {
            recipient.modify(existing2, same_payer, [&](auto &content) {
                content.EOS_amount += quantity;
            });
        }
    }
}

void Donate::end()
{
    //删除表操作
    auto donator_begin_it = donator.begin();
    while (donator_begin_it != donator.end())
    {
        donator_begin_it = donator.erase(donator_begin_it);
    }

    auto recipient_begin_it = recipient.begin();
    while (recipient_begin_it != recipient.end())
    {
        recipient_begin_it = recipient.erase(recipient_begin_it);
    }

    auto foundation_begin_it = foundation.begin();
    while (foundation_begin_it != foundation.end())
    {
        foundation_begin_it = foundation.erase(foundation_begin_it);
    }
}

void Donate::sendtofound(name from, name to, string memo)
{
    //assert
    eosio_assert(from != to, "cannot transfer to self");
    require_auth(from);
    eosio_assert(from == _self, "invalid transfer account");
    eosio_assert(to == name("zjubca"), "invalid transfer account");

    asset ZJUBCA_quantity = asset(MAX_ZJUBCA_QUANTITY, symbol("ZJUBCA", 4));
    asset EOS_quantity = asset(MAX_EOS_QUANTITY, symbol("EOS", 3));
    auto existing1 = recipient.find(from.value);
    if (existing1 != recipient.end())
    {
        const auto &st1 = *existing1;
        if (st1.ZJUBCA_amount >= ZJUBCA_quantity)
        {
            //查表并且检查合约地址中token数量是否发超过一定限额
            recipient.modify(st1, from, [&](auto &content) {
                content.ZJUBCA_amount -= ZJUBCA_quantity;
            });

            auto existing2 = foundation.find(to.value);
            if (existing2 != foundation.end())
            {
                const auto &st2 = *existing2;
                foundation.modify(st2, from, [&](auto &content) {
                    content.ZJUBCA_amount += ZJUBCA_quantity;
                });
            }
            action(
                permission_level{from, "active"_n},
                "zjubca.token"_n,
                "transfer"_n,
                std::make_tuple(from, to, ZJUBCA_quantity, memo))
                .send();
        }
        if (st1.EOS_amount >= EOS_quantity)
        {
            //逻辑同上
            recipient.modify(st1, from, [&](auto &content) {
                content.EOS_amount -= EOS_quantity;
            });

            auto existing2 = foundation.find(to.value);
            if (existing2 != foundation.end())
            {
                const auto &st2 = *existing2;
                foundation.modify(st2, from, [&](auto &content) {
                    content.EOS_amount += EOS_quantity;
                });
            }
            //此处由于是合约将自己的token进行转出，所以可以直接使用inline action操作，不存在安全性问题
            action(
                permission_level{from, "active"_n},
                "eosio.token"_n,
                "transfer"_n,
                std::make_tuple(from, to, EOS_quantity, memo))
                .send();
        }
    }
}

// EOSIO_DISPATCH(Donate, (start)(donatezjubca)(donateeos)(sendtofound)(end))
extern "C"
{
    //apply函数是所有EOS合约的入口
    //在EOS中，合约之间是可以相互调用的，假设A合约在B这个action中调用了C合约，那么receiver就是C合约，code就是A合约，action就是B这个action
    void apply(uint64_t receiver, uint64_t code, uint64_t action)
    {
        //判断是action的入口是本合约还是别的合约
        //如果调用方和被调用方式相同的，并且不是从transfer这个action进入的
        if (code == receiver && action != "transfer"_n.value)
        {
            switch (action)
            {
            //进入不同的action
            case "start"_n.value:
                execute_action(name(receiver), name(code), &Donate::start);    //使用execute_action进入相应的action
                break;
            case "end"_n.value:
                execute_action(name(receiver), name(code), &Donate::end);
                break;
            case "sendtofound"_n.value:
                execute_action(name(receiver), name(code), &Donate::sendtofound);
                break;
            default:
                break;
            }
        }
        //code == zjubca.token和code == eosio.token意为调用本合约的合约为zjubca.token和eosio.token，并且是从transfer这个action进入的
        else if ((code == "zjubca.token"_n.value || code == "eosio.token"_n.value) && action == "transfer"_n.value)
        {
            switch (code)
            {
            //如果是zjubca.token进入本合约，进行donatezjubca这个action
            case "zjubca.token"_n.value:
                execute_action(name(receiver), name(receiver), &Donate::donatezjubca);
                break;
            //如果是eosio.token进入本合约，进行donateeos这个action
            case "eosio.token"_n.value:
                execute_action(name(receiver), name(receiver), &Donate::donateeos);
                break;
            default:
                break;
            }
        }
    }
};