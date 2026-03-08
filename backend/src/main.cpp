#include "rules_engine.h"
#include "transaction.h"

#include <iostream>
#include <sstream>
#include <string>

// =====================================================================
//  Phase 7 — Persistent Daemon Mode (stdin/stdout)
//
//  The engine runs as a long-lived process, reading one transaction
//  per line from stdin and writing a JSON verdict to stdout.
//
//  Input format  (space-delimited, one per line):
//    txId userId amount ip device timestamp
//
//  Output format (one JSON object per line):
//    {"status":"safe","reason":"Transaction approved"}
//    {"status":"blocked","reason":"FRAUD: Velocity Limit Exceeded"}
//
//  The unordered_map inside rules_engine.cpp persists across lines
//  because this is a single long-lived process.  This is what makes
//  the velocity check actually work across multiple requests.
//
//  The process exits cleanly when stdin reaches EOF.
// =====================================================================

/**
 * Escapes a string for safe inclusion in a JSON value.
 */
static std::string escapeJSON(const std::string &input) {
  std::string out;
  out.reserve(input.size());
  for (char ch : input) {
    switch (ch) {
    case '\\':
      out += "\\\\";
      break;
    case '"':
      out += "\\\"";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      out += ch;
      break;
    }
  }
  return out;
}

int main() {
  std::string line;

  // ── Main loop: read one transaction per line from stdin ────────
  while (std::getline(std::cin, line)) {
    if (line.empty())
      continue;

    std::istringstream stream(line);
    Transaction tx;
    std::string amountStr, timestampStr;

    // Parse the 6 space-delimited fields
    if (!(stream >> tx.id >> tx.userId >> amountStr >> tx.ipAddress >>
          tx.deviceId >> timestampStr)) {
      std::cout << "{\"status\":\"error\",\"reason\":\"Invalid input format\"}"
                << std::endl;
      continue;
    }

    // Convert amount (string → double)
    try {
      tx.amount = std::stod(amountStr);
    } catch (...) {
      std::cout << "{\"status\":\"error\",\"reason\":\"Invalid amount\"}"
                << std::endl;
      continue;
    }

    // Convert timestamp (string → uint64_t)
    try {
      tx.timestamp = std::stoull(timestampStr);
    } catch (...) {
      std::cout << "{\"status\":\"error\",\"reason\":\"Invalid timestamp\"}"
                << std::endl;
      continue;
    }

    // ── Evaluate against all fraud rules ────────────────────────
    RuleResult result = evaluateTransaction(tx);

    // ── Output JSON verdict ─────────────────────────────────────
    if (result == RuleResult::SAFE) {
      std::cout << "{\"status\":\"safe\",\"reason\":\"Transaction approved\"}"
                << std::endl;
    } else {
      std::cout << "{\"status\":\"blocked\",\"reason\":\""
                << escapeJSON(ruleResultToString(result)) << "\"}" << std::endl;
    }
    // std::endl flushes the buffer — critical for Node to read
    // the response immediately without waiting for more data.
  }

  return 0;
}
