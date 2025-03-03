#!/usr/bin/env python3
import socket
import time

# Broker settings
BROKER = "tigoe.net"
PORT = 1883

# MQTT Protocol Constants
PROTOCOL_NAME = b'MQTT'
PROTOCOL_LEVEL = 4  # MQTT 3.1.1
CONNECT_FLAGS = 0xC2  # Clean Session + Username + Password
# Connect flags breakdown:
# 0xC2 = 11000010 in binary
# Bit 7: Username flag (1)
# Bit 6: Password flag (1)
# Bit 5: Will Retain (0)
# Bit 4-3: Will QoS (00)
# Bit 2: Will Flag (0)
# Bit 1: Clean Session (1)
# Bit 0: Reserved (0)
KEEP_ALIVE = 60  # seconds - time interval the client commits to maintaining communication with the broker

# Client credentials
CLIENT_ID = b'mgandhi'
USERNAME = b'conndev'
PASSWORD = b'b4s1l!'

# Topic and message
TOPIC = b'tesing/topic/two/three/four/five/six/seven/eight/nine/ten/11/12/13/14/15/16/17/18'
MESSAGE = b'Hello MQTT'

# --- Build MQTT CONNECT Packet ---
# MQTT packet structure:
# +----------------+--------------------+---------------+
# | Fixed Header   | Variable Header    | Payload       |
# | (2+ bytes)     | (10 bytes-CONNECT) | (variable)    |
# +----------------+--------------------+---------------+

# Variable header components
protocol_name_len = len(PROTOCOL_NAME)
# Protocol name is prefixed with its length as a 2-byte field (MSB, LSB)
protocol_header = bytes([0x00, protocol_name_len]) + PROTOCOL_NAME
protocol_level = bytes([PROTOCOL_LEVEL])  # Single byte indicating MQTT version
connect_flags = bytes([CONNECT_FLAGS])    # Connection behavior flags
# Keep alive is a 2-byte integer (MSB, LSB)
keep_alive = bytes([KEEP_ALIVE >> 8, KEEP_ALIVE & 0xFF])

# Payload components - each string is prefixed with a 2-byte length field
client_id_len = len(CLIENT_ID)
client_id = bytes([0x00, client_id_len]) + CLIENT_ID

username_len = len(USERNAME)
username = bytes([0x00, username_len]) + USERNAME

password_len = len(PASSWORD)
password = bytes([0x00, password_len]) + PASSWORD

# Assemble variable header and payload
variable_header = protocol_header + protocol_level + connect_flags + keep_alive
payload = client_id + username + password

# Calculate remaining length (size of variable header + payload)
remaining_length = len(variable_header) + len(payload)
# For simplicity, assuming remaining_length < 128 (fits in one byte)
# In a complete implementation, this would use variable-length encoding
remaining_length_bytes = bytes([remaining_length])

# Assemble complete CONNECT packet
# 0x10 = MQTT Control Packet type 1 (CONNECT) in the first 4 bits, with 0000 flags
connect_packet = b'\x10' + remaining_length_bytes + variable_header + payload

# CONNECT packet structure:
# +----------------+--------------------+------------------------+
# | Fixed Header   | Variable Header    | Payload                |
# | 0x10 + Length  | Protocol details   | ClientID + User + Pass |
# +----------------+--------------------+------------------------+
#
# Fixed Header:
# +--------+-------------+
# | 0x10   | Rem. Length |
# +--------+-------------+
#
# Variable Header:
# +----------------+----------------+----------------+----------------+
# | Protocol Name  | Protocol Level | Connect Flags  | Keep Alive     |
# | Length + "MQTT"| 0x04           | 0xC2           | 2 bytes (60s)  |
# +----------------+----------------+----------------+----------------+

# --- Build MQTT PUBLISH Packet ---
# Variable header - Topic
topic_len = len(TOPIC)
# Topic name is prefixed with its length as a 2-byte field
topic_header = bytes([0x00, topic_len]) + TOPIC

# Calculate remaining length (topic length field + topic + message)
publish_remaining_length = len(topic_header) + len(MESSAGE)
# Again, assuming length < 128 bytes for simplicity
publish_remaining_length_bytes = bytes([publish_remaining_length])

# Assemble complete PUBLISH packet
# 0x30 = MQTT Control Packet type 3 (PUBLISH) in the first 4 bits, with 0000 flags
# Flags would normally set QoS, DUP, and RETAIN properties
publish_packet = b'\x30' + publish_remaining_length_bytes + topic_header + MESSAGE

# PUBLISH packet structure:
# +----------------+--------------------+---------------+
# | Fixed Header   | Variable Header    | Payload       |
# | 0x30 + Length  | Topic              | Message       |
# +----------------+--------------------+---------------+
#
# Fixed Header:
# +--------+-------------+
# | 0x30   | Rem. Length |
# +--------+-------------+
#
# Variable Header:
# +----------------+
# | Topic Name     |
# | Length + Topic |
# +----------------+

# --- Build MQTT DISCONNECT Packet ---
# 0xE0 = MQTT Control Packet type 14 (DISCONNECT) in the first 4 bits, with 0000 flags
# 0x00 = Remaining Length (0, as DISCONNECT has no variable header or payload)
disconnect_packet = b'\xE0\x00'

# DISCONNECT packet structure (simplest packet):
# +----------------+
# | Fixed Header   |
# | 0xE0 + 0x00    |
# +----------------+

# --- Send the packets over TCP ---
# Create a socket connection to the MQTT broker
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((BROKER, PORT))

# Send CONNECT packet
sock.send(connect_packet)
time.sleep(1)  # Wait for broker to process the connection

# Send PUBLISH packet
sock.send(publish_packet)
time.sleep(1)  # Wait for broker to process the message

# Send DISCONNECT packet
sock.send(disconnect_packet)

# Close the socket
sock.close()
