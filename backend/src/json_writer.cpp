#include "json_writer.h"

#include <fstream>
#include <sstream>

// =====================================================================
//  escapeJSON — escapes characters that are special in JSON strings.
//
//  We handle:  \ → \\,  " → \",  newline → \n,  tab → \t
//
//  For our use case (transaction IDs, user IDs, rule labels) none of
//  these should appear — but it's good practice to be safe.
// =====================================================================
static std::string escapeJSON(const std::string &input) {
  std::string output;
  output.reserve(input.size());

  for (char ch : input) {
    switch (ch) {
    case '\\':
      output += "\\\\";
      break;
    case '"':
      output += "\\\"";
      break;
    case '\n':
      output += "\\n";
      break;
    case '\t':
      output += "\\t";
      break;
    default:
      output += ch;
      break;
    }
  }
  return output;
}

// =====================================================================
//  writeFlaggedTransactionsJSON
//
//  Builds the JSON string manually using std::ostringstream, then
//  writes it to the output file in a single pass.
//
//  Step-by-step:
//    1. Open the output file with std::ofstream.
//    2. Start the JSON array "[".
//    3. For each FlaggedTransaction, emit an object with 4 keys.
//       - Use a comma separator between objects (but not after the last one).
//    4. Close the array "]".
//    5. Flush and check for write errors.
// =====================================================================
bool writeFlaggedTransactionsJSON(
    const std::vector<FlaggedTransaction> &flagged,
    const std::string &filePath) {

  // ── Step 1: Open the file ─────────────────────────────────────
  std::ofstream outFile(filePath);
  if (!outFile.is_open()) {
    return false;
  }

  // ── Step 2–4: Build the JSON string ───────────────────────────
  std::ostringstream json;
  json << "[\n";

  for (size_t i = 0; i < flagged.size(); ++i) {
    const auto &f = flagged[i];

    json << "  {\n";
    json << "    \"transactionId\": \"" << escapeJSON(f.transactionId)
         << "\",\n";
    json << "    \"userId\": \"" << escapeJSON(f.userId) << "\",\n";
    json << "    \"reason\": \"" << escapeJSON(f.reason) << "\",\n";
    json << "    \"timestamp\": " << f.timestamp << "\n";
    json << "  }";

    // Add a comma after every object except the last one.
    // This keeps the output as valid JSON (trailing commas are illegal).
    if (i + 1 < flagged.size()) {
      json << ",";
    }
    json << "\n";
  }

  json << "]\n";

  // ── Step 5: Write and verify ──────────────────────────────────
  outFile << json.str();
  outFile.close();

  return !outFile.fail();
}
