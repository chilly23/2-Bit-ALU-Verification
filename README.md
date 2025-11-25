# 2-Bit ALU Verification System: Technical Documentation

## Executive Summary

This document describes a complete hardware-in-the-loop verification framework for a custom 2-bit Arithmetic Logic Unit (ALU) design. The system implements a UVM-inspired verification methodology, utilizing a Python-based testbench, Arduino hardware bridge, and real-time web-based monitoring interface. The verification framework achieves deterministic coverage across all 2-bit input combinations with configurable stimulus generation modes and sub-200ms verification cycles in production configurations.

The ALU design under verification implements seven distinct operations: NAND, NOT-A, OR, AND, XOR, binary addition (SUM), and carry-out (COUT). The verification infrastructure provides real-time assertion checking, automated scoreboarding, and comprehensive mismatch reporting through a multi-layer architecture spanning embedded firmware, system-level Python orchestration, and web-based visualization.

---

## System Architecture

### High-Level Architecture

The verification system follows a three-tier architecture pattern:

```
┌─────────────────────────────────────────────────────────────┐
│                    Web Monitoring Interface                   │
│              (HTML/CSS/JavaScript + WebSocket)                │
└─────────────────────┬───────────────────────────────────────┘
                      │ WebSocket (TCP/8765)
                      │ JSON Protocol
┌─────────────────────▼───────────────────────────────────────┐
│              Python Verification Server                       │
│    (Stimulus Generation + Scoreboard + Protocol Handler)     │
└─────────────────────┬───────────────────────────────────────┘
                      │ Serial/USB (115200 baud)
                      │ Command/Response Protocol
┌─────────────────────▼───────────────────────────────────────┐
│              Arduino Hardware Bridge                          │
│        (Signal Encoding/Decoding + Clock Generation)          │
└─────────────────────┬───────────────────────────────────────┘
                      │ Digital I/O (TTL Levels)
                      │ Clock: 5kHz on Pin 13
┌─────────────────────▼───────────────────────────────────────┐
│                    Physical ALU Design                        │
│         (Breadboard Implementation with ICs + LEDs)           │
└─────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

**Design Under Test (DUT):** Physical 2-bit ALU circuit implemented on breadboard using discrete logic ICs. Accepts two single-bit inputs (A, B) and a 5kHz clock signal. Produces seven synchronous outputs representing the various arithmetic and logical operations.

**Arduino Hardware Bridge:** Serves as a protocol translation layer and signal conditioning interface. Responsible for:
- Encoding high-level commands from Python into TTL-level digital signals
- Decoding ALU output signals and formatting them for transmission
- Generating a continuous 5kHz clock signal for ALU synchronization
- Managing bidirectional serial communication at 115200 baud

**Python Verification Server:** Core testbench engine implementing:
- Stimulus generation (sequential and random modes)
- Real-time output verification against golden reference model
- Scoreboard maintenance and mismatch tracking
- WebSocket server for real-time state distribution
- Asynchronous event loop management for non-blocking I/O

**Web Monitoring Interface:** Frontend visualization and control layer providing:
- Real-time ALU state visualization
- Interactive input control for manual verification
- Color-coded truth table with per-operation verification status
- Detailed mismatch reporting
- Mode switching between deterministic and randomized test sequences

---

## ALU Design Specification

### Functional Operations

The ALU implements the following operations concurrently (all outputs computed in parallel):

| Operation | Description | Truth Table Behavior |
|-----------|-------------|---------------------|
| NAND      | Logical NAND of A and B | `!(A && B)` |
| NOT-A     | Logical negation of input A | `!A` |
| OR        | Logical OR of A and B | `A \|\| B` |
| AND       | Logical AND of A and B | `A && B` |
| XOR       | Exclusive OR of A and B | `A ^ B` |
| SUM       | Binary sum of A and B | `(A + B) mod 2` |
| COUT      | Carry-out from binary addition | `(A + B) / 2` |

### Complete Truth Table

| A | B | NAND | NOT-A | OR | AND | XOR | SUM | COUT |
|---|---|------|-------|-------|----|-----|-----|-----|------|
| 0 | 0 |  1   |   1   | 0  |  0  |  0  |  0  |  0   |
| 0 | 1 |  1   |   1   | 1  |  0  |  1  |  1  |  0   |
| 1 | 0 |  1   |   0   | 1  |  0  |  1  |  1  |  0   |
| 1 | 1 |  0   |   0   | 1  |  1  |  0  |  0  |  1   |

### Clock Requirements

The ALU requires a continuous 5kHz clock signal for proper operation. This corresponds to:
- Period: 200μs
- High time: 100μs
- Low time: 100μs
- Duty cycle: 50%

---

## Hardware Interface Specification

### Pin Mapping (Arduino Uno)

#### Input Control Signals (Arduino Output → ALU Input)

| Signal | Arduino Pin | Physical Wire | Description |
|--------|-------------|---------------|-------------|
| A      | Digital 3   | Yellow1       | First operand input |
| B      | Digital 4   | Yellow2       | Second operand input |
| CLK    | Digital 13  | (Generated)   | 5kHz clock signal |

#### Output Readback Signals (ALU Output → Arduino Input)

| Signal | Arduino Pin | Physical Wire | Pull-up Configuration |
|--------|-------------|---------------|----------------------|
| NAND   | Digital 8   | Yellow        | INPUT_PULLUP |
| NOT-A  | Digital 9   | Green         | INPUT_PULLUP |
| OR     | Digital 10  | Blue          | INPUT_PULLUP |
| COUT   | Digital 5   | Pink          | INPUT_PULLUP |
| SUM    | Digital 6   | Grey          | INPUT_PULLUP |
| AND    | Digital 11  | White         | INPUT_PULLUP |
| XOR    | Digital 12  | Brown         | INPUT_PULLUP |

**Note:** All output pins are configured with internal pull-up resistors to prevent floating inputs when the ALU outputs are in high-impedance states or during transient conditions. This ensures deterministic behavior and prevents false readings from electrical noise.

### Power Distribution

- Active +5V supply: Arduino Pin 7 (Black wire)
- Ground: Common ground connection across all components

---

## Serial Communication Protocol

### Physical Layer

- **Interface:** USB Serial (FTDI-compatible)
- **Baud Rate:** 115200
- **Data Bits:** 8
- **Stop Bits:** 1
- **Parity:** None
- **Flow Control:** None

### Command Protocol (PC → Arduino)

Commands are ASCII strings terminated by newline (`\n`) or carriage return (`\r`) characters.

#### Manual Mode Command: `Mab`

Sets ALU inputs to specific values in manual mode.

**Format:** `M<digit><digit>`

- First character: `M` (command identifier)
- Second character: `0` or `1` (value for input A)
- Third character: `0` or `1` (value for input B)

**Examples:**
- `M00` → Set A=0, B=0
- `M01` → Set A=0, B=1
- `M10` → Set A=1, B=0
- `M11` → Set A=1, B=1

**Response:** Arduino responds with status message:
```
STATUS: Manual Input Set: A=<a>, B=<b>
```

#### Random Mode Command: `R`

Enables random input generation mode. Arduino will generate random values for A and B on each cycle.

**Format:** `R`

**Response:**
```
STATUS: Random Mode ON
```

#### Stop/Manual Command: `S`

Disables random mode without setting specific inputs (maintains current state).

**Format:** `S`

**Response:**
```
STATUS: Manual Mode ON (No random updates)
```

### Data Response Protocol (Arduino → PC)

Arduino continuously sends formatted data packets containing current input state and all ALU outputs.

**Format:** `[<A>,<B>|<OUTPUT_STRING>]`

Where:
- `<A>` is the current value of input A (0 or 1)
- `<B>` is the current value of input B (0 or 1)
- `<OUTPUT_STRING>` is a 7-character string representing outputs in fixed order

**Output String Order:**
1. Character 0: NAND output
2. Character 1: NOT-A output
3. Character 2: OR output
4. Character 3: COUT output
5. Character 4: SUM output
6. Character 5: AND output
7. Character 6: XOR output

**Example:**
```
[0,1|1110101]
```

This indicates:
- A=0, B=1
- NAND=1, NOT-A=1, OR=1, COUT=0, SUM=1, AND=0, XOR=1

**Parsing Logic:** The Python server filters for lines that start with `[` and end with `]`, then extracts the pipe-delimited input and output sections.

---

## Python Verification Server Implementation

### Core Data Structures

#### Expected Outputs Dictionary

```python
EXPECTED_OUTPUTS = {
    (0, 0): (1, 1, 0, 0, 0, 0, 0),  # NAND, NOT_A, OR, COUT, SUM, AND, XOR
    (0, 1): (1, 1, 1, 0, 1, 0, 1),
    (1, 0): (1, 0, 1, 0, 1, 0, 1),
    (1, 1): (0, 0, 1, 1, 0, 1, 0)
}
```

This dictionary serves as the golden reference model. The tuple ordering matches the Arduino output string format exactly.

#### Output Mapping

```python
OUTPUT_MAP = {
    0: "NAND", 1: "NOT_A", 2: "OR", 3: "COUT",
    4: "SUM", 5: "AND", 6: "XOR"
}
```

Provides human-readable names for each output position in the verification results.

### Verification Algorithm

The `verify_outputs()` function implements the core checking logic:

```python
def verify_outputs(a_in, b_in, actual_outputs_str):
    key = (a_in, b_in)
    expected = EXPECTED_OUTPUTS.get(key)
    
    # Parse actual outputs from string format
    actual = tuple(int(c) for c in actual_outputs_str)
    
    # Per-operation comparison
    verification = {}
    mismatches = 0
    
    for i in range(7):
        name = OUTPUT_MAP[i]
        act = actual[i]
        exp = expected[i]
        match = (act == exp)
        
        verification[name] = {
            "Actual": act,
            "Expected": exp,
            "Match": match
        }
        
        if not match:
            mismatches += 1
    
    # Overall status determination
    if mismatches == 0:
        return "OK", verification
    elif mismatches == 7:
        return "ERROR", verification
    else:
        return "MIXED", verification
