// --- ARDUINO CODE FOR 2-BIT ALU INTERFACE ---

// Define Input Pins (from Python/Web to ALU)
const int A_INPUT_PIN = 3;     // Yellow1 - A input
const int B_INPUT_PIN = 4;     // Yellow2 - B input

// Define Output Pins (from ALU to Arduino)
const int P_NAND = 8;        // yellow
const int P_NOT_A = 9;       // green
const int P_OR = 10;         // blue
const int P_CARRY_OUT = 5;   // pink
const int P_SUM = 6;         // grey
const int P_AND = 11;        // white
const int P_XOR = 12;        // brown

// Define Clock Pin (for physical 5kHz signal)
const int CLOCK_PIN = 13;

// Clock signal timing for the pulse (5 kHz): 100us HIGH, 100us LOW
const int HALF_PERIOD_US = 100;

// State variables
volatile int current_a = 0;
volatile int current_b = 0;
volatile bool random_mode = false;

void setup() {
  // Initialize Serial Communication
  Serial.begin(115200);
  // Wait a moment for serial to initialize, especially important for Uno/Nano clones
  delay(100); 

  // Setup Input Control Pins (output from Arduino to ALU)
  pinMode(A_INPUT_PIN, OUTPUT);
  pinMode(B_INPUT_PIN, OUTPUT);

  // Setup Clock Pin (output from Arduino)
  pinMode(CLOCK_PIN, OUTPUT);
  
  // Setup ALU Output Pins (input to Arduino)
  // INPUT_PULLUP ensures inputs are HIGH when disconnected/floating, 
  // preventing false readings.
  pinMode(P_NAND, INPUT_PULLUP);
  pinMode(P_NOT_A, INPUT_PULLUP);
  pinMode(P_OR, INPUT_PULLUP);
  pinMode(P_CARRY_OUT, INPUT_PULLUP);
  pinMode(P_SUM, INPUT_PULLUP);
  pinMode(P_AND, INPUT_PULLUP);
  pinMode(P_XOR, INPUT_PULLUP);

  // Set initial input state to 0, 0
  digitalWrite(A_INPUT_PIN, LOW);
  digitalWrite(B_INPUT_PIN, LOW);
  
  Serial.println("STATUS: Ready to receive commands (Mab, R, S).");
}

void loop() {
  // 1. Clock Pulse Generation (Physical 5 kHz signal on Pin 13)
  // This signal runs continuously without affecting the data transmission rate.
  digitalWrite(CLOCK_PIN, HIGH);
  delayMicroseconds(HALF_PERIOD_US);
  digitalWrite(CLOCK_PIN, LOW);
  delayMicroseconds(HALF_PERIOD_US);

  // 2. Handle serial commands from Python
  checkSerialCommands();
  
  // 3. If in random mode, generate new inputs here
  if (random_mode) {
      // In random mode, the Python server dictates the 500ms cycle 
      // by constantly sending 'R' or 'M' commands.
      // We only apply random inputs here when commanded by the server.
      current_a = random(2); 
      current_b = random(2);
      applyInputs();
      // Wait slightly after applying inputs before sending data
      delay(1); 
  }
  
  // 4. Read Outputs and Send Data (This is triggered every cycle/when Python commands)
  sendDataToPython();
}

// Function to handle incoming serial commands from Python/Web
void checkSerialCommands() {
  // Use a temporary buffer to capture the whole command
  static String commandBuffer = ""; 
  
  while (Serial.available()) {
    char inChar = Serial.read();
    
    // Check for end of line marker
    if (inChar == '\n' || inChar == '\r') {
      commandBuffer.toUpperCase(); // Convert to uppercase for robustness

      if (commandBuffer.startsWith("R")) { // 'R' for Random Mode
        random_mode = true;
        // Inputs will be set by the random generator in the loop()
        Serial.println("STATUS: Random Mode ON");
        
      } else if (commandBuffer.startsWith("M")) { // 'Mab' for Manual Mode
        random_mode = false;
        
        if (commandBuffer.length() >= 3) { // Expecting "M" + a + b
          // Read 'a' and 'b' from the buffer
          int a_val = commandBuffer.charAt(1) - '0';
          int b_val = commandBuffer.charAt(2) - '0';
          
          if ((a_val == 0 || a_val == 1) && (b_val == 0 || b_val == 1)) {
            current_a = a_val;
            current_b = b_val;
            applyInputs();
            Serial.print("STATUS: Manual Input Set: A=");
            Serial.print(current_a);
            Serial.print(", B=");
            Serial.println(current_b);
          } else {
            Serial.println("ERROR: Invalid manual input values. Use 'M10' or 'M01'.");
          }
        } else {
           Serial.println("ERROR: Incomplete manual input. Expecting 'Mab'.");
        }
        
      } else if (commandBuffer.startsWith("S")) { // 'S' for Stop/Manual (no random updates)
        random_mode = false;
        Serial.println("STATUS: Manual Mode ON (No random updates)");
      }
      
      // Clear buffer for next command
      commandBuffer = "";
      
    } else if (inChar != ' ') { // Ignore spaces
      // Add character to buffer (limit length just in case)
      if (commandBuffer.length() < 10) { 
        commandBuffer += inChar;
      }
    }
  }
}

// Function to apply the current A and B values to the ALU
void applyInputs() {
  digitalWrite(A_INPUT_PIN, current_a);
  digitalWrite(B_INPUT_PIN, current_b);
}

// Function to read all ALU outputs and format them for Python
void sendDataToPython() {
  // Read digital inputs (ALU outputs)
  int out_nand = digitalRead(P_NAND);
  int out_not_a = digitalRead(P_NOT_A);
  int out_or = digitalRead(P_OR);
  int out_cout = digitalRead(P_CARRY_OUT);
  int out_sum = digitalRead(P_SUM);
  int out_and = digitalRead(P_AND);
  int out_xor = digitalRead(P_XOR);

  // Format: [A_IN,B_IN|NAND,NOT_A,OR,COUT,SUM,AND,XOR]
  // Note: Outputs printed in the specified order (8, 9, 10, 5, 6, 11, 12)
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
}