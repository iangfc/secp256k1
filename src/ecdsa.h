// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_ECDSA_
#define _SECP256K1_ECDSA_

#include "num.h"

typedef struct {
    secp256k1_num_t r, s;
} secp256k1_ecdsa_sig_t;

int static secp256k1_ecdsa_sig_parse(secp256k1_ecdsa_sig_t *r, const unsigned char *sig, int size);
int static secp256k1_ecdsa_sig_serialize(unsigned char *sig, int *size, const secp256k1_ecdsa_sig_t *a);
int static secp256k1_ecdsa_sig_verify(const secp256k1_ecdsa_sig_t *sig, const secp256k1_ge_t *pubkey, const secp256k1_num_t *message);
int static secp256k1_ecdsa_sig_sign(secp256k1_ecdsa_sig_t *sig, const secp256k1_scalar_t *seckey, const secp256k1_scalar_t *message, const secp256k1_scalar_t *nonce, int *recid);
int static secp256k1_ecdsa_sig_recover(const secp256k1_ecdsa_sig_t *sig, secp256k1_ge_t *pubkey, const secp256k1_num_t *message, int recid);
void static secp256k1_ecdsa_sig_set_rs(secp256k1_ecdsa_sig_t *sig, const secp256k1_num_t *r, const secp256k1_num_t *s);

#endif
