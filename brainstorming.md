# Claude is used in understanding, syntax, debugging
# Brainstorming

This file is used to document your thoughts, approaches and research conducted across all tasks in the Technical Assessment.

## Before everything

1. Docker revision

## Firmware

TASK 1

1. Looked thru some files to understand what I'm working with:
- dbc files, dump log, solution, cmakelist, dockerfile, so on.

2. did a cheeky grep to see the CAN message definitions and signals
- grep -A 5 "^BO_" dbc-files/SensorBus.dbc | head -30

3. added dbcppp as a git submodule

4. updated cmakelists.txt to add in dbcppp
- disabled unnecessary components like tools, tests, examples, KCD support
- added dbcppp as subdir
- linked dbcppp lib to answer executable

5. updated dockerfile to include dependencies
- libboost-all-dev (dbcppp Boost requirement)
- libxml2-dev (XML parsing)

6. CAN parser (main.cpp)
- initializeNetworks(): Loads the three DBC files, map to interfaces
- parseLine(): Parse dump.log,  convert hex CAN ID to decimal
- processFrame(): Match CAN ID to DBC message, decode signals using sig.Decode() and sig.RawToPhys()
- processCANDump(): Main loop, sort results by timestamp
- writeOutput(): Write 363,489 decoded signals to output.txt

7. processing
- 363489 signals
- encountered some issues like pressure sensors & steering angle having huge numbers,
which i found out that it was signed values being interpreted wrongly:
- got really huge numbers which were key indicators of unsigned overflow from -ve signed values
- checked DBC files inside the container to understand signal definitions, and raw data
- '@1-': indicates signed values with little endian byte order
- adjusted data conversions for signed/unsigned signal interpretation, scaling factors, offsets

8. validation
- did some sanity checks (sensor ranges)
- looks reasonable:
- pressures: around -5k to 5k kPa?
- speeds: 0-few hundred kmph
- voltage: 0-1000v
- temps: -50 to 150 degrees

steps to run it in docker:

1. build the docker image:

docker compose up --build

2. once built, open another terminal and run:

docker ps

docker exec -it firmware-firmware-1 bash

3. cd into solution: cd /app/build/solution
and run either answer or tests

TASK 2

Resources I looked through:

- http://dewesoft.com/blog/what-is-can-bus
- https://www.youtube.com/watch?v=JZSCzRT9TTo&t=196s&ab_channel=Electronoobs
- https://www.youtube.com/watch?v=IyGwvGzrqp8&t=61s&ab_channel=Electronoobs
- https://www.youtube.com/watch?v=0nVNwozXsIc&ab_channel=Rohde%26Schwarz
- https://www.mercedesamgf1.com/news/feature-data-and-electronics-in-f1-explained
- https://medium.com/formula-one-forever/data-the-unseen-driver-in-formula-1-cars-63f31c16f2fe
- https://embetronicx.com/uncategorized/spi-vs-can-choosing-the-right-protocol-for-your-project/
- https://www.quora.com/Why-is-controller-area-network-preferred-over-USBs-in-automobiles-and-industry-automation-for-rapid-transfer-of-data-between-devices
- https://www.lenovo.com/ie/en/glossary/what-is-pcie/?orgRef=https%253A%252F%252Fwww.google.com%

STM32 Chip Selection

- Went to the product selector webpage, filtered the columns to only show:

Some general specs like:
1. Part number
2. Marketing Status
3. Package
4. Core
5. Operating frequency

Specific specs:
1. Flash Size (kb) Prog to 2048
2. Timers 16-bit typ to 12
3. Number of A/D Converters typ to >= 3
4. CAN 2.0 to >= 3
5. Ethernet
6. USB Type under USB Interfaces

Other:
1. Buy On Line

Exported to an excel file, eligible products file is in the firmware folder.
Checked distributors' stock, lead time, cost.
Choice : STM32F767ZI

Task 3:

