/**
 * ============================================================
 *  SENTINEL API GATEWAY — server.js
 *
 *  Spawns the C++ fraud engine ONCE as a persistent child process
 *  and communicates via stdin/stdout piping.  The engine's
 *  unordered_map stays alive across requests, enabling stateful
 *  rules like the velocity check.
 *
 *  Flow:
 *    Client → POST /api/evaluate (JSON body)
 *           → Node writes "txId userId amount ip device ts\n" to engine.stdin
 *           → C++ evaluates, writes JSON line to stdout
 *           → Node reads the line, sends HTTP response
 * ============================================================
 */

const express = require('express');
const { spawn } = require('child_process');
const path = require('path');
const readline = require('readline');

const app = express();
const PORT = 3000;

// ── Path to the compiled C++ binary ────────────────────────────
const ENGINE_PATH = path.resolve(__dirname, '..', 'backend', 'sentinel');

// ── Request queue ──────────────────────────────────────────────
// Each incoming HTTP request pushes a callback onto this queue.
// When the engine writes a line to stdout, we shift the oldest
// callback and resolve it with the engine's response.
let requestQueue = [];

// ── Spawn the persistent C++ engine ────────────────────────────
let engine = null;
let engineReader = null;

function spawnEngine() {
    console.log('  [ENGINE] Spawning:', ENGINE_PATH);

    engine = spawn(ENGINE_PATH, [], {
        stdio: ['pipe', 'pipe', 'pipe'],
    });

    // Read engine stdout line-by-line
    engineReader = readline.createInterface({ input: engine.stdout });

    engineReader.on('line', (line) => {
        // Each line from stdout is a JSON response for the oldest queued request
        if (requestQueue.length > 0) {
            const callback = requestQueue.shift();
            callback(null, line);
        }
    });

    // Handle engine stderr (for debugging)
    engine.stderr.on('data', (data) => {
        console.error('  [ENGINE STDERR]', data.toString().trim());
    });

    // Handle engine crash — reject all pending requests and restart
    engine.on('close', (code) => {
        console.error(`  [ENGINE] Process exited with code ${code}`);
        // Reject all pending requests
        while (requestQueue.length > 0) {
            const callback = requestQueue.shift();
            callback(new Error('Engine crashed'), null);
        }
        // Auto-restart after a short delay
        setTimeout(() => {
            console.log('  [ENGINE] Restarting...');
            spawnEngine();
        }, 500);
    });

    console.log('  [ENGINE] Ready (PID:', engine.pid + ')');
}

// Boot the engine
spawnEngine();

// ── Middleware ──────────────────────────────────────────────────
app.use(express.json());

// CORS for frontend
app.use((req, res, next) => {
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Headers', 'Content-Type');
    res.header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    if (req.method === 'OPTIONS') return res.sendStatus(200);
    next();
});

// ── GET /api/health ────────────────────────────────────────────
app.get('/api/health', (req, res) => {
    res.json({
        status: 'online',
        engine: 'sentinel (persistent daemon)',
        enginePid: engine ? engine.pid : null,
        version: '0.7',
        uptime: process.uptime(),
    });
});

// ── POST /api/evaluate ─────────────────────────────────────────
app.post('/api/evaluate', (req, res) => {
    const { txId, userId, amount, ip, device, timestamp } = req.body;

    // Input validation
    const missing = [];
    if (!txId) missing.push('txId');
    if (!userId) missing.push('userId');
    if (amount === undefined || amount === null) missing.push('amount');
    if (!ip) missing.push('ip');
    if (!device) missing.push('device');
    if (timestamp === undefined || timestamp === null) missing.push('timestamp');

    if (missing.length > 0) {
        return res.status(400).json({ error: 'Missing required fields', missing });
    }

    // Check engine is alive
    if (!engine || engine.killed) {
        return res.status(503).json({ error: 'Engine is not running' });
    }

    // Build the space-delimited input line for the C++ engine
    const inputLine = `${txId} ${userId} ${amount} ${ip} ${device} ${timestamp}\n`;

    // Queue a callback that will be resolved when the engine responds
    requestQueue.push((err, outputLine) => {
        if (err) {
            return res.status(500).json({ error: 'Engine error', details: err.message });
        }

        try {
            const result = JSON.parse(outputLine);
            res.json({
                ...result,
                transaction: { txId, userId, amount, ip, device, timestamp },
            });
        } catch (parseErr) {
            res.status(500).json({ error: 'Failed to parse engine response', raw: outputLine });
        }
    });

    // Write to the engine's stdin
    engine.stdin.write(inputLine);
});

// ── Start the server ───────────────────────────────────────────
app.listen(PORT, () => {
    console.log('');
    console.log('  ╔══════════════════════════════════════════╗');
    console.log('  ║   SENTINEL API GATEWAY                   ║');
    console.log('  ║   ────────────────────────────────────    ║');
    console.log(`  ║   Listening on http://localhost:${PORT}     ║`);
    console.log('  ║   Engine mode: PERSISTENT DAEMON         ║');
    console.log('  ║                                          ║');
    console.log('  ║   POST /api/evaluate   → Evaluate txn    ║');
    console.log('  ║   GET  /api/health     → Health check    ║');
    console.log('  ╚══════════════════════════════════════════╝');
    console.log('');
});
