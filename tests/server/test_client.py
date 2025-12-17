#!/usr/bin/env python3
"""
R-Type Server Test Client
Interactive UDP client to test the R-Type server protocol
"""

import socket
import struct
import time
import sys
from typing import Optional

class RTypeTestClient:
    """Interactive test client for R-Type server"""
    CLIENT_CONNECT = 0x01
    CLIENT_DISCONNECT = 0x02
    CLIENT_PING = 0x04
    CLIENT_JOIN_LOBBY = 0x05
    CLIENT_LEAVE_LOBBY = 0x06
    CLIENT_INPUT = 0x10

    SERVER_ACCEPT = 0x81
    SERVER_REJECT = 0x82
    SERVER_PLAYER_JOINED = 0x83
    SERVER_PLAYER_LEFT = 0x84
    SERVER_PONG = 0x85
    SERVER_LOBBY_STATE = 0x87

    def __init__(self, host: str = 'localhost', port: int = 4242):
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(2.0)
        self.server_address = (host, port)
        self.player_id: Optional[int] = None
        self.player_name: str = ""
        self.sequence_number: int = 1
        self.connected: bool = False

    def build_header(self, packet_type: int, payload_length: int) -> bytes:
        """Build packet header (8 bytes)"""
        return struct.pack('!BBHI',
            0x01,
            packet_type,
            payload_length,
            self.sequence_number
        )

    def send_packet(self, packet_type: int, payload: bytes):
        """Send a packet to the server"""
        header = self.build_header(packet_type, len(payload))
        packet = header + payload

        print(f"[→] Sending packet type 0x{packet_type:02X}, size: {len(packet)} bytes")
        self.sock.sendto(packet, self.server_address)
        self.sequence_number += 1

    def receive_packet(self, timeout: float = 2.0) -> Optional[tuple]:
        """Receive a packet from the server"""
        old_timeout = self.sock.gettimeout()
        self.sock.settimeout(timeout)

        try:
            data, addr = self.sock.recvfrom(2048)
            self.sock.settimeout(old_timeout)
            if len(data) < 8:
                print(f"[!] Received packet too small: {len(data)} bytes")
                return None
            version, pkt_type, payload_len, seq_num = struct.unpack('!BBHI', data[:8])
            payload = data[8:8+payload_len]
            return (version, pkt_type, payload_len, seq_num, payload)
        except socket.timeout:
            self.sock.settimeout(old_timeout)
            print("[!] Timeout waiting for response")
            return None

    def connect_to_server(self, name: str):
        """Send CLIENT_CONNECT packet"""
        self.player_name = name
        player_name_bytes = name.encode('utf-8')[:32].ljust(32, b'\x00')
        payload = struct.pack('B', 0x01) + player_name_bytes

        print(f"\n[*] Connecting as '{name}'...")
        self.send_packet(self.CLIENT_CONNECT, payload)
        response = self.receive_packet()
        if response:
            version, pkt_type, payload_len, seq_num, payload = response
            if pkt_type == self.SERVER_ACCEPT:
                player_id, tick_rate, max_players, map_id = struct.unpack('!IBBH', payload)
                self.player_id = player_id
                self.connected = True
                print(f"[✓] Connected successfully!")
                print(f"    Player ID: {player_id}")
                print(f"    Server tick rate: {tick_rate} Hz")
                print(f"    Max players: {max_players}")
                print(f"    Map ID: {map_id}")
            elif pkt_type == self.SERVER_REJECT:
                reason_code = payload[0]
                reason_msg = payload[1:65].decode('utf-8').rstrip('\x00')
                print(f"[✗] Connection rejected!")
                print(f"    Reason code: {reason_code}")
                print(f"    Message: {reason_msg}")

    def disconnect_from_server(self):
        """Send CLIENT_DISCONNECT packet"""
        if not self.connected:
            print("[!] Not connected to server")
            return
        payload = struct.pack('!IB', self.player_id, 0x01)

        print(f"\n[*] Disconnecting from server...")
        self.send_packet(self.CLIENT_DISCONNECT, payload)
        self.connected = False
        self.player_id = None
        print("[✓] Disconnected")

    def send_ping(self):
        """Send CLIENT_PING packet"""
        if not self.connected:
            print("[!] Not connected to server")
            return
        timestamp = int(time.time() * 1000) & 0xFFFFFFFF
        payload = struct.pack('!II', self.player_id, timestamp)

        print(f"\n[*] Sending ping (timestamp: {timestamp})...")
        send_time = time.time()
        self.send_packet(self.CLIENT_PING, payload)
        response = self.receive_packet()
        if response:
            version, pkt_type, payload_len, seq_num, payload = response
            if pkt_type == self.SERVER_PONG:
                recv_time = time.time()
                rtt = (recv_time - send_time) * 1000 # in milliseconds
                client_ts, server_ts = struct.unpack('!II', payload)
                print(f"[✓] Pong received!")
                print(f"    Round-trip time: {rtt:.2f} ms")
                print(f"    Client timestamp: {client_ts}")
                print(f"    Server timestamp: {server_ts}")

    def join_lobby(self, game_mode: int, difficulty: int):
        """Send CLIENT_JOIN_LOBBY packet"""
        if not self.connected:
            print("[!] Not connected to server")
            return
        payload = struct.pack('!IBB', self.player_id, game_mode, difficulty)
        game_modes = {1: "DUO", 2: "TRIO", 3: "SQUAD"}
        difficulties = {1: "EASY", 2: "NORMAL", 3: "HARD"}

        print(f"\n[*] Joining lobby ({game_modes.get(game_mode, '?')}, {difficulties.get(difficulty, '?')})...")
        self.send_packet(self.CLIENT_JOIN_LOBBY, payload)
        response = self.receive_packet()
        if response:
            version, pkt_type, payload_len, seq_num, payload = response
            if pkt_type == self.SERVER_LOBBY_STATE:
                print(f"[✓] Lobby state received (TODO: parse)")

    def leave_lobby(self, lobby_id: int):
        """Send CLIENT_LEAVE_LOBBY packet"""
        if not self.connected:
            print("[!] Not connected to server")
            return
        payload = struct.pack('!II', self.player_id, lobby_id)

        print(f"\n[*] Leaving lobby {lobby_id}...")
        self.send_packet(self.CLIENT_LEAVE_LOBBY, payload)
        print("[✓] Left lobby")

    def send_input(self, flags: int):
        """Send CLIENT_INPUT packet"""
        if not self.connected:
            print("[!] Not connected to server")
            return
        # Client tick (monotonically increasing) - assuming 64 Hz client logic
        tick = int(time.time() * 64) & 0xFFFFFFFF
        payload = struct.pack('!IHI', self.player_id, flags, tick)

        print(f"\n[*] Sending input (flags: 0x{flags:04X})...")
        self.send_packet(self.CLIENT_INPUT, payload)

    def send_custom_payload(self, packet_type: int, hex_payload: str):
        """Send custom packet with hex payload"""
        try:
            payload = bytes.fromhex(hex_payload.replace(' ', ''))
            print(f"\n[*] Sending custom packet type 0x{packet_type:02X}...")
            self.send_packet(packet_type, payload)
            response = self.receive_packet(timeout=1.0)
            if response:
                version, pkt_type, payload_len, seq_num, payload = response
                print(f"[✓] Received response type 0x{pkt_type:02X}")
                print(f"    Payload ({len(payload)} bytes): {payload.hex()}")
        except ValueError as e:
            print(f"[!] Invalid hex string: {e}")

    def close(self):
        """Close the socket"""
        self.sock.close()

