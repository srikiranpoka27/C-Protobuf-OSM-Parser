# OpenStreetMap PBF Parser in C

## Overview

This project is a lightweight, from-scratch implementation of a parser for OpenStreetMap (OSM) data in the compact PBF (Protocolbuffer Binary Format), developed in C. It reads and extracts geographic information from raw PBF files using custom-built parsing logic, without relying on any external protobuf libraries.

## Features

- **Low-Level Protocol Buffer Parsing:** The parser directly interprets the binary-encoded protocol buffer format by manually decoding wire types and field numbers.
- **Streaming Support:** Data is processed as a stream from standard input or file input, making the tool memory-efficient and suitable for large datasets.
- **Compressed Blob Handling:** Supports PBF blob decompression using zlib to access raw data chunks inside the file.
- **Flexible Querying:** Allows querying of core OSM elements such as nodes, ways, and summary information through a structured command-line interface.
- **Memory-Efficient Design:** Custom message structures and tight control over memory allocation ensure efficient performance on constrained systems.
- **No External Dependencies:** All functionality is implemented from scratch, including file parsing, buffer management, and protocol decoding logic.
