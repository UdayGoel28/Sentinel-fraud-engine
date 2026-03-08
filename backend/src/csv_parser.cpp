#include "csv_parser.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::vector<Transaction> parseCSV(const std::string &filePath) {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filePath);
  }

  std::vector<Transaction> transactions;
  std::string line;

  // ── Skip header row ──────────────────────────────────────────
  if (!std::getline(file, line)) {
    return transactions; // Empty file — nothing to parse
  }

  // ── Parse data rows ──────────────────────────────────────────
  while (std::getline(file, line)) {
    if (line.empty())
      continue; // skip blank lines

    std::istringstream stream(line);
    std::string token;
    Transaction txn;

    // Column order: id, userId, amount, ipAddress, deviceId, timestamp
    if (!std::getline(stream, txn.id, ','))
      continue;
    if (!std::getline(stream, txn.userId, ','))
      continue;

    if (!std::getline(stream, token, ','))
      continue;
    try {
      txn.amount = std::stod(token);
    } catch (const std::exception &e) {
      std::cerr << "[WARN] Skipping row — bad amount: " << token << "\n";
      continue;
    }

    if (!std::getline(stream, txn.ipAddress, ','))
      continue;
    if (!std::getline(stream, txn.deviceId, ','))
      continue;

    if (!std::getline(stream, token, ','))
      continue;
    try {
      txn.timestamp = std::stoull(token);
    } catch (const std::exception &e) {
      std::cerr << "[WARN] Skipping row — bad timestamp: " << token << "\n";
      continue;
    }

    transactions.push_back(std::move(txn));
  }

  return transactions;
}
