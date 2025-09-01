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
- https://www.lenovo.com/ie/en/glossary/what-is-pcie/?orgRef=https%253A%252F%252Fwww.google.com%252F

## Spyder

## Cloud