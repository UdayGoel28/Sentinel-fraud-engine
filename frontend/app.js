/**
 * ============================================================
 *  FRAUD DETECTION COMMAND CENTER — app.js (Phase 7)
 *  Attack Simulator + Live API Integration
 * ============================================================
 */

// ── Configuration ──────────────────────────────────────────────
const API_URL = 'http://localhost:3000/api/evaluate';

// ── DOM References ─────────────────────────────────────────────
const alertsBody = document.getElementById('alerts-body');
const alertCount = document.getElementById('alert-count');
const headerTime = document.getElementById('header-time');
const tableWrapper = document.getElementById('table-wrapper');
const emptyState = document.getElementById('empty-state');
const redFlash = document.getElementById('red-flash');

const btnNormal = document.getElementById('btn-normal');
const btnBotnet = document.getElementById('btn-botnet');
const btnClear = document.getElementById('btn-clear');

// Stat elements
const statSafe = document.getElementById('stat-safe');
const statTotal = document.getElementById('stat-total');
const statVelocity = document.getElementById('stat-velocity');
const statDevice = document.getElementById('stat-device');
const statIp = document.getElementById('stat-ip');

// ── State ──────────────────────────────────────────────────────
let eventCount = 0;
let safeCount = 0;
let blockedCount = 0;
let velocityCount = 0;
let deviceCount = 0;
let ipCount = 0;
let txCounter = 1000;  // auto-incrementing transaction ID

// ── Live Clock ─────────────────────────────────────────────────
function updateClock() {
    const now = new Date();
    const h = String(now.getHours()).padStart(2, '0');
    const m = String(now.getMinutes()).padStart(2, '0');
    const s = String(now.getSeconds()).padStart(2, '0');
    headerTime.textContent = `${h}:${m}:${s} LOCAL`;
}
setInterval(updateClock, 1000);
updateClock();

// ── Random Data Generators ─────────────────────────────────────
const USERS = ['user_alpha', 'user_beta', 'user_gamma', 'user_delta', 'user_echo', 'user_foxtrot'];
const IPS = ['192.168.1.10', '10.0.0.5', '172.16.0.1', '98.76.54.32', '203.45.67.89'];
const DEVICES = ['DEV_A1', 'DEV_B1', 'DEV_G1', 'DEV_D1', 'DEV_E1', 'DEV_F1'];

function randomFrom(arr) {
    return arr[Math.floor(Math.random() * arr.length)];
}

function generateNormalTransaction() {
    txCounter++;
    return {
        txId: `TXN${txCounter}`,
        userId: randomFrom(USERS),
        amount: Math.round((Math.random() * 500 + 5) * 100) / 100,
        ip: randomFrom(IPS),
        device: randomFrom(DEVICES),
        timestamp: Math.floor(Date.now() / 1000),
    };
}

// ── API Call ───────────────────────────────────────────────────
async function sendTransaction(txData) {
    const response = await fetch(API_URL, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(txData),
    });

    if (!response.ok) {
        throw new Error(`API returned ${response.status}`);
    }

    return response.json();
}

// ── Classify Rule ──────────────────────────────────────────────
function classifyResult(result) {
    if (result.status === 'safe') {
        return { cssClass: 'approved', label: 'APPROVED', badgeClass: 'safe', badgeLabel: 'PASSED' };
    }
    if (result.reason.includes('Velocity')) {
        return { cssClass: 'velocity', label: 'VELOCITY', badgeClass: 'blocked', badgeLabel: 'BLOCKED' };
    }
    if (result.reason.includes('Device')) {
        return { cssClass: 'device', label: 'DEVICE', badgeClass: 'blocked', badgeLabel: 'BLOCKED' };
    }
    if (result.reason.includes('IP')) {
        return { cssClass: 'ip-hop', label: 'IP HOP', badgeClass: 'blocked', badgeLabel: 'BLOCKED' };
    }
    return { cssClass: '', label: 'BLOCKED', badgeClass: 'blocked', badgeLabel: 'BLOCKED' };
}

