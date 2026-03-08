#ifndef JSON_WRITER_H
#define JSON_WRITER_H

#include "rules_engine.h"
#include "transaction.h"

#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────
//  FlaggedTransaction — a lightweight record for every transaction
//  that was blocked by the rules engine.  We store just enough
//  data to produce the JSON output file.
// ─────────────────────────────────────────────────────────────────────
struct FlaggedTransaction {
  std::string transactionId;
  std::string userId;
  std::string reason; // human-readable rule label
  uint64_t timestamp;
};

// ─────────────────────────────────────────────────────────────────────
//  writeFlaggedTransactionsJSON
//
//  Writes the vector of flagged transactions to `filePath` as a
//  JSON array of objects.  Uses manual string formatting — no
//  external JSON libraries.
//
//  Output format:
//    [
//      {
//        "transactionId": "TXN004",
//        "userId": "user_alpha",
//        "reason": "FRAUD: Velocity Limit Exceeded",
//        "timestamp": 1700000055
//      },
//      ...
//    ]
//
//  Returns true on success, false if the file could not be opened.
// ─────────────────────────────────────────────────────────────────────
bool writeFlaggedTransactionsJSON(
    const std::vector<FlaggedTransaction> &flagged,
    const std::string &filePath);

#endif // JSON_WRITER_H
