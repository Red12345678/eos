/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/types.hpp>
#include <future>

namespace boost { namespace asio {
   class thread_pool;
}}

namespace eosio { namespace chain {

class transaction_metadata;
using transaction_metadata_ptr = std::shared_ptr<transaction_metadata>;
using signing_keys_future_value_type = std::tuple<chain_id_type, fc::microseconds, flat_set<public_key_type>>;
using signing_keys_future_type = std::shared_future<signing_keys_future_value_type>;
using recovery_keys_type = std::pair<fc::microseconds, const flat_set<public_key_type>&>;

/**
 *  This data structure should store context-free cached data about a transaction such as
 *  packed/unpacked/compressed and recovered keys
 */
class transaction_metadata {
   public:
      transaction_id_type                                        id;
      transaction_id_type                                        signed_id;
      packed_transaction_ptr                                     packed_trx;
      signing_keys_future_type                                   signing_keys_future;
      bool                                                       accepted = false;
      bool                                                       implicit = false;
      bool                                                       scheduled = false;

      transaction_metadata() = delete;
      transaction_metadata(const transaction_metadata&) = delete;
      transaction_metadata(transaction_metadata&&) = delete;
      transaction_metadata operator=(transaction_metadata&) = delete;
      transaction_metadata operator=(transaction_metadata&&) = delete;

      explicit transaction_metadata( const signed_transaction& t, uint32_t max_variable_sig_size = UINT32_MAX, packed_transaction::compression_type c = packed_transaction::none )
      :id(t.id()), packed_trx(std::make_shared<packed_transaction>(t, c)) {
         //raw_packed = fc::raw::pack( static_cast<const transaction&>(trx) );
         check_variable_sig_size(max_variable_sig_size);
         signed_id = digest_type::hash(*packed_trx);
      }

      explicit transaction_metadata( const packed_transaction_ptr& ptrx, uint32_t max_variable_sig_size = UINT32_MAX )
      :id(ptrx->id()), packed_trx(ptrx) {
         //raw_packed = fc::raw::pack( static_cast<const transaction&>(trx) );
         check_variable_sig_size(max_variable_sig_size);
         signed_id = digest_type::hash(*packed_trx);
      }

      void check_variable_sig_size(uint32_t max) {
         for(const signature_type& sig : packed_trx->get_signed_transaction().signatures) {
            EOS_ASSERT(sig.variable_size() <= max, sig_variable_size_limit_exception, "signature variable length component size (${s}) greater than subjective maximum (${m})", ("s", sig.variable_size())("m", max));
         }
      }

      // must be called from main application thread
      static signing_keys_future_type
      start_recover_keys( const transaction_metadata_ptr& mtrx, boost::asio::io_context& thread_pool,
                          const chain_id_type& chain_id, fc::microseconds time_limit, uint32_t variable_sig_limit );

      // start_recover_keys must be called first
      recovery_keys_type recover_keys( const chain_id_type& chain_id, uint32_t variable_sig_limit );
};

} } // eosio::chain