// ── Render Result Row ──────────────────────────────────────────
function addResultToFeed(result) {
    // Hide empty state, show table
    emptyState.classList.add('hidden');
    tableWrapper.classList.remove('hidden');

    const info = classifyResult(result);
    const txn = result.transaction || {};
    const tr = document.createElement('tr');

    tr.className = result.status === 'safe' ? 'row-safe' : 'row-blocked';

    const now = new Date().toLocaleTimeString('en-US', { hour12: false });

    tr.innerHTML = `
    <td>
      <span class="status-badge ${info.badgeClass}">
        <span class="badge-dot"></span>
        ${info.badgeLabel}
      </span>
    </td>
    <td><span class="txn-id">${txn.txId || '—'}</span></td>
    <td><span class="user-id">${txn.userId || '—'}</span></td>
    <td><span class="rule-tag ${info.cssClass}">${info.label}</span></td>
    <td><span class="ts-value">${now}</span></td>
  `;

    // Prepend (newest at top)
    alertsBody.insertBefore(tr, alertsBody.firstChild);

    // Limit to 50 rows to avoid DOM bloat
    while (alertsBody.children.length > 50) {
        alertsBody.removeChild(alertsBody.lastChild);
    }

    // Update counters
    eventCount++;
    if (result.status === 'safe') {
        safeCount++;
    } else {
        blockedCount++;
        if (result.reason.includes('Velocity')) velocityCount++;
        else if (result.reason.includes('Device')) deviceCount++;
        else if (result.reason.includes('IP')) ipCount++;
    }

    updateStats();
}

// ── Update Stats Display ───────────────────────────────────────
function updateStats() {
    statSafe.textContent = safeCount;
    statTotal.textContent = blockedCount;
    statVelocity.textContent = velocityCount;
    statDevice.textContent = deviceCount;
    statIp.textContent = ipCount;
    alertCount.textContent = `${eventCount} EVENT${eventCount !== 1 ? 'S' : ''}`;
}

// ── Trigger Red Flash ──────────────────────────────────────────
function triggerRedFlash() {
    redFlash.classList.remove('active');
    // Force reflow to restart animation
    void redFlash.offsetWidth;
    redFlash.classList.add('active');
}

// ── Button Handlers ────────────────────────────────────────────

// NORMAL TRANSACTION — single random safe transaction
btnNormal.addEventListener('click', async () => {
    btnNormal.disabled = true;
    const txn = generateNormalTransaction();

    try {
        const result = await sendTransaction(txn);
        addResultToFeed(result);

        if (result.status === 'blocked') {
            triggerRedFlash();
        }
    } catch (err) {
        console.error('[ERROR] Normal transaction failed:', err);
        addResultToFeed({
            status: 'error',
            reason: err.message,
            transaction: txn,
        });
    }

    btnNormal.disabled = false;
});

// BOTNET ATTACK — 5 identical transactions in rapid succession
btnBotnet.addEventListener('click', async () => {
    btnBotnet.disabled = true;
    btnNormal.disabled = true;

    // Generate ONE identity — same user, same IP, same device
    const attackUser = 'user_botnet';
    const attackIp = '6.6.6.6';
    const attackDevice = 'DEV_BOT';

    // Fire 5 requests as fast as possible
    for (let i = 0; i < 5; i++) {
        txCounter++;
        const txn = {
            txId: `ATK${txCounter}`,
            userId: attackUser,
            amount: Math.round((Math.random() * 100 + 1) * 100) / 100,
            ip: attackIp,
            device: attackDevice,
            timestamp: Math.floor(Date.now() / 1000),
        };

        try {
            const result = await sendTransaction(txn);
            addResultToFeed(result);

            if (result.status === 'blocked') {
                triggerRedFlash();
            }
        } catch (err) {
            console.error('[ERROR] Botnet request failed:', err);
        }

        // Tiny delay between requests for visual stagger (100ms)
        await new Promise(r => setTimeout(r, 100));
    }

    btnBotnet.disabled = false;
    btnNormal.disabled = false;
});

// CLEAR FEED
btnClear.addEventListener('click', () => {
    alertsBody.innerHTML = '';
    eventCount = safeCount = blockedCount = velocityCount = deviceCount = ipCount = 0;
    updateStats();
    tableWrapper.classList.add('hidden');
    emptyState.classList.remove('hidden');
});

// ── Boot ───────────────────────────────────────────────────────
document.addEventListener('DOMContentLoaded', () => {
    tableWrapper.classList.add('hidden');
    emptyState.classList.remove('hidden');
    updateStats();
});
