#ifndef USER_PROFILE_H
#define USER_PROFILE_H

#include <cstdint>
#include <string>
#include <vector>

// =====================================================================
//  UserProfile — per-user memory state for the fraud detection engine.
//
//  One instance exists per userId inside the global
//  std::unordered_map<std::string, UserProfile>.
//
//  Fields:
//    transactionTimes  – Unix timestamps of past *approved* transactions.
//                        Used by the Velocity Check (Machine Gun Rule).
//    lastKnownIp       – The IP address from the most recent approved
//                        transaction.  Used by the IP Hopping rule.
//    lastKnownDevice   – The device ID from the most recent approved
//                        transaction.  Used by the Device Anomaly rule.
//
//  IMPORTANT:  Only approved (non-fraudulent) transactions update
//              these fields.  Blocked transactions are never recorded.
// =====================================================================
struct UserProfile {
  std::vector<uint64_t> transactionTimes;

  // Empty strings mean "no prior transaction recorded yet".
  std::string lastKnownIp;
  std::string lastKnownDevice;
};

#endif // USER_PROFILE_H
