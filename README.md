# 🛡️ Sentinel: Sub-Millisecond Fraud Detection Engine

Sentinel is a real-time, high-performance fraud detection microservice built for high-risk digital transactions. It uses a custom-compiled C++ risk engine wrapped in a Node.js API gateway to detect botnet attacks, velocity anomalies, and identity theft in milliseconds.

---

## 📖 How It Works (For Business)

**The Problem:** Modern financial fraud is automated. Hackers use botnets to test thousands of stolen credit cards or API endpoints per minute. Traditional security software built on standard web languages often lags or crashes under this rapid-fire assault.

**The Solution:** Sentinel splits the workload. It uses a lightweight Node.js web server to handle the incoming internet traffic, but hands the actual fraud analysis off to a custom-built C++ persistent daemon. Because C++ talks directly to the hardware, it calculates risk rules and blocks rapid-fire bot attacks in sub-milliseconds—long before the transaction reaches the payment gateway.

---

## ⚙️ Architecture (For Engineers)

Sentinel uses a high-performance **Open-Core Microservice Architecture** to ensure zero-latency risk scoring.

### The Data Flow
Instead of spinning up a new C++ instance for every request (which wipes memory and kills speed), the C++ engine is spawned once by Node.js and runs as a **persistent daemon**.

```text
+-------------------------+
| Browser (Control Panel) |
+-------------------------+
      |        ^ 
      | POST   | HTTP JSON
      v        |
+-------------------------+
| Node.js API (:3000)     |
+-------------------------+
      |        ^ 
      | stdin  | stdout JSON
      v        |
+-------------------------+
| C++ Daemon (Persistent) |
+-------------------------+