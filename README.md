# 2-Bit ALU Verification System: Technical Documentation

## Overview

Hardware-in-the-loop verification framework for a custom 2-bit ALU implementing UVM-inspired methodology. System architecture spans three tiers: Python testbench (stimulus generation, scoreboard), Arduino hardware bridge (signal encoding/decoding, 5kHz clock generation), and web-based real-time monitoring interface. Achieves 100% functional coverage across all 2-bit input combinations with ~150ms verification cycles (configurable to 500ms for demonstration).

**ALU Operations:** NAND, NOT-A, OR, AND, XOR, SUM (binary addition), COUT (carry-out)

**Architecture:**
```
Web Interface (WebSocket/JSON) → Python Server (Serial/USB) → Arduino Bridge (TTL I/O) → Physical ALU
```

---

## ALU Specification

| A | B | NAND | NOT-A | OR | AND | XOR | SUM | COUT |
|---|---|------|-------|-------|----|-----|-----|-----|------|
| 0 | 0 |  1   |   1   | 0  |  0  |  0  |  0  |  0   |
| 0 | 1 |  1   |   1   | 1  |  0  |  1  |  1  |  0   |
| 1 | 0 |  1   |   0   | 1  |  0  |  1  |  1  |  0   |
| 1 | 1 |  0   |   0   | 1  |  1  |  0  |  0  |  1   |

**Clock:** 5kHz continuous (200μs period, 50% duty cycle)

---

## Hardware Interface

### Pin Mapping (Arduino Uno)

**Inputs (Arduino → ALU):**
- A: Pin 3 (Yellow1)
- B: Pin 4 (Yellow2)
- CLK: Pin 13 (5kHz generated)

**Outputs (ALU → Arduino):**
- NAND: Pin 8 (Yellow) | NOT-A: Pin 9 (Green) | OR: Pin 10 (Blue)
- COUT: Pin 5 (Pink) | SUM: Pin 6 (Grey) | AND: Pin 11 (White) | XOR: Pin 12 (Brown)

All output pins configured with INPUT_PULLUP. Power: +5V on Pin 7.

---

## Serial Communication Protocol

**Physical:** USB Serial, 115200 baud, 8N1

### Commands (PC → Arduino)

- `Mab` - Manual mode: Set A=a, B=b (e.g., `M01` → A=0, B=1)
- `R` - Enable random mode
- `S` - Stop random mode

### Data Response (Arduino → PC)

Format: `[<A>,<B>|<OUTPUTS>]`

Output order: NAND, NOT-A, OR, COUT, SUM, AND, XOR

Example: `[0,1|1110101]` → A=0, B=1, outputs = 1,1,1,0,1,0,1

---

## Implementation Details

### Python Verification Server

**Golden Reference Model:**
```python
EXPECTED_OUTPUTS = {
    (0, 0): (1, 1, 0, 0, 0, 0, 0),  # NAND, NOT_A, OR, COUT, SUM, AND, XOR
    (0, 1): (1, 1, 1, 0, 1, 0, 1),
    (1, 0): (1, 0, 1, 0, 1, 0, 1),
    (1, 1): (0, 0, 1, 1, 0, 1, 0)
}
```

**Verification Status:** `OK` (all match), `MIXED` (partial), `ERROR` (all fail)

**Stimulus Modes:**
- Sequential: Cycles (0,0)→(0,1)→(1,0)→(1,1)→repeat
- Random: Pseudorandom A/B generation

**Asynchronous Architecture:**
- ALU control loop: 500ms intervals (configurable)
- WebSocket server: Port 8765, broadcasts state to clients
- Non-blocking I/O via `asyncio`

### Arduino Firmware

**Clock Generation:**
- Continuous 5kHz on Pin 13 via `delayMicroseconds(100)` loop
- ~200μs per iteration, independent of data cycles

**Command Parsing:**
- Character accumulation until newline
- State machine: classify (M/R/S), extract params, validate, execute
- Buffer limit: 10 characters

