// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "include/secp256k1.h"

#include <assert.h>
#include "util.h"
#include "num_impl.h"
#include "field_impl.h"
#include "scalar_impl.h"
#include "group_impl.h"
#include "ecmult_impl.h"
#include "ecmult_gen_impl.h"
#include "ecdsa_impl.h"
#include "eckey_impl.h"

void secp256k1_start(unsigned int flags) {
    secp256k1_fe_start();
    secp256k1_ge_start();
    if (flags & SECP256K1_START_SIGN) {
        secp256k1_ecmult_gen_start();
    }
    if (flags & SECP256K1_START_VERIFY) {
        secp256k1_ecmult_start();
    }
}

void secp256k1_stop(void) {
    secp256k1_ecmult_stop();
    secp256k1_ecmult_gen_stop();
    secp256k1_ge_stop();
    secp256k1_fe_stop();
}

int secp256k1_ecdsa_verify(const unsigned char *msg, int msglen, const unsigned char *sig, int siglen, const unsigned char *pubkey, int pubkeylen) {
    DEBUG_CHECK(secp256k1_ecmult_consts != NULL);
    DEBUG_CHECK(msg != NULL);
    DEBUG_CHECK(msglen <= 32);
    DEBUG_CHECK(sig != NULL);
    DEBUG_CHECK(pubkey != NULL);

    int ret = -3;
    secp256k1_num_t m; 
    secp256k1_ecdsa_sig_t s;
    secp256k1_ge_t q;
    secp256k1_num_set_bin(&m, msg, msglen);

    if (!secp256k1_eckey_pubkey_parse(&q, pubkey, pubkeylen)) {
        ret = -1;
        goto end;
    }
    if (!secp256k1_ecdsa_sig_parse(&s, sig, siglen)) {
        ret = -2;
        goto end;
    }
    if (!secp256k1_ecdsa_sig_verify(&s, &q, &m)) {
        ret = 0;
        goto end;
    }
    ret = 1;
end:
    return ret;
}

int secp256k1_ecdsa_sign(const unsigned char *message, int messagelen, unsigned char *signature, int *signaturelen, const unsigned char *seckey, const unsigned char *nonce) {
    DEBUG_CHECK(secp256k1_ecmult_gen_consts != NULL);
    DEBUG_CHECK(message != NULL);
    DEBUG_CHECK(messagelen <= 32);
    DEBUG_CHECK(signature != NULL);
    DEBUG_CHECK(signaturelen != NULL);
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(nonce != NULL);

    secp256k1_scalar_t sec, non, msg;
    secp256k1_scalar_set_b32(&sec, seckey, NULL);
    int overflow = 0;
    secp256k1_scalar_set_b32(&non, nonce, &overflow);
    {
        unsigned char c[32] = {0};
        memcpy(c + 32 - messagelen, message, messagelen);
        secp256k1_scalar_set_b32(&msg, c, NULL);
        memset(c, 0, 32);
    }
    int ret = !secp256k1_scalar_is_zero(&non) && !overflow;
    secp256k1_ecdsa_sig_t sig;
    if (ret) {
        ret = secp256k1_ecdsa_sig_sign(&sig, &sec, &msg, &non, NULL);
    }
    if (ret) {
        secp256k1_ecdsa_sig_serialize(signature, signaturelen, &sig);
    }
    secp256k1_scalar_clear(&msg);
    secp256k1_scalar_clear(&non);
    secp256k1_scalar_clear(&sec);
    return ret;
}

