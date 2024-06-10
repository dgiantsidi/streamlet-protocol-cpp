#pragma once

#include <iostream>
#include <memory>
#include <openssl/sha.h>
#include <stdint.h>
#include <vector>

// configurations of the system

constexpr int kNodes = 4; // 3f+1
constexpr int kRounds = 10;
constexpr int kEpochDuration = 2;
constexpr int kFinalEpochVal = 11;

constexpr size_t k_hash_size = SHA256_DIGEST_LENGTH; /* do not modify */
constexpr size_t k_payload_size = 64;
constexpr size_t k_signature_size = 256; /* do not modify */