**I/O Timing:**
- Input application: Synchronous via `digitalWrite()`
- Output sampling: Atomic read of all 7 outputs sequentially
- Stabilization delay: 1ms after input change in random mode

### Web Interface

**WebSocket:** Persistent connection to `ws://127.0.0.1:8765`, JSON state broadcast

**State Object:**
```json
{
    "A": 0, "B": 1,
    "NAND": 1, "NOT_A": 1, "OR": 1, "COUT": 0,
    "SUM": 1, "AND": 0, "XOR": 1,
    "VerificationStatus": "OK",
    "VerificationDetails": {<per-operation results>},
    "Timestamp": 1700864412.345,
    "Mode": "AUTO"
}
```

**Visualization:**
- Live ALU block diagram with current inputs/outputs
- 4-row truth table with color-coded verification (green=match, red=mismatch)
- Real-time mismatch reporting
- Manual/Random mode control

---

## Verification Methodology

**Coverage:** 100% functional (4 input vectors × 7 operations = 28 assertions)

**Scoreboard Pattern:**
1. Reference model (golden truth table)
2. Actual capture (Arduino sampling)
3. Per-operation comparison
4. Status aggregation (OK/MIXED/ERROR)
5. Real-time visualization

**Verification Flow:**
```
Stimulus Generation → Serial Command → Input Application → ALU Computation →
Output Sampling → Packet Transmission → Parsing → Verification → 
State Update → WebSocket Broadcast → UI Update → Loop
```

**UVM Mapping:**
| UVM Component | Implementation |
|---------------|----------------|
| DUT | Physical ALU Circuit |
| Driver | Arduino `applyInputs()` |
| Monitor | Arduino `sendDataToPython()` |
| Agent | Arduino Firmware |
| Sequencer | Python `advance_inputs()` / Random Generator |
| Scoreboard | Python `verify_outputs()` + Web Interface |
| Environment | Python `alu_loop()` + WebSocket Server |

---

## Performance

**Cycle Timing:**
- Production: ~150ms minimum (6.67 cycles/sec)
- Demonstration: 500ms default (2 cycles/sec)

**End-to-End Latency:** ~5-10ms (input application → UI update, excluding cycle delay)

**Throughput:**
- 500ms mode: 8 vectors/sec (2 complete truth table passes/sec)
- 150ms mode: 26.67 vectors/sec (6.67 passes/sec)

**Coverage Rate:**
- Complete truth table: 2 seconds (500ms) or 0.6 seconds (150ms)

---

## Operational Procedures

**Startup:**
1. Connect Arduino via USB, power ALU circuit, verify pin connections
2. Upload `alu_veriification.ino`, confirm "STATUS: Ready" at 115200 baud
3. Set `ARDUINO_PORT` in `alu_server.py`, run server, verify WebSocket on port 8765
4. Open `index.html`, confirm "CONNECTED" status

**Modes:**
- Sequential: Automatic cycling through all input combinations
- Manual: Direct A/B input control via web interface
- Random: Continuous pseudorandom stimulus generation

**Error Handling:**
- Parsing errors: Invalid packets discarded, status="ERROR"
- Functional errors: Recorded in VerificationDetails, status="MIXED"/"ERROR"
- Communication errors: Serial timeout logged, WebSocket auto-reconnect (3s)

---

## Configuration

| Parameter | Location | Default | Description |
|-----------|----------|---------|-------------|
| `ARDUINO_PORT` | `alu_server.py` | `'COM5'` | Serial port |
| `BAUD_RATE` | `alu_server.py` | `115200` | Serial speed |
| `WEB_SOCKET_PORT` | `alu_server.py` | `8765` | WebSocket port |
| `VERIFICATION_INTERVAL_SEC` | `alu_server.py` | `0.5` | Cycle time |
| `HALF_PERIOD_US` | `alu_veriification.ino` | `100` | Clock half-period |

---

## Dependencies

**Python:** `pyserial`, `websockets`, `asyncio`  
**Arduino:** Standard Arduino IDE libraries  
**Web:** Modern browser with WebSocket support, Tailwind CSS (CDN)

---

*Document Version: 1.0 | November 2024*