```

**Status Semantics:**
- `OK`: All seven outputs match expected values
- `MIXED`: Partial match (1-6 outputs incorrect)
- `ERROR`: Complete mismatch or parsing failure

### Stimulus Generation

#### Sequential Mode (Default)

The `advance_inputs()` function implements a deterministic state machine that cycles through all four input combinations:

```
(0,0) → (0,1) → (1,0) → (1,1) → (0,0) ...
```

This ensures complete coverage of the 2-bit input space in a predictable sequence.

#### Random Mode

When enabled, random mode generates inputs using Arduino's `random(2)` function, producing pseudorandom 0 or 1 values for each input on every cycle. The Python server maintains synchronization by sending `R` commands periodically.

### Asynchronous Event Loop

The server uses Python's `asyncio` framework to manage concurrent operations:

1. **ALU Control Loop (`alu_loop`)**: Runs at 500ms intervals (configurable via `VERIFICATION_INTERVAL_SEC`)
   - Sends current input command to Arduino
   - Reads and processes incoming data
   - Broadcasts state updates to WebSocket clients
   - Advances input sequence

2. **WebSocket Server**: Handles client connections on port 8765
   - Accepts connections from web interface
   - Broadcasts ALU state updates to all connected clients
   - Maintains connection set for efficient broadcasting

The two tasks run concurrently without blocking, ensuring responsive operation even during serial I/O delays.

### State Management

The `ALU_STATE` dictionary maintains the complete system state:

```python
ALU_STATE = {
    "A": <current input A>,
    "B": <current input B>,
    "NAND": <current output>,
    "NOT_A": <current output>,
    "OR": <current output>,
    "COUT": <current output>,
    "SUM": <current output>,
    "AND": <current output>,
    "XOR": <current output>,
    "VerificationStatus": <"OK"|"MIXED"|"ERROR">,
    "VerificationDetails": {<per-operation results>},
    "Timestamp": <Unix timestamp>,
    "Mode": <"AUTO"|"MANUAL"|"RANDOM">
}
```

This state is atomically updated on each verification cycle and broadcast to all WebSocket clients.

---

## Arduino Firmware Implementation

### Clock Generation

The firmware generates a continuous 5kHz clock signal on pin 13 using a simple delay-based loop:

```cpp
void loop() {
    digitalWrite(CLOCK_PIN, HIGH);
    delayMicroseconds(HALF_PERIOD_US);  // 100μs
    digitalWrite(CLOCK_PIN, LOW);
    delayMicroseconds(HALF_PERIOD_US);  // 100μs
    // ... rest of loop logic
}
```

**Timing Analysis:**
- Clock generation occupies ~200μs per loop iteration
- Remaining loop operations (serial handling, I/O reading) add minimal overhead
- Actual clock frequency may vary slightly due to interrupt servicing, but maintains approximately 5kHz

**Note:** The clock signal runs continuously independent of data transmission cycles. This ensures the ALU always has a valid clock reference.

### Command Parsing

The `checkSerialCommands()` function implements a state machine for parsing incoming serial commands:

1. **Character Accumulation:** Builds command string character-by-character until newline detected
2. **Command Classification:** Identifies command type by first character (`M`, `R`, or `S`)
3. **Parameter Extraction:** For `M` commands, extracts A and B values from positions 1 and 2
4. **Validation:** Ensures extracted values are valid (0 or 1)
5. **Execution:** Applies inputs immediately or sets mode flags

**Buffer Management:** Command buffer limited to 10 characters to prevent memory exhaustion from malformed input.

### Input Application

The `applyInputs()` function directly drives the ALU input pins:

```cpp
void applyInputs() {
    digitalWrite(A_INPUT_PIN, current_a);
    digitalWrite(B_INPUT_PIN, current_b);
}
```

**Timing Considerations:** Input changes are applied synchronously. The ALU's internal logic should handle setup/hold times relative to the clock signal. A 1ms delay is introduced after input application in random mode to allow signal stabilization before reading outputs.

### Output Sampling

The `sendDataToPython()` function samples all seven ALU outputs in a single atomic read operation:

```cpp
int out_nand = digitalRead(P_NAND);
int out_not_a = digitalRead(P_NOT_A);
int out_or = digitalRead(P_OR);
int out_cout = digitalRead(P_CARRY_OUT);
int out_sum = digitalRead(P_SUM);
int out_and = digitalRead(P_AND);
int out_xor = digitalRead(P_XOR);
```

**Sampling Strategy:** All outputs are read sequentially within microseconds of each other, ensuring consistent temporal alignment. This is critical for verifying that all operations are computed on the same input state.

### Serial Output Formatting

The formatted output string follows the protocol specification exactly:

```cpp
Serial.print("[");
Serial.print(current_a);
Serial.print(",");
Serial.print(current_b);
Serial.print("|");
Serial.print(out_nand);
Serial.print(out_not_a);
Serial.print(out_or);
Serial.print(out_cout);
Serial.print(out_sum);
Serial.print(out_and);
Serial.print(out_xor);
Serial.println("]");
```

This produces lines like `[0,1|1110101]` which are parsed by the Python server's regex-based filtering.

---

## Web Interface Implementation

### WebSocket Protocol

The web interface establishes a persistent WebSocket connection to `ws://127.0.0.1:8765`. The Python server broadcasts complete `ALU_STATE` JSON objects on every verification cycle.