int secp256k1_ecdsa_sign_compact(const unsigned char *message, int messagelen, unsigned char *sig64, const unsigned char *seckey, const unsigned char *nonce, int *recid) {
    DEBUG_CHECK(secp256k1_ecmult_gen_consts != NULL);
    DEBUG_CHECK(message != NULL);
    DEBUG_CHECK(messagelen <= 32);
    DEBUG_CHECK(sig64 != NULL);
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(nonce != NULL);

    secp256k1_scalar_t sec, non, msg;
    secp256k1_scalar_set_b32(&sec, seckey, NULL);
    int overflow = 0;
    secp256k1_scalar_set_b32(&non, nonce, &overflow);
    {
        unsigned char c[32] = {0};
        memcpy(c + 32 - messagelen, message, messagelen);
        secp256k1_scalar_set_b32(&msg, c, NULL);
        memset(c, 0, 32);
    }
    int ret = !secp256k1_scalar_is_zero(&non) && !overflow;
    secp256k1_ecdsa_sig_t sig;
    if (ret) {
        ret = secp256k1_ecdsa_sig_sign(&sig, &sec, &msg, &non, recid);
    }
    if (ret) {
        secp256k1_num_get_bin(sig64, 32, &sig.r);
        secp256k1_num_get_bin(sig64 + 32, 32, &sig.s);
    }
    secp256k1_scalar_clear(&msg);
    secp256k1_scalar_clear(&non);
    secp256k1_scalar_clear(&sec);
    return ret;
}

int secp256k1_ecdsa_recover_compact(const unsigned char *msg, int msglen, const unsigned char *sig64, unsigned char *pubkey, int *pubkeylen, int compressed, int recid) {
    DEBUG_CHECK(secp256k1_ecmult_consts != NULL);
    DEBUG_CHECK(msg != NULL);
    DEBUG_CHECK(msglen <= 32);
    DEBUG_CHECK(sig64 != NULL);
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(pubkeylen != NULL);
    DEBUG_CHECK(recid >= 0 && recid <= 3);

    int ret = 0;
    secp256k1_num_t m; 
    secp256k1_ecdsa_sig_t sig;
    secp256k1_num_set_bin(&sig.r, sig64, 32);
    secp256k1_num_set_bin(&sig.s, sig64 + 32, 32);
    secp256k1_num_set_bin(&m, msg, msglen);

    secp256k1_ge_t q;
    if (secp256k1_ecdsa_sig_recover(&sig, &q, &m, recid)) {
        secp256k1_eckey_pubkey_serialize(&q, pubkey, pubkeylen, compressed);
        ret = 1;
    }
    return ret;
}

int secp256k1_ec_seckey_verify(const unsigned char *seckey) {
    DEBUG_CHECK(seckey != NULL);

    secp256k1_scalar_t sec;
    int overflow;
    secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    int ret = !secp256k1_scalar_is_zero(&sec) && !overflow;
    secp256k1_scalar_clear(&sec);
    return ret;
}

int secp256k1_ec_pubkey_verify(const unsigned char *pubkey, int pubkeylen) {
    DEBUG_CHECK(pubkey != NULL);

    secp256k1_ge_t q;
    return secp256k1_eckey_pubkey_parse(&q, pubkey, pubkeylen);
}

int secp256k1_ec_pubkey_create(unsigned char *pubkey, int *pubkeylen, const unsigned char *seckey, int compressed) {
    DEBUG_CHECK(secp256k1_ecmult_gen_consts != NULL);
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(pubkeylen != NULL);
    DEBUG_CHECK(seckey != NULL);

    secp256k1_scalar_t sec;
    secp256k1_scalar_set_b32(&sec, seckey, NULL);
    secp256k1_gej_t pj;
    secp256k1_ecmult_gen(&pj, &sec);
    secp256k1_scalar_clear(&sec);
    secp256k1_ge_t p;
    secp256k1_ge_set_gej(&p, &pj);
    secp256k1_eckey_pubkey_serialize(&p, pubkey, pubkeylen, compressed);
    return 1;
}

int secp256k1_ec_pubkey_decompress(unsigned char *pubkey, int *pubkeylen) {
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(pubkeylen != NULL);

    secp256k1_ge_t p;
    if (!secp256k1_eckey_pubkey_parse(&p, pubkey, *pubkeylen))
        return 0;
    secp256k1_eckey_pubkey_serialize(&p, pubkey, pubkeylen, 0);
    return 1;
}

