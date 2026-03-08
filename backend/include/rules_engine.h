#ifndef RULES_ENGINE_H
#define RULES_ENGINE_H

#include "transaction.h"

// ─────────────────────────────────────────────────────────────────────
//  RuleResult — identifies which rule (if any) blocked a transaction.
//
//  SAFE                  → Passed all checks.
//  FRAUD_VELOCITY        → Too many transactions in the time window.
//  FRAUD_DEVICE_ANOMALY  → Device ID changed from the user's known device.
//  FRAUD_IP_HOP          → IP changed within a suspiciously short window.
// ─────────────────────────────────────────────────────────────────────
enum class RuleResult {
  SAFE,
  FRAUD_VELOCITY,
  FRAUD_DEVICE_ANOMALY,
  FRAUD_IP_HOP,
};

// ─────────────────────────────────────────────────────────────────────
//  evaluateTransaction
//
//  Runs all fraud rules against a single transaction, in order:
//    1. Velocity Check   (60-second window, threshold = 3)
//    2. Device Anomaly   (device ID mismatch)
//    3. IP Hopping       (IP change within 300 seconds)
//
//  Short-circuits on the first failure.  If all rules pass, the
//  user's profile is updated with the new timestamp, IP, and device.
// ─────────────────────────────────────────────────────────────────────
RuleResult evaluateTransaction(const Transaction &tx);

/**
 * Returns a human-readable label for a RuleResult value.
 */
const char *ruleResultToString(RuleResult result);

#endif // RULES_ENGINE_H
