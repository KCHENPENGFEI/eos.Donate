#pragma once

//包含需要的头文件
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/symbol.hpp>
#include <string>

//宏定义 MAX_ZJUBCA_QUANTITY用于检查合约账户中ZJUBCA数量是否超过10000.0000 ZJUBCA
//MAX_EOS_QUANTITY 用于检查合约账户中EOS数量是否超过1000.000 EOS  (此处ZJUBCA精度是4位，EOS是三位)
#define MAX_ZJUBCA_QUANTITY 100000000
#define MAX_EOS_QUANTITY 1000000

using namespace eosio;
using namespace std;

namespace eosiosystem {
class system_contract;
}

//定义zjubca.donate合约这个类
class [[eosio::contract("zjubca.donate")]] Donate : public contract
{
private:
    //定义donator表的结构，该表用于存储捐赠者的基本信息
    //主键是donator_name，类型是name，ZJUBCA_amount和EOS_amount用于保存该用户总共捐赠的ZJUBCA和EOS数量，类型是assset
    // @abi table doantor
    struct [[eosio::table]] donator {
        name donator_name;
        asset ZJUBCA_amount;
        asset EOS_amount;

        auto primary_key() const {return donator_name.value;}  //声明主键是donator_name.value
    };

    //定义recipient表的结构，该表用于存储该合约的基本信息
    //主键是recipient_name，类型是name，(其实这个数据只有本合约)ZJUBCA_amount和EOS_amount用于保存本合约总共收到捐赠的ZJUBCA和EOS数量，类型是assset
    // @abi table recipient
    struct [[eosio::table]] recipient {
        name recipient_name;
        asset ZJUBCA_amount;
        asset EOS_amount;

        auto primary_key() const {return recipient_name.value;}  ////声明主键是recipient_name.value
    };

    //定义foundation表的结构，该表用于存储基金会的基本信息，和recipient类似
    // @abi table foundation
    struct [[eosio::table]] foundation {
        name foundation_name;
        asset ZJUBCA_amount;
        asset EOS_amount;

        auto primary_key() const {return foundation_name.value;}
    };
    

    //mutil_index<"donators"_n, donator>是mutil_index的初始化, (_donators是一个类)现在我们可以拿着_donators去实例化一张表
    typedef multi_index<"donators"_n, donator> _donators;
    typedef multi_index<"recipients"_n, recipient> _recipients;
    typedef multi_index<"foundation"_n, foundation> _foundation;

    _donators donator;      //实例化donator这张表
    _recipients recipient;     //实例化recipent这张表
    _foundation foundation;      //实例化foundation这张表

public:
    using contract::contract;

    //构造函数以及三张表的初始化
    Donate(name receiver, name code, datastream<const char*> ds):contract(receiver, code, ds), 
        donator(receiver, code.value), recipient(receiver, code.value), foundation(receiver, code.value){}

    //声明各个action
    [[eosio::action]] void start();     //合约开始的时候要执行start， 向recipient和foundation这两张表中插入数据
    [[eosio::action]] void end();       //删除表，需要时可以执行
    [[eosio::action]] void donatezjubca(name from, name to, asset quantity, string memo);    //用户捐赠ZJUBCA给合约
    [[eosio::action]] void donateeos(name from, name to, asset quantity, string memo);       //用户捐赠EOS给合约
    [[eosio::action]] void sendtofound(name from, name to, string memo);     //检查合约账户中EOS数量是否超过1000.000 EOS，以及ZJUBCA数量是否超过10000.0000 ZJUBCA，然后将其转到基金会
};
