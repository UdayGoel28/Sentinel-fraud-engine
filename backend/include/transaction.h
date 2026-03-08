#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <cstdint>
#include <string>

/**
 * Core data payload for every transaction flowing through the engine.
 * Kept as a plain-old-data struct — no business logic lives here.
 * All rule evaluation happens in the rules engine layer.
 */
struct Transaction {
    std::string id;          // Unique transaction identifier (e.g. "TXN001")
    std::string userId;      // The user initiating the transaction
    double      amount;      // Transaction value in USD
    std::string ipAddress;   // Source IP of the request
    std::string deviceId;    // Browser / device fingerprint hash
    uint64_t    timestamp;   // Unix epoch seconds (UTC)
};

#endif // TRANSACTION_H
