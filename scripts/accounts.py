import dbs
import bson
import re
import pymongo
from config import *

name_regex = "^[a-z1-5\.]{1,12}$|^[a-z1-5\.]{12}[a-j1-5\.]$"
name_re = re.compile(name_regex)
def check_name(name):
    return name_re.match(name)

golos = dbs.Tables([
        ('accounts', dbs.golos_db['account_object'],           None, [('name',True)]),
        ('keys',     dbs.golos_db['account_authority_object'], None, [('account',True)]),
])

cyberway = dbs.Tables([
        ('permusage',  dbs.cyberway_db['permusage'],               dbs.get_next_id, None),
        ('permission', dbs.cyberway_db['permission'],              dbs.get_next_id, [('owner',False)]),
        ('account',    dbs.cyberway_db['account'],                 dbs.get_next_id, [('id', True),('name',True)]),
        ('accountseq', dbs.cyberway_db['accountseq'],              None,            None),
        ('reslimit',   dbs.cyberway_db['reslimit'],                None,            None),
        ('resusage',   dbs.cyberway_db['resusage'],                None,            None),
        ('balance',    dbs.cyberway_eosio_token_db['accounts'],    None,            None),
        ('vesting',    dbs.cyberway_gls_vesting_db['balances'],    None,            None),
])


def create_permission(doc, name, parent_id):
    permusage = {
	"id" : bson.Int64(cyberway.permusage.nextId()),
	"last_used" : doc["last_owner_update"].isoformat(),
	"_SCOPE_" : "",
	"_PAYER_" : "",
	"_SIZE_" : bson.Int64(16)
    }
    cyberway.permusage.append(permusage)

    permission_id = cyberway.permission.nextId()
    permission = {
	"id" : bson.Int64(permission_id),
	"usage_id" : permusage["id"],
	"parent" : bson.Int64(parent_id),
	"owner" : doc["account"],
	"name" : name,
	"last_updated" : doc["last_owner_update"].isoformat(),
	"auth" : {
	    "threshold" : bson.Int64(1),
	    "keys" : [
		{
		    "key" : doc[name][0],
		    "weight" : bson.Int64(1)
		}
	    ],
	    "accounts" : [ ],
	    "waits" : [ ]
	},
	"_SCOPE_" : "",
	"_PAYER_" : "",
	"_SIZE_" : bson.Int64(91)
    }
    cyberway.permission.append(permission)
    return permission_id



def create_account(doc):
    account_id = cyberway.account.nextId()

    account = {
        "id" : bson.Int64(account_id),
        "name" : doc["name"],
        "vm_type" : bson.Int64(0),
        "vm_version" : bson.Int64(0),
        "privileged" : False,
        "last_code_update" : "1970-01-01T00:00:00.000",
        "code_version" : "0000000000000000000000000000000000000000000000000000000000000000",
        "creation_date" : doc["created"].isoformat(),
        "code" : "",
        "abi" : "",
        "_SCOPE_" : "",
        "_PAYER_" : "",
        "_SIZE_" : bson.Int64(65)
    }
    cyberway.account.append(account)

    accountseq = {
        "id" : bson.Int64(account_id),
        "name" : doc["name"],
        "recv_sequence" : bson.Int64(0),
        "auth_sequence" : bson.Int64(0),
        "code_sequence" : bson.Int64(0),
        "abi_sequence" : bson.Int64(0),
        "_SCOPE_" : "",
        "_PAYER_" : "",
        "_SIZE_" : bson.Int64(48)
    }
    cyberway.accountseq.append(accountseq)

    reslimit = {
        "id" : bson.Int64(account_id),
        "owner" : doc["name"],
        "pending" : False,
        "net_weight" : bson.Int64(-1),
        "cpu_weight" : bson.Int64(-1),
        "ram_bytes" : bson.Int64(-1),
        "_SCOPE_" : "",
        "_PAYER_" : "",
        "_SIZE_" : bson.Int64(41)
    }
    cyberway.reslimit.append(reslimit)

    resusage = {
        "id" : bson.Int64(account_id),
        "owner" : doc["name"],
        "net_usage" : {
	        "last_ordinal" : bson.Int64(0),
	        "value_ex" : bson.Int64(0),
	        "consumed" : bson.Int64(0)
        },
        "cpu_usage" : {
	        "last_ordinal" : bson.Int64(0),
	        "value_ex" : bson.Int64(0),
	        "consumed" : bson.Int64(0)
        },
        "ram_usage" : bson.Int64(2724),
        "_SCOPE_" : "",
        "_PAYER_" : "",
        "_SIZE_" : bson.Int64(64)
    }
    cyberway.resusage.append(resusage)