int secp256k1_ec_privkey_tweak_add(unsigned char *seckey, const unsigned char *tweak) {
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(tweak != NULL);

    secp256k1_scalar_t term;
    int overflow = 0;
    secp256k1_scalar_set_b32(&term, tweak, &overflow);
    secp256k1_scalar_t sec;
    secp256k1_scalar_set_b32(&sec, seckey, NULL);

    int ret = secp256k1_eckey_privkey_tweak_add(&sec, &term) && !overflow;
    if (ret) {
        secp256k1_scalar_get_b32(seckey, &sec);
    }

    secp256k1_scalar_clear(&sec);
    secp256k1_scalar_clear(&term);
    return ret;
}

int secp256k1_ec_pubkey_tweak_add(unsigned char *pubkey, int pubkeylen, const unsigned char *tweak) {
    DEBUG_CHECK(secp256k1_ecmult_consts != NULL);
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(tweak != NULL);

    secp256k1_num_t term;
    secp256k1_num_set_bin(&term, tweak, 32);
    secp256k1_ge_t p;
    int ret = secp256k1_eckey_pubkey_parse(&p, pubkey, pubkeylen);
    if (ret) {
        ret = secp256k1_eckey_pubkey_tweak_add(&p, &term);
    }
    if (ret) {
        int oldlen = pubkeylen;
        secp256k1_eckey_pubkey_serialize(&p, pubkey, &pubkeylen, oldlen <= 33);
        VERIFY_CHECK(pubkeylen == oldlen);
    }

    return ret;
}

int secp256k1_ec_privkey_tweak_mul(unsigned char *seckey, const unsigned char *tweak) {
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(tweak != NULL);

    secp256k1_scalar_t factor;
    int overflow = 0;
    secp256k1_scalar_set_b32(&factor, tweak, &overflow);
    secp256k1_scalar_t sec;
    secp256k1_scalar_set_b32(&sec, seckey, NULL);
    int ret = secp256k1_eckey_privkey_tweak_mul(&sec, &factor) && !overflow;
    if (ret) {
        secp256k1_scalar_get_b32(seckey, &sec);
    }

    secp256k1_scalar_clear(&sec);
    secp256k1_scalar_clear(&factor);
    return ret;
}

int secp256k1_ec_pubkey_tweak_mul(unsigned char *pubkey, int pubkeylen, const unsigned char *tweak) {
    DEBUG_CHECK(secp256k1_ecmult_consts != NULL);
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(tweak != NULL);

    secp256k1_num_t factor;
    secp256k1_num_set_bin(&factor, tweak, 32);
    secp256k1_ge_t p;
    int ret = secp256k1_eckey_pubkey_parse(&p, pubkey, pubkeylen);
    if (ret) {
        ret = secp256k1_eckey_pubkey_tweak_mul(&p, &factor);
    }
    if (ret) {
        int oldlen = pubkeylen;
        secp256k1_eckey_pubkey_serialize(&p, pubkey, &pubkeylen, oldlen <= 33);
        VERIFY_CHECK(pubkeylen == oldlen);
    }

    return ret;
}

int secp256k1_ec_privkey_export(const unsigned char *seckey, unsigned char *privkey, int *privkeylen, int compressed) {
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(privkey != NULL);
    DEBUG_CHECK(privkeylen != NULL);

    secp256k1_scalar_t key;
    secp256k1_scalar_set_b32(&key, seckey, NULL);
    int ret = secp256k1_eckey_privkey_serialize(privkey, privkeylen, &key, compressed);
    secp256k1_scalar_clear(&key);
    return ret;
}

int secp256k1_ec_privkey_import(unsigned char *seckey, const unsigned char *privkey, int privkeylen) {
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(privkey != NULL);

    secp256k1_scalar_t key;
    int ret = secp256k1_eckey_privkey_parse(&key, privkey, privkeylen);
    if (ret)
        secp256k1_scalar_get_b32(seckey, &key);
    secp256k1_scalar_clear(&key);
    return ret;
}
