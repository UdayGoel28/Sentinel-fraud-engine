#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include "transaction.h"
#include <string>
#include <vector>

/**
 * Parses a CSV file into a vector of Transaction structs.
 *
 * Expectations:
 *   - First row is a header (skipped automatically).
 *   - Columns: id, userId, amount, ipAddress, deviceId, timestamp
 *   - Delimiter: comma (,)
 *   - No quoted fields or embedded commas in values.
 *
 * Performance notes:
 *   - Single-pass, no regex, no dynamic allocation beyond the result vector.
 *   - O(n) time where n = number of rows.
 */
std::vector<Transaction> parseCSV(const std::string &filePath);

#endif // CSV_PARSER_H
