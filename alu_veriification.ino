
// --- ARDUINO CODE FOR 2-BIT ALU INTERFACE ---

const int A_INPUT_PIN = 3;     // Yellow1 - A input
const int B_INPUT_PIN = 4;     // Yellow2 - B input

const int P_NAND = 8;        // yellow
const int P_NOT_A = 9;   // green
const int P_OR = 10;         // blue
const int P_CARRY_OUT = 5; // pink
const int P_SUM = 6;         // grey
const int P_AND = 11; // white
const int P_XOR = 12;        // brown
const int CLOCK_PIN = 13;
const int HALF_PERIOD_US = 100;

volatile int current_a = 0;
volatile int current_b = 0;
volatile bool random_mode = false;

void setup() {
  Serial.begin(115200);
  delay(100); 

  pinMode(A_INPUT_PIN, OUTPUT);
  pinMode(B_INPUT_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(P_NAND, INPUT_PULLUP);
  pinMode(P_NOT_A, INPUT_PULLUP);
  pinMode(P_OR, INPUT_PULLUP);
  pinMode(P_CARRY_OUT, INPUT_PULLUP);
  pinMode(P_SUM, INPUT_PULLUP);
  pinMode(P_AND, INPUT_PULLUP);
  pinMode(P_XOR, INPUT_PULLUP);

  digitalWrite(A_INPUT_PIN, LOW);
  digitalWrite(B_INPUT_PIN, LOW);
  
  Serial.println("STATUS: Ready to receive commands (Mab, R, S).");
}

void loop() {
  // clock near 5 khz
  digitalWrite(CLOCK_PIN, HIGH);
  delayMicroseconds(HALF_PERIOD_US);
  digitalWrite(CLOCK_PIN, LOW);
  delayMicroseconds(HALF_PERIOD_US);

  // handle serial commands from python
  checkscommds();
  
  // if random mode, generate new inputs here
  if (random_mode) {
      // In random mode, the python server dictates the 500ms cycle 
      // by constantly sending 'R' or 'M' commands.so, only apply random inputs here when commanded by the server.
      current_a = random(2); 
      current_b = random(2);
      applyinpuutts();
      delay(1);}
  senddatatopython();
}

void checkscommds() {
  static String commandBuffer = ""; 
  
  while (Serial.available()) {
    char inChar = Serial.read();
    if (inChar == '\n' || inChar == '\r') {
      commandBuffer.toUpperCase();
      if (commandBuffer.startsWith("R")) {
        random_mode = true;
        Serial.println("STATUS: Random Mode ON");} 
      else if (commandBuffer.startsWith("M")) {
        random_mode = false;
        if (commandBuffer.length() >= 3) { // Expecting "M" + a + b ------------ Read 'a' and 'b' from the buffer
          int a_val = commandBuffer.charAt(1) - '0';
          int b_val = commandBuffer.charAt(2) - '0';
          
          if ((a_val == 0 || a_val == 1) && (b_val == 0 || b_val == 1)) {
            current_a = a_val;
            current_b = b_val;
            applyinpuutts();
            Serial.print("STATUS: Manual Input Set: A=");
            Serial.print(current_a);
            Serial.print(", B=");
            Serial.println(current_b);} 
          else {
            Serial.println("ERROR: Invalid manual input values. Use 'M10' or 'M01'.");}} 
        else {
           Serial.println("ERROR: Incomplete manual input. Expecting 'Mab'.");}} 
        
      else if (commandBuffer.startsWith("S")) { // 'S' for stop/manual
        random_mode = false;
        Serial.println("STATUS: Manual Mode ON (No random updates)");}
      
      commandBuffer = "";} 
    else if (inChar != ' ') { // Ignore spaces
      if (commandBuffer.length() < 10) { 
        commandBuffer += inChar;
      }}}}

void applyinpuutts() {
  digitalWrite(A_INPUT_PIN, current_a);
  digitalWrite(B_INPUT_PIN, current_b);}

void senddatatopython() {
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