**Message Format:**
```json
{
    "A": 0,
    "B": 1,
    "NAND": 1,
    "NOT_A": 1,
    "OR": 1,
    "COUT": 0,
    "SUM": 1,
    "AND": 0,
    "XOR": 1,
    "VerificationStatus": "OK",
    "VerificationDetails": {
        "NAND": {"Actual": 1, "Expected": 1, "Match": true},
        "NOT_A": {"Actual": 1, "Expected": 1, "Match": true},
        ...
    },
    "Timestamp": 1700864412.345,
    "Mode": "AUTO"
}
```

### Control Interface

The control panel supports two operational modes:

1. **Manual Mode:** User can directly set A and B inputs via button interface
   - Inputs are sent immediately via WebSocket message
   - Format: `{"mode": "M", "a": <value>, "b": <value>}`
   - Note: Manual control via web interface requires Python server to forward commands to Arduino (current implementation uses sequential mode by default)

2. **Random Mode:** Enables randomized stimulus generation
   - Sends `{"mode": "R"}` message
   - Python server switches to random mode and commands Arduino accordingly

**Connection Status Indicator:**
- Green "CONNECTED" when WebSocket is established
- Red "DISCONNECTED" when connection lost
- Automatic reconnection with 3-second retry interval

### Visualization Components

