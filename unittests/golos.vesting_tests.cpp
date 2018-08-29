#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>

#include <golos.vesting/golos.vesting.wast.hpp>
#include <golos.vesting/golos.vesting.abi.hpp>

#include <eosio.token/eosio.token.wast.hpp>
#include <eosio.token/eosio.token.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;

class golos_vesting_tester : public tester {
public:

   golos_vesting_tester() {
      produce_blocks( 2 );

      create_accounts( { N(sania), N(pasha), N(golos.vest), N(eosio.token) } );
      produce_blocks( 2 );

      set_code( N(eosio.token), eosio_token_wast );
      set_abi ( N(eosio.token), eosio_token_abi );

      set_code( N(golos.vest), golos_vesting_wast );
      set_abi ( N(golos.vest), golos_vesting_abi );

      produce_blocks();

      const auto& account_token = control->db().get<account_object,by_name>( N(eosio.token) );
      abi_def abi_t;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(account_token.abi, abi_t), true);
      abi_ser_t.set_abi(abi_t, abi_serializer_max_time);

      const auto& account_vesting = control->db().get<account_object,by_name>( N(golos.vest) );
      abi_def abi_v;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(account_vesting.abi, abi_v), true);
      abi_ser_v.set_abi(abi_v, abi_serializer_max_time);
   }

   action_result push_action( const account_name& signer, const action_name &name, const variant_object &data ) {
      string action_type_name = abi_ser_t.get_action_type(name);

      action act;
      act.account = N(eosio.token);
      act.name    = name;
      act.data    = abi_ser_t.variant_to_binary( action_type_name, data, abi_serializer_max_time );

      return base_tester::push_action( std::move(act), uint64_t(signer));
   }

   action_result push_action_golos_vesting( const account_name& signer, const action_name &name, const variant_object &data ) {
      string action_type_name = abi_ser_v.get_action_type(name);

      action act;
      act.account = N(golos.vest);
      act.name    = name;
      act.data    = abi_ser_v.variant_to_binary( action_type_name, data, abi_serializer_max_time );

      return base_tester::push_action( std::move(act), uint64_t(signer));
   }

   fc::variant get_stats( const string& symbolname )
   {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(eosio.token), symbol_code, N(stat), symbol_code );
      return data.empty() ? fc::variant() : abi_ser_t.binary_to_variant( "currency_stats", data, abi_serializer_max_time );
   }

   fc::variant get_account( account_name acc, const string& symbolname)
   {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(eosio.token), acc, N(accounts), symbol_code );
      return data.empty() ? fc::variant() : abi_ser_t.binary_to_variant( "account", data, abi_serializer_max_time );
   }


   fc::variant get_account_vesting( account_name acc, const string& symbolname)
   {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(golos.vest), acc, N(balances), symbol_code );
      return data.empty() ? fc::variant() : abi_ser_v.binary_to_variant( "user_balance", data, abi_serializer_max_time );
   }

   action_result create( account_name issuer,
                         asset maximum_supply ) {

      return push_action( N(eosio.token), N(create), mvo()
           ( "issuer", issuer)
           ( "maximum_supply", maximum_supply)
      );
   }

   action_result issue( account_name issuer, account_name to, asset quantity, string memo ) {
      return push_action( issuer, N(issue), mvo()
           ( "to", to)
           ( "quantity", quantity)
           ( "memo", memo)
      );
   }

   action_result transfer( account_name from,
                  account_name to,
                  asset        quantity,
                  string       memo ) {
      return push_action( from, N(transfer), mvo()
           ( "from", from)
           ( "to", to)
           ( "quantity", quantity)
           ( "memo", memo)
      );
   }

   action_result accrue_vesting(account_name sender, account_name user, asset quantity) {
      return push_action_golos_vesting( sender, N(accruevg), mvo()
           ( "sender", sender)
           ( "user", user)
           ( "quantity", quantity)
      );
   }

   action_result convert_vesting(account_name sender, account_name recipient, asset quantity) {
       return push_action_golos_vesting( sender, N(convertvg), mvo()
            ( "sender", sender)
            ( "recipient", recipient)
            ( "quantity", quantity)
       );
   }


   action_result cancel_convert_vesting(account_name sender, asset type) {
       return push_action_golos_vesting( sender, N(cancelvg), mvo()
            ( "sender", sender)
            ( "type", type)
       );
   }


   action_result delegate_vesting(account_name sender, account_name recipient, asset quantity, uint16_t percentage_deductions) {
       return push_action_golos_vesting( sender, N(delegatevg), mvo()
            ( "sender", sender)
            ( "recipient", recipient)
            ( "quantity", quantity)
            ( "percentage_deductions", percentage_deductions)
       );
   }


   action_result undelegate_vesting(account_name sender, account_name recipient, asset quantity) {
       return push_action_golos_vesting( sender, N(undelegatevg), mvo()
            ( "sender", sender)
            ( "recipient", recipient)
            ( "quantity", quantity)
       );
   }

   action_result create_pair(asset token, asset vesting) {
       return push_action_golos_vesting( N(golos.vest), N(createpair), mvo()
                           ("token", token)
                           ("vesting", vesting)
                           );
   }

   abi_serializer abi_ser_v;
   abi_serializer abi_ser_t;
};

