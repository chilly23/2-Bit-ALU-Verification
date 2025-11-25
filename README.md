I made a 2Bit ALU, and developed a full verification flow around it using in Python, Arduino, and monitoring interface. The Arduino acted as the hardware bridge to translate, encode and decode signals, and stream them to the verification script over USB.



For those who don't know this: This comes under Chip design, Verification, Embedded systems, Software monitor/testing and Hardware Engineering.



The Python environment handled stimulus generation, automated checking, and real time scoreboard visualisation. Green cells indicated correct matches and red cells flagged mismatches across both deterministic and randomised test sequences. 

The ALU supports a set of arithmetic and logic operations (addition, subtraction, multiplication, NOT, AND, NAND, XOR, XNOR, OR) along with internal flag registers and counters. The overall verification structure mirrors a UVM style flow: design → testbench → encoding/decoding peripherals → sequencer → monitor → scoreboard



The complete verification cycle originally ran in about 150 milliseconds. I slowed it down to around 3 seconds for recording.



#STEM #TechCommunity #MakerCommunity #OpenHardware #ElectronicsEngineering #ComputerEngineering #ChipDesign #DigitalDesign #VLSI #FPGA
