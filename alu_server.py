import serial
import time
import asyncio
import websockets
import json
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

ARDUINO_PORT = 'COM5'
BAUD_RATE = 115200
WEB_SOCKET_HOST = '0.0.0.0'
WEB_SOCKET_PORT = 8765
VERIFICATION_INTERVAL_SEC = 0.5

arduino_serial = None
global_input_a = 0
global_input_b = 0

# AUTO is the default mode
global_mode = 'AUTO'

ALU_STATE = {
    "A": 0, "B": 0,
    "NAND": 0, "NOT_A": 0, "OR": 0, "COUT": 0, "SUM": 0, "AND": 0, "XOR": 0,
    "VerificationStatus": "Waiting",
    "Timestamp": time.time()
}

EXPECTED_OUTPUTS = {
    (0, 0): (1, 1, 0, 0, 0, 0, 0),
    (0, 1): (1, 1, 1, 0, 1, 0, 1),
    (1, 0): (1, 0, 1, 0, 1, 0, 1),
    (1, 1): (0, 0, 1, 1, 0, 1, 0)
}

OUTPUT_MAP = {
    0: "NAND", 1: "NOT_A", 2: "OR", 3: "COUT",
    4: "SUM", 5: "AND", 6: "XOR"
}

def verify_outputs(a_in, b_in, actual_outputs_str):
    key = (a_in, b_in)
    expected = EXPECTED_OUTPUTS.get(key)

    if not expected:
        return "ERROR", {"Details": "Invalid input set"}

    try:
        actual = tuple(int(c) for c in actual_outputs_str)
    except:
        return "ERROR", {"Details": "Bad output format"}

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

    if mismatches == 0:
        return "OK", verification
    elif mismatches == 7:
        return "ERROR", verification
    else:
        return "MIXED", verification


def setup_serial():
    global arduino_serial
    logging.info(f"Connecting to Arduino on {ARDUINO_PORT}")
    arduino_serial = serial.Serial(ARDUINO_PORT, BAUD_RATE, timeout=0.1)
    time.sleep(2)
    arduino_serial.reset_input_buffer()
    logging.info("Serial ready")


def read_and_process_data():
    global ALU_STATE
    if not arduino_serial:
        return

    try:
        while arduino_serial.in_waiting > 0:
            line = arduino_serial.readline().decode('utf8', errors='ignore').strip()

            if not (line.startswith('[') and line.endswith(']')):
                if line:
                    logging.info(f"[Arduino]: {line}")
                continue

            data = line[1:-1]

            if '|' not in data:
                continue

            inputs_str, outputs_str = data.split('|')
            if len(outputs_str) != 7:
                continue

            a_str, b_str = inputs_str.split(',')
            a_in = int(a_str)
            b_in = int(b_str)

            status, details = verify_outputs(a_in, b_in, outputs_str)

            ALU_STATE = {
                "A": a_in,
                "B": b_in,
                "NAND": int(outputs_str[0]),
                "NOT_A": int(outputs_str[1]),
                "OR": int(outputs_str[2]),
                "COUT": int(outputs_str[3]),
                "SUM": int(outputs_str[4]),
                "AND": int(outputs_str[5]),
                "XOR": int(outputs_str[6]),
                "VerificationStatus": status,
                "VerificationDetails": details,
                "Timestamp": time.time(),
                "Mode": global_mode
            }
    except Exception as e:
        logging.error(f"Error parsing serial: {e}")


def send_to_arduino(cmd):
    if arduino_serial:
        arduino_serial.write(cmd.encode() + b'\n')
        time.sleep(0.01)


CONNECTIONS = set()

async def handler(websocket):
    CONNECTIONS.add(websocket)
    try:
        async for message in websocket:
            pass
    except:
        pass
    finally:
        CONNECTIONS.remove(websocket)


def advance_inputs():
    global global_input_a, global_input_b

    if global_input_a == 0 and global_input_b == 0:
        global_input_b = 1
    elif global_input_a == 0 and global_input_b == 1:
        global_input_a = 1
        global_input_b = 0
    elif global_input_a == 1 and global_input_b == 0:
        global_input_b = 1
    else:
        global_input_a = 0
        global_input_b = 0


async def alu_loop():
    while True:
        start = time.time()

        send_to_arduino(f"M{global_input_a}{global_input_b}")
        read_and_process_data()

        if CONNECTIONS:
            websockets.broadcast(CONNECTIONS, json.dumps(ALU_STATE))

        advance_inputs()

        elapsed = time.time() - start
        remaining = VERIFICATION_INTERVAL_SEC - elapsed
        if remaining > 0:
            await asyncio.sleep(remaining)


async def main():
    setup_serial()
    asyncio.create_task(alu_loop())

    async with websockets.serve(handler, WEB_SOCKET_HOST, WEB_SOCKET_PORT):
        logging.info(f"WebSocket running at ws://{WEB_SOCKET_HOST}:{WEB_SOCKET_PORT}")
        await asyncio.Future()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        if arduino_serial:
            arduino_serial.close()