BOOST_AUTO_TEST_SUITE(eosio_token_tests)

BOOST_FIXTURE_TEST_CASE( create_tokens, golos_vesting_tester ) try {
   auto token = create( N(eosio.token), asset::from_string("100000.0000 GOLOS"));
   auto stats = get_stats("4,GOLOS");

   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "0.0000 GOLOS")
      ("max_supply", "100000.0000 GOLOS")
      ("issuer", "eosio.token")
   );
//   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( create_pair_token_and_vesting, golos_vesting_tester ) try {
    create_pair(asset::from_string("0.0000 GOLOS"), asset::from_string("0.0000 VEST"));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( issue_tokens_accounts, golos_vesting_tester ) try {
    auto token = create( N(eosio.token), asset::from_string("100000.0000 GOLOS"));

    issue( N(eosio.token), N(sania), asset::from_string("500.0000 GOLOS"), "issue tokens sania" );
    issue( N(eosio.token), N(pasha), asset::from_string("500.0000 GOLOS"), "issue tokens pasha" );
    produce_blocks(1);

    auto stats = get_stats("4,GOLOS");
    REQUIRE_MATCHING_OBJECT( stats, mvo()
       ("supply", "1000.0000 GOLOS")
       ("max_supply", "100000.0000 GOLOS")
       ("issuer", "eosio.token")
    );

    auto sania_balance = get_account(N(sania), "4,GOLOS");
    REQUIRE_MATCHING_OBJECT( sania_balance, mvo()
                            ("balance", "500.0000 GOLOS")
    );


    auto pasha_balance = get_account(N(pasha), "4,GOLOS");
    REQUIRE_MATCHING_OBJECT( pasha_balance, mvo()
                            ("balance", "500.0000 GOLOS")
    );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( convert_token_to_vesting, golos_vesting_tester ) try {
    auto token = create( N(eosio.token), asset::from_string("100000.0000 GOLOS"));

    issue( N(eosio.token), N(sania), asset::from_string("500.0000 GOLOS"), "issue tokens sania" );
    issue( N(eosio.token), N(pasha), asset::from_string("500.0000 GOLOS"), "issue tokens pasha" );
    produce_blocks(1);

    auto status_create_pair = create_pair(asset::from_string("0.0000 GOLOS"), asset::from_string("0.0000 VEST"));
    auto status_transfer_1 = transfer( N(sania), N(golos.vest), asset::from_string("100.0000 GOLOS"), "convert token to vesting" );
    auto status_transfer_2 = transfer( N(pasha), N(golos.vest), asset::from_string("100.0000 GOLOS"), "convert token to vesting" );

    produce_blocks(1);

    auto sania_token_balance = get_account(N(sania), "4,GOLOS");
    REQUIRE_MATCHING_OBJECT( sania_token_balance, mvo()
                             ("balance", "400.0000 GOLOS") );

    auto pasha_token_balance = get_account(N(pasha), "4,GOLOS");
    REQUIRE_MATCHING_OBJECT( pasha_token_balance, mvo()
                             ("balance", "400.0000 GOLOS") );

    auto sania_vesting_balance = get_account_vesting(N(sania), "4,VEST");
    REQUIRE_MATCHING_OBJECT( sania_vesting_balance, mvo()
                             ("vesting", "100.0000 VEST")
                             ("delegate_vesting", "0.0000 VEST")
                             ("received_vesting", "0.0000 VEST") );

    auto pasha_vesting_balance = get_account_vesting(N(pasha), "4,VEST");
    REQUIRE_MATCHING_OBJECT( pasha_vesting_balance, mvo()
                             ("vesting", "100.0000 VEST")
                             ("delegate_vesting", "0.0000 VEST")
                             ("received_vesting", "0.0000 VEST") );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( accrue_vesting_user, golos_vesting_tester ) try {
    auto status = create_pair(asset::from_string("0.0000 GOLOS"), asset::from_string("0.0000 VEST"));

    auto status_1 = accrue_vesting(N(golos.vest), N(sania), asset::from_string("15.0000 VEST"));
    auto status_2 = accrue_vesting(N(golos.vest), N(pasha), asset::from_string("15.0000 VEST"));

    auto sania_vesting_balance = get_account_vesting(N(sania), "4,VEST");
    REQUIRE_MATCHING_OBJECT( sania_vesting_balance, mvo()
                             ("vesting", "15.0000 VEST")
                             ("delegate_vesting", "0.0000 VEST")
                             ("received_vesting", "0.0000 VEST") );

    auto pasha_vesting_balance = get_account_vesting(N(pasha), "4,VEST");
    REQUIRE_MATCHING_OBJECT( pasha_vesting_balance, mvo()
                             ("vesting", "15.0000 VEST")
                             ("delegate_vesting", "0.0000 VEST")
                             ("received_vesting", "0.0000 VEST") );
} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_SUITE_END()