I decided that since there are 5 recommended unit tests to implement, it'll be good to have a single entry point (testmain.cpp) to include all modules

1. canframecheck.cpp:

since my main.cpp file uses sscanf to parse the camdump format, some issues such as:

- malformed parentheses and timestamps
- non hex CAN IDs
- corrupted hex data 

may occur, so i did a canframe struct that mirrors the main.cpp structure

test cases:

- interface validation, where it maps:
can0 -> ControlBus
can1 -> SensorBus
can2 -> TractiveBus

- CAN ID boundaries (standard vs extended)
- data length limits (test 0-8 byte scenarios)
- error recovery

2. sensorvalue.cpp

since main.cpp calls sig.Decode() and sig.RawToPhys() from dbcppp, the tests need to work without that library, so i created a test implementation mirroring that:

static uint64_t extractBits(const std::vector<uint8_t>& data, const Signal& sig)
static double convertToPhysical(uint64_t raw, const Signal& sig)

test cases:

- Endianness handling 
1. Little endian (intel) vs Big endia (motorola) bit ordering

- signed value interpretation
- scaling - temp sensors, rpm
- bit level extraction when signals dont align to byte boundaries

3. idhandling.cpp 

since main.cpp loads 3 DBC files, the risks are that the same CAN ID is defined differently across files

- reality: 

OEM DBC defines CAN ID 0x2C0 as vehicle speed (scale 0.01). Aftermarket ECU DBC defines same ID as wheel speed (scale 0.0625). since code processes both, there might be inconsistent results

example dbc structure with dbcppp dependency

conflict detection:

1. Map CAN IDs to all messages that use them
2. Check for bit overlaps between signals
3. Check for same signal name, different scaling
4. Report interface combinations (can0+can2 conflict, etc.)

test cases:

- no conflicts: different CAN IDs across dbc files
- Bit overlaps: two dbc files define overlapping bit ranges for same CAN ID
- Scaling conflicts: same signal name, different scale/offset values
- 3way conflicts: all three DBC files define same CAN ID

4. calculationcheck.cpp

since CAN signals use complex bit layouts, 
test cases:

1. Temperature scaling - Raw 160 * 0.75 - 48 = 72°C
2. RPM calculation - Raw 4000 * 0.25 = 1000 RPM
3. Signed steering - 0xFF as 8-bit signed = -1 degrees
4. Voltage precision - Raw 12500 * 0.001 = 12.5V

Boundary tests
- single bit extraction (status flags)
- max bit lengths (64-bit values)
- cross-byte scenarios (16-bit signal spanning bytes)

5. errorhandling.cpp
since the main.cpp file has try-catch around parseLine(), where bad lines are skipped, here the tests define what bad is:

- Format check: (timestamp) interface id#data structure
- Timestamp validation: numeric, positive, reasonable range
- Interface validation: only can0/can1/can2 allowed (matches your DBC mapping)
- CAN ID validation: hex format, within 29-bit limit
- Data validation: even hex length, max 16 characters (8 bytes)

I edited the docker-compose.yml file since initially,
the platform is being set to linux/amd64, which makes docker
run the container in x86_64 emulation but im on apple silicon

solution: removed 'platform:' line and let docker pick the correct architecture for my machine

Steps to run the test cases inside docker:

1. Enter the container:

docker compose -f 'firmware/docker-compose.yml' up -d --build 'firmware' 

if this doesnt work, go directly to the docker-compose.yml and click 'Run Service'

once running, 

2. Open another terminal, 

docker compose exec firmware bash

3. cd to the solution directory and check if the test binary exists:

cd /app/build/solution
ls -l

4. run tests

./tests

5. exit when done

exit

Stage 4:

tried doing the dbc parser from scratch without the library, the foudations is somewhat complete but the signal regex isnt matching the dbc definitions, so 0 signals parsed. messages were found but no signals extracted and the decoding is incomplete

## Spyder

## Cloud