def convert_amount(amount, precision):
    return bson.Int64(int(amount*(10**precision)))


def create_balance(doc):
    balance = {
	"balance": {
	    "amount": convert_amount(doc["balance_value"], BALANCE_PRECISION),
	    "decs": bson.Int64(BALANCE_PRECISION),
	    "sym": BALANCE_SYMBOL
	},
	"_SCOPE_": doc["name"],
	"_PAYER_": doc["name"],
	"_SIZE_": bson.Int64(16)
    }
    cyberway.balance.append(balance)


def create_vesting(doc):
    vesting = {
	"vesting": {
	    "amount": convert_amount(doc["vesting_shares_value"], VESTING_PRECISION),
	    "decs": bson.Int64(VESTING_PRECISION),
	    "sym": VESTING_SYMBOL
	},
	"delegate_vesting": {
	    "amount": convert_amount(doc["delegated_vesting_shares_value"], VESTING_PRECISION),
	    "decs": bson.Int64(VESTING_PRECISION),
	    "sym": VESTING_SYMBOL
	},
	"received_vesting": {
	    "amount": convert_amount(doc["received_vesting_shares_value"], VESTING_PRECISION),
	    "decs": bson.Int64(VESTING_PRECISION),
	    "sym": VESTING_SYMBOL
	},
	"unlocked_limit": {
	    "amount": bson.Int64(0),
	    "decs": bson.Int64(VESTING_PRECISION),
	    "sym": VESTING_SYMBOL
	},
	"_SCOPE_": doc["name"],
	"_PAYER_": doc["name"],
	"_SIZE_": bson.Int64(64)
    }
    cyberway.vesting.append(vesting)



def convert_accounts(count = -1):
    print("Create exists accounts set")
    exists_account = {}
    for acc in cyberway.account.table.find(projection={'name':1}):
        exists_account[acc['name']] = True

    print("Convert accounts")

    accounts = golos.accounts.table.find({'name':{'$regex':name_regex}},sort=[('name',pymongo.ASCENDING)])
    authorities = golos.keys.table.find({'account':{'$regex':name_regex}},sort=[('account',pymongo.ASCENDING)])

    account_obj = accounts.next()
    auth_obj = authorities.next()

    total_count = 0
    while account_obj and auth_obj:
        while accounts.alive and (account_obj['name'] < auth_obj['account']):
            account_obj = accounts.next()
        while authorities.alive and (account_obj['name'] > auth_obj['account']):
            auth_obj = authorities.next()
        if account_obj['name'] != auth_obj['account']:
            break
        if not account_obj['name'] in exists_account:
            create_account(account_obj)
            create_balance(account_obj)
            create_vesting(account_obj)
            owner_id = create_permission(auth_obj, 'owner', 0)
            active_id = create_permission(auth_obj, 'active', owner_id)
            posting_id = create_permission(auth_obj, 'posting', active_id)

            total_count += 1
            if count != -1 and total_count >= count:
                break

            if total_count % 10000 == 0:
                print("total_count %d"%total_count)
                cyberway.writeCache()

        if not accounts.alive or not authorities.alive:
            break

        account_obj = accounts.next()
        auth_obj = authorities.next()

    cyberway.writeCache()
    print("total_count: %d"%total_count);