def print_menu():
    """Print interactive menu"""
    print("\n" + "="*50)
    print("R-Type Server Test Client")
    print("="*50)
    print("1. Connect to server")
    print("2. Disconnect from server")
    print("3. Send ping")
    print("4. Join lobby")
    print("5. Leave lobby")
    print("6. Send input")
    print("7. Send custom packet")
    print("8. Show status")
    print("0. Quit")
    print("="*50)

def main():
    """Main interactive loop"""
    if len(sys.argv) > 1:
        host = sys.argv[1]
    else:
        host = 'localhost'

    if len(sys.argv) > 2:
        port = int(sys.argv[2])
    else:
        port = 4242

    print(f"Connecting to {host}:{port}")
    client = RTypeTestClient(host, port)

    try:
        while True:
            print_menu()
            if client.connected:
                print(f"Status: Connected as '{client.player_name}' (ID: {client.player_id})")
            else:
                print("Status: Disconnected")
            choice = input("\nChoice: ").strip()
            if choice == '0':
                if client.connected:
                    client.disconnect_from_server()
                print("Goodbye!")
                break
            elif choice == '1':
                name = input("Player name: ").strip() or "TestPlayer"
                client.connect_to_server(name)
            elif choice == '2':
                client.disconnect_from_server()
            elif choice == '3':
                client.send_ping()
            elif choice == '4':
                print("Game modes: 1=DUO, 2=TRIO, 3=SQUAD")
                mode = int(input("Game mode: ").strip() or "3")
                print("Difficulties: 1=EASY, 2=NORMAL, 3=HARD")
                diff = int(input("Difficulty: ").strip() or "2")
                client.join_lobby(mode, diff)
            elif choice == '5':
                lobby_id = int(input("Lobby ID: ").strip() or "0")
                client.leave_lobby(lobby_id)
            elif choice == '6':
                print("Input flags (hex): UP=0x0001, DOWN=0x0002, LEFT=0x0004, RIGHT=0x0008, SHOOT=0x0010")
                flags_str = input("Flags (hex, e.g. 0x0011 for UP+SHOOT): ").strip() or "0x0000"
                flags = int(flags_str, 16)
                client.send_input(flags)
            elif choice == '7':
                pkt_type = int(input("Packet type (hex, e.g. 0x01): ").strip(), 16)
                hex_payload = input("Payload (hex, e.g. 01 54 65 73 74): ").strip()
                client.send_custom_payload(pkt_type, hex_payload)
            elif choice == '8':
                print(f"\n--- Status ---")
                print(f"Connected: {client.connected}")
                print(f"Player name: {client.player_name}")
                print(f"Player ID: {client.player_id}")
                print(f"Sequence number: {client.sequence_number}")
            else:
                print("[!] Invalid choice")

    except KeyboardInterrupt:
        print("\n\n[!] Interrupted by user")
        if client.connected:
            client.disconnect_from_server()

    finally:
        client.close()

if __name__ == '__main__':
    main()