#### ALU Block Diagram

Central display showing:
- Current input values (A, B)
- Symbolic ALU block representation
- List of supported operations
- Last update timestamp

#### Live Output Display

Real-time output values displayed next to the ALU block:
- Green text for verified outputs
- Red text for mismatched outputs
- Updates on every WebSocket message

#### Truth Table

Complete 4-row truth table showing:
- Static expected values in column headers
- Dynamic actual values in cells
- Color-coded verification status:
  - **Bright green (#008000):** Match
  - **Bright red (#C91B00):** Mismatch
  - **Yellow background:** Current test vector (row highlight)
- Updates accumulate across cycles, building a complete verification history

#### Mismatch Report

Scrollable text area listing all detected mismatches in format:
```
Mismatch on <OPERATION>: A=<a>, B=<b> - Expected <exp>, Got <act>
```

Only mismatches from the current cycle are displayed (not historical accumulation).

### UI Framework

The interface uses Tailwind CSS for styling with a monochrome design aesthetic:
- Black borders (`mono-border`)
- White backgrounds (`mono-bg`)
- High-contrast button states
- Minimal color palette (green/red for verification status only)

---

## Verification Methodology

### Coverage Model

The verification framework achieves 100% functional coverage of the 2-bit input space:

- **Input Combinations:** 4 unique vectors (00, 01, 10, 11)
- **Output Coverage:** All 7 operations verified per vector
- **Total Assertions:** 28 individual output checks (4 vectors × 7 operations)

### Test Modes

#### Deterministic Sequential Mode

- **Purpose:** Reproducible verification with predictable sequence
- **Execution:** Cycles through all four input combinations in order
- **Use Case:** Regression testing, debugging, demonstration
- **Cycle Time:** Configurable (default 500ms, production ~150ms)

#### Randomized Mode

- **Purpose:** Stress testing with pseudorandom stimulus
- **Execution:** Generates random A/B values on each cycle
- **Use Case:** Extended reliability testing, corner case discovery
- **Statistical Coverage:** Over time, all input combinations will be tested multiple times

### Scoreboarding Strategy

The verification system implements a real-time scoreboard pattern:

1. **Reference Model:** Golden truth table stored in Python server
2. **Actual Capture:** ALU outputs sampled via Arduino
3. **Comparison:** Per-operation comparison on every cycle
4. **Status Aggregation:** Overall status derived from individual results
5. **Visualization:** Color-coded feedback in web interface

**Scoreboard States:**
- **PASS:** All outputs match expected (green)
- **PARTIAL:** Some outputs incorrect (amber/orange)
- **FAIL:** All outputs incorrect or parsing error (red)

### Verification Flow

```
1. Python Server: Generate stimulus (A, B values)
   ↓
2. Python → Arduino: Send command (e.g., "M01")
   ↓
3. Arduino: Apply inputs to ALU physical pins
   ↓
4. ALU: Compute outputs (synchronized to 5kHz clock)
   ↓
5. Arduino: Sample all outputs simultaneously
   ↓
6. Arduino → Python: Send formatted data packet
   ↓
7. Python: Parse packet, extract inputs and outputs
   ↓
8. Python: Compare outputs against golden reference
   ↓
9. Python: Update ALU_STATE with verification results
   ↓
10. Python → Web Interface: Broadcast state via WebSocket
   ↓
11. Web Interface: Update visualization, highlight mismatches
   ↓
12. Loop: Advance to next input combination (or generate random)
```

### Error Detection and Reporting

**Parsing Errors:**
- Invalid packet format → Verification status: "ERROR"
- Missing outputs → Packet discarded, logged
- Invalid input values → Command rejected by Arduino

**Functional Errors:**
- Output mismatch → Recorded in VerificationDetails
- Status set to "MIXED" or "ERROR" based on severity
- Detailed mismatch report generated for web interface

**Communication Errors:**
- Serial timeout → Logged, state marked as stale
- WebSocket disconnection → Automatic reconnection attempted
- Arduino not responding → Connection status updated in UI

---

## Performance Characteristics

### Verification Cycle Timing

**Production Configuration:**
- Minimum cycle time: ~150ms (6.67 cycles/second)
- Breakdown:
  - Serial command transmission: ~1-2ms
  - Arduino processing + ALU settling: ~10-20ms
  - Serial response transmission: ~1-2ms
  - Python parsing + verification: <1ms
  - Overhead (scheduling, I/O): ~130ms

**Demonstration Configuration:**
- Default cycle time: 500ms (2 cycles/second)
- Slowed for visibility and recording purposes

### Latency Analysis

**End-to-End Latency (Stimulus → Visualization):**
- Arduino input application: <1ms
- ALU computation: <1 clock cycle (200μs at 5kHz)
- Arduino output sampling: <1ms
- Serial transmission: ~2ms (115200 baud, ~20 bytes)
- Python processing: <1ms
- WebSocket broadcast: <1ms (local network)
- Browser rendering: <16ms (60 FPS target)

**Total:** Approximately 5-10ms from input application to UI update, excluding the configured cycle delay.

### Throughput

**Verification Rate:**
- With 500ms cycle: 8 vectors/second (2 complete truth table passes/second)
- With 150ms cycle: 26.67 vectors/second (6.67 complete truth table passes/second)

**Coverage Rate:**
- Complete truth table coverage: 2 seconds (500ms mode) or 0.6 seconds (150ms mode)

---

## UVM-Style Architecture Mapping

The verification framework can be conceptually mapped to UVM components:

| UVM Component | Implementation | Responsibility |
|---------------|----------------|----------------|
| **Design (DUT)** | Physical ALU Circuit | Device under test |
| **Driver** | Arduino `applyInputs()` | Drives inputs to DUT |
| **Monitor** | Arduino `sendDataToPython()` | Captures DUT outputs |
| **Agent** | Arduino Firmware | Encapsulates driver + monitor |
| **Sequencer** | Python `advance_inputs()` / Random Generator | Generates stimulus |
| **Scoreboard** | Python `verify_outputs()` + Web Interface | Compares actual vs expected |
| **Environment** | Python `alu_loop()` + WebSocket Server | Orchestrates all components |
| **Test** | Manual/Random mode selection | Configures test scenario |

### Verification Infrastructure

**Reference Model:** `EXPECTED_OUTPUTS` dictionary serves as the golden reference model, equivalent to a UVM predictor.

**Transaction-Level Modeling:** 
- Input transactions: (A, B) pairs
- Output transactions: 7-tuple of operation results
- Verification transactions: Comparison results with status

**Coverage Collection:**
- Implicit coverage: All 4 input combinations exercised
- Explicit coverage: Per-operation verification status tracked
- Coverage visualization: Truth table with color-coding

---

## Operational Procedures

### System Startup Sequence

1. **Hardware Setup:**
   - Connect Arduino to PC via USB
   - Ensure ALU circuit is powered and clocked
   - Verify all I/O connections per pin mapping

2. **Arduino Firmware:**
   - Upload `alu_veriification.ino` to Arduino
   - Open serial monitor to verify "STATUS: Ready" message
   - Confirm baud rate is 115200

3. **Python Server:**
   - Update `ARDUINO_PORT` in `alu_server.py` to match system COM port
   - Run: `python alu_server.py`
   - Verify serial connection message in logs
   - Confirm WebSocket server startup on port 8765

4. **Web Interface:**
   - Open `index.html` in web browser
   - Verify WebSocket connection (status should show "CONNECTED")
   - Observe initial ALU state display

### Normal Operation

**Sequential Verification Mode:**
- Server automatically cycles through all input combinations
- Web interface updates in real-time
- Truth table accumulates results across cycles
- Mismatches are highlighted immediately

**Manual Verification Mode:**
- Use web interface controls to set A/B inputs
- Observe outputs update after each change
- Truth table updates for current input combination

**Random Verification Mode:**
- Enable random mode via web interface
- System generates random inputs continuously
- Useful for extended stress testing

### Troubleshooting

**No Connection to Arduino:**
- Check COM port configuration
- Verify USB cable connection
- Confirm Arduino is recognized by OS
- Check for port conflicts (other serial programs)

**No Data Received:**
- Verify ALU circuit is powered
- Check I/O pin connections
- Confirm clock signal is present (pin 13)
- Review Arduino serial monitor for status messages

**Verification Failures:**
- Compare actual outputs to expected truth table
- Check for wiring errors in ALU circuit
- Verify IC functionality independently
- Review pull-up resistor configurations

**Web Interface Not Updating:**
- Check WebSocket connection status
- Verify Python server is running
- Check browser console for errors
- Confirm firewall isn't blocking port 8765

---

## Future Enhancements

### Potential Extensions

1. **Expanded Input Width:** Extend to 4-bit or 8-bit ALU with corresponding protocol changes
2. **Operation Coverage:** Add subtraction, multiplication, shift operations
3. **Advanced Stimulus:** Constrained random generation, weighted distributions
4. **Coverage Metrics:** Explicit coverage collection with UVM-style coverage groups
5. **Waveform Capture:** Export timing diagrams for debugging
6. **Regression Suite:** Automated test sequences with pass/fail criteria
7. **Performance Profiling:** Detailed timing analysis of ALU operations
8. **Multi-Device Support:** Parallel verification of multiple ALU instances

### Architectural Improvements

1. **Protocol Enhancements:** Binary protocol for higher throughput
2. **Error Correction:** CRC or checksum validation of serial packets
3. **State Machine Formalization:** Explicit state machine for command parsing
4. **Logging Infrastructure:** Structured logging with rotation and archival
5. **Remote Access:** Secure remote monitoring and control capabilities

---

## Conclusion

This verification framework demonstrates a complete hardware-in-the-loop verification solution for a 2-bit ALU design. By combining embedded firmware, system-level Python orchestration, and web-based visualization, the system achieves comprehensive functional verification with real-time feedback and intuitive debugging capabilities.

The architecture's modular design allows for straightforward extension to more complex designs, and the UVM-inspired methodology provides a solid foundation for scaling to larger verification projects. The system's ability to operate in both deterministic and randomized modes makes it suitable for both development debugging and production validation scenarios.

---

## Appendix A: File Structure

```
alu_verification/
├── alu_server.py              # Python verification server
├── alu_veriification.ino      # Arduino firmware
├── index.html                 # Web monitoring interface
├── peripheral.txt             # Pin mapping reference
└── [screenshots/videos]       # Documentation media
```

## Appendix B: Dependencies

**Python Requirements:**
- `pyserial` (serial communication)
- `websockets` (WebSocket server)
- `asyncio` (standard library, async I/O)

**Arduino Requirements:**
- Arduino Uno or compatible board
- Standard Arduino IDE libraries

**Web Interface:**
- Modern web browser with WebSocket support
- Tailwind CSS (loaded via CDN)

## Appendix C: Configuration Parameters

| Parameter | Location | Default | Description |
|-----------|----------|---------|-------------|
| `ARDUINO_PORT` | `alu_server.py` | `'COM5'` | Serial port identifier |
| `BAUD_RATE` | `alu_server.py` | `115200` | Serial communication speed |
| `WEB_SOCKET_PORT` | `alu_server.py` | `8765` | WebSocket server port |
| `VERIFICATION_INTERVAL_SEC` | `alu_server.py` | `0.5` | Cycle time in seconds |
| `HALF_PERIOD_US` | `alu_veriification.ino` | `100` | Clock half-period in microseconds |
| `WS_URL` | `index.html` | `ws://127.0.0.1:8765` | WebSocket server URL |

---

*Document Version: 1.0*  
*Last Updated: November 2024*  
*Author: ALU Verification Project Team*

