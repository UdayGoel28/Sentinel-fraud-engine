#include "rules_engine.h"
#include "user_profile.h"

#include <algorithm> // std::count_if
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

// =====================================================================
//  GLOBAL STATE — per-user profiles keyed by userId.
//
//  std::unordered_map gives O(1) average-case lookup.
//  Each UserProfile stores:
//    • transactionTimes  — vector of approved timestamps (for velocity)
//    • lastKnownIp       — most recent approved IP   (for IP hopping)
//    • lastKnownDevice   — most recent approved device (for device anomaly)
//
//  The map is file-scope (static) so it persists across calls to
//  evaluateTransaction() but is invisible to other translation units.
// =====================================================================
static std::unordered_map<std::string, UserProfile> userState;

// ─────────────────────────────────────────────────────────────────────
//  TUNABLE CONSTANTS
// ─────────────────────────────────────────────────────────────────────
//  VELOCITY_WINDOW_SECONDS   : How far back we look for rapid-fire txns.
//  VELOCITY_MAX_IN_WINDOW    : Max allowed txns in that window.
//  IP_HOP_WINDOW_SECONDS     : If IP changes within this many seconds
//                               of the last transaction, flag it.
// ─────────────────────────────────────────────────────────────────────
static constexpr uint64_t VELOCITY_WINDOW_SECONDS = 60;
static constexpr int VELOCITY_MAX_IN_WINDOW = 3;
static constexpr uint64_t IP_HOP_WINDOW_SECONDS = 300;

// =====================================================================
//  evaluateTransaction — runs all three rules in sequence.
//
//  Rule order:
//    1. Velocity Check   — cheapest (integer comparison on timestamps)
//    2. Device Anomaly   — one string comparison
//    3. IP Hopping       — one string comparison + one integer comparison
//
//  On the first failure the function returns immediately.
//  State is ONLY updated if all rules pass (SAFE).
// =====================================================================
RuleResult evaluateTransaction(const Transaction &tx) {

  // ── Retrieve (or auto-create) the user's profile ──────────────
  //  operator[] inserts a default-constructed UserProfile if the
  //  userId has never been seen — empty vectors and empty strings.
  UserProfile &profile = userState[tx.userId];

  // ================================================================
  //  RULE 1 — Velocity Check (Machine Gun Rule)
  //
  //  Count how many of this user's past approved timestamps fall
  //  within the window [tx.timestamp - 60, tx.timestamp].
  //  If count >= 3, the user is "spraying" transactions.
  // ================================================================
  {
    const uint64_t windowStart = (tx.timestamp >= VELOCITY_WINDOW_SECONDS)
                                     ? (tx.timestamp - VELOCITY_WINDOW_SECONDS)
                                     : 0;
    const uint64_t windowEnd = tx.timestamp;

    // Linear scan over this user's approved timestamps.
    // O(k) where k = number of past approved txns for this user.
    const int recentCount = static_cast<int>(std::count_if(
        profile.transactionTimes.begin(), profile.transactionTimes.end(),
        [windowStart, windowEnd](uint64_t ts) {
          return ts >= windowStart && ts <= windowEnd;
        }));

    if (recentCount >= VELOCITY_MAX_IN_WINDOW) {
      // 🚨 Too many transactions too fast — block without recording.
      return RuleResult::FRAUD_VELOCITY;
    }
  }

  // ================================================================
  //  RULE 2 — Device Anomaly
  //
  //  If the user already has a known device on file, and the
  //  incoming transaction's deviceId is different, flag it.
  //
  //  This catches account takeovers where a fraudster uses a
  //  completely new device.
  //
  //  NOTE: An empty lastKnownDevice means this is the user's first
  //        transaction — we skip the check and let it through so the
  //        engine can learn the baseline device.
  // ================================================================
  if (!profile.lastKnownDevice.empty() &&
      tx.deviceId != profile.lastKnownDevice) {
    // 🚨 Device mismatch — potential account takeover.
    return RuleResult::FRAUD_DEVICE_ANOMALY;
  }

  // ================================================================
  //  RULE 3 — IP Hopping
  //
  //  If the user's IP address has changed since their last approved
  //  transaction AND the time gap is less than 300 seconds (5 min),
  //  flag it.
  //
  //  Rationale: a legitimate user's IP rarely changes within minutes
  //  unless they're using a VPN / proxy chain to mask their origin.
  //
  //  We use a simple string comparison on the raw IP — no Geo-IP
  //  library needed.
  //
  //  Two conditions must BOTH be true to trigger:
  //    a) The IP is different from lastKnownIp.
  //    b) The time since the most recent approved transaction is
  //       less than IP_HOP_WINDOW_SECONDS (300s).
  //
  //  If the profile has no prior IP (first transaction), we skip.
  // ================================================================
  if (!profile.lastKnownIp.empty() && tx.ipAddress != profile.lastKnownIp) {
    // IP is different — now check how recently the user transacted.
    if (!profile.transactionTimes.empty()) {
      // The most recent approved timestamp is the last element
      // (we always push_back, so the vector is in chronological order).
      const uint64_t lastTxnTime = profile.transactionTimes.back();
      const uint64_t timeSinceLast =
          (tx.timestamp >= lastTxnTime) ? (tx.timestamp - lastTxnTime) : 0;

      if (timeSinceLast < IP_HOP_WINDOW_SECONDS) {
        // 🚨 IP changed suspiciously fast.
        return RuleResult::FRAUD_IP_HOP;
      }
    }
  }

  // ================================================================
  //  ALL RULES PASSED — update the user's profile.
  //
  //  We record the timestamp, IP, and device so the engine "learns"
  //  this user's normal behavior for future checks.
  //
  //  CRITICAL: This block runs ONLY for approved transactions.
  //  Blocked transactions must never pollute the profile.
  // ================================================================
  profile.transactionTimes.push_back(tx.timestamp);
  profile.lastKnownIp = tx.ipAddress;
  profile.lastKnownDevice = tx.deviceId;

  return RuleResult::SAFE;
}

// ─────────────────────────────────────────────────────────────────────
//  ruleResultToString — maps each enum value to a console-friendly label.
// ─────────────────────────────────────────────────────────────────────
const char *ruleResultToString(RuleResult result) {
  switch (result) {
  case RuleResult::SAFE:
    return "SAFE";
  case RuleResult::FRAUD_VELOCITY:
    return "FRAUD: Velocity Limit Exceeded";
  case RuleResult::FRAUD_DEVICE_ANOMALY:
    return "FRAUD: Device Anomaly";
  case RuleResult::FRAUD_IP_HOP:
    return "FRAUD: Suspicious IP Hop";
  default:
    return "UNKNOWN";
  }
}
