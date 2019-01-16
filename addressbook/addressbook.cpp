#include <eosiolib/eosio.hpp>

using std::string;
using namespace eosio;

class [[eosio::contract]] addressbook : public eosio::contract{
  public:
    addressbook( name receiver, name code, datastream<const char *> ds) : contract(receiver,code,ds) {}
    [[eosio::action]]
    void create(name user, string name, string address, string tel) {
      require_auth(user);

      address_index addresses(_code, user.value);

      addresses.emplace(user, [&]( auto& row ) {
        row.id = addresses.available_primary_key();
        row.name = name;
        row.address = address;
        row.tel = tel;
      });
    }

    private:
      struct [[eosio::table]] person {
        uint64_t id;
        string name;
        string address;
        string tel;

        auto primary_key() const { return id; }
      };

      typedef eosio::multi_index<"people"_n, person> address_index;
};

EOSIO_DISPATCH(addressbook,(create))
