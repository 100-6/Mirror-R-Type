#!/usr/bin/env python3
"""
R-Type Room Management Test Client
Test client for custom room functionality
"""

import socket
import struct
import time
import sys
import hashlib
from typing import Optional

class RTypeRoomTestClient:
    """Test client for R-Type custom rooms"""

    # Client -> Server
    CLIENT_CONNECT = 0x01
    CLIENT_DISCONNECT = 0x02
    CLIENT_PING = 0x04
    CLIENT_JOIN_LOBBY = 0x05
    CLIENT_LEAVE_LOBBY = 0x06
    CLIENT_INPUT = 0x10

    # Room Management (Client -> Server)
    CLIENT_CREATE_ROOM = 0x20
    CLIENT_JOIN_ROOM = 0x21
    CLIENT_LEAVE_ROOM = 0x22
    CLIENT_REQUEST_ROOM_LIST = 0x23
    CLIENT_START_GAME = 0x24

    # Server -> Client
    SERVER_ACCEPT = 0x81
    SERVER_REJECT = 0x82
    SERVER_PLAYER_JOINED = 0x83
    SERVER_PLAYER_LEFT = 0x84
    SERVER_PONG = 0x85
    SERVER_LOBBY_STATE = 0x87

    # Room Management (Server -> Client)
    SERVER_ROOM_CREATED = 0x90
    SERVER_ROOM_LIST = 0x91
    SERVER_ROOM_JOINED = 0x92
    SERVER_ROOM_LEFT = 0x93
    SERVER_ROOM_STATE_UPDATE = 0x94
    SERVER_ROOM_ERROR = 0x95

    def __init__(self, host: str = 'localhost', port: int = 4242):
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(2.0)
        self.server_address = (host, port)
        self.player_id: Optional[int] = None
        self.player_name: str = ""
        self.sequence_number: int = 1
        self.connected: bool = False
        self.current_room_id: Optional[int] = None

    def connect_socket(self):
        """Connect TCP socket to server"""
        try:
            self.sock.connect(self.server_address)
            print(f"[✓] TCP socket connected to {self.host}:{self.port}")
        except Exception as e:
            print(f"[✗] Failed to connect socket: {e}")
            sys.exit(1)

    def build_header(self, packet_type: int, payload_length: int) -> bytes:
        """Build packet header (8 bytes)"""
        return struct.pack('!BBHI',
            0x01,  # Protocol version
            packet_type,
            payload_length,
            self.sequence_number
        )

    def send_packet(self, packet_type: int, payload: bytes):
        """Send a packet to the server via TCP"""
        header = self.build_header(packet_type, len(payload))
        packet = header + payload

        print(f"[→] Sending packet type 0x{packet_type:02X}, size: {len(packet)} bytes")
        try:
            self.sock.sendall(packet)
            self.sequence_number += 1
        except Exception as e:
            print(f"[✗] Failed to send packet: {e}")

    def receive_packet(self, timeout: float = 2.0) -> Optional[tuple]:
        """Receive a packet from the server"""
        old_timeout = self.sock.gettimeout()
        self.sock.settimeout(timeout)

        try:
            # Read header (8 bytes)
            header_data = self.sock.recv(8)
            if len(header_data) < 8:
                print(f"[!] Incomplete header: {len(header_data)} bytes")
                return None

            version, pkt_type, payload_len, seq_num = struct.unpack('!BBHI', header_data)

            # Read payload
            payload = b''
            while len(payload) < payload_len:
                chunk = self.sock.recv(payload_len - len(payload))
                if not chunk:
                    break
                payload += chunk

            self.sock.settimeout(old_timeout)
            print(f"[←] Received packet type 0x{pkt_type:02X}, payload: {len(payload)} bytes")
            return (version, pkt_type, payload_len, seq_num, payload)
        except socket.timeout:
            self.sock.settimeout(old_timeout)
            print("[!] Timeout waiting for response")
            return None
        except Exception as e:
            self.sock.settimeout(old_timeout)
            print(f"[!] Error receiving packet: {e}")
            return None

    def hash_password(self, password: str) -> str:
        """Hash password using SHA256"""
        if not password:
            return ""
        return hashlib.sha256(password.encode('utf-8')).hexdigest()

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
                return True
            elif pkt_type == self.SERVER_REJECT:
                reason_code = payload[0]
                reason_msg = payload[1:65].decode('utf-8').rstrip('\x00')
                print(f"[✗] Connection rejected! Code: {reason_code}, Message: {reason_msg}")
                return False
        return False

    def create_room(self, room_name: str = "", password: str = "",
                    game_mode: int = 3, difficulty: int = 2, map_id: int = 0):
        """Send CLIENT_CREATE_ROOM packet"""
        if not self.connected:
            print("[!] Not connected to server")
            return False

        # Hash password if provided
        password_hash = self.hash_password(password)

        # Build payload
        room_name_bytes = room_name.encode('utf-8')[:32].ljust(32, b'\x00')
        password_hash_bytes = password_hash.encode('utf-8')[:64].ljust(64, b'\x00')

        payload = struct.pack('!I', self.player_id)
        payload += room_name_bytes
        payload += password_hash_bytes
        payload += struct.pack('!BBH', game_mode, difficulty, map_id)

        game_modes = {1: "DUO", 2: "TRIO", 3: "SQUAD"}
        difficulties = {1: "EASY", 2: "NORMAL", 3: "HARD"}

        print(f"\n[*] Creating room:")
        print(f"    Name: '{room_name or 'Auto-generated'}'")
        print(f"    Mode: {game_modes.get(game_mode, '?')}")
        print(f"    Difficulty: {difficulties.get(difficulty, '?')}")
        print(f"    Private: {'Yes' if password else 'No'}")

        self.send_packet(self.CLIENT_CREATE_ROOM, payload)
        response = self.receive_packet()

        if response:
            version, pkt_type, payload_len, seq_num, payload = response
            if pkt_type == self.SERVER_ROOM_CREATED:
                room_id = struct.unpack('!I', payload[:4])[0]
                created_name = payload[4:36].decode('utf-8').rstrip('\x00')
                self.current_room_id = room_id
                print(f"[✓] Room created successfully!")
                print(f"    Room ID: {room_id}")
                print(f"    Room Name: '{created_name}'")
                return True
            elif pkt_type == self.SERVER_ROOM_ERROR:
                error_code = payload[0]
                error_msg = payload[1:65].decode('utf-8').rstrip('\x00')
                print(f"[✗] Room creation failed!")
                print(f"    Error code: {error_code}")
                print(f"    Message: {error_msg}")
                return False
        return False

    def request_room_list(self):
        """Send CLIENT_REQUEST_ROOM_LIST packet"""
        if not self.connected:
            print("[!] Not connected to server")
            return

        print(f"\n[*] Requesting room list...")
        self.send_packet(self.CLIENT_REQUEST_ROOM_LIST, b'')
        response = self.receive_packet()

        if response:
            version, pkt_type, payload_len, seq_num, payload = response
            if pkt_type == self.SERVER_ROOM_LIST:
                room_count = struct.unpack('!H', payload[:2])[0]
                print(f"[✓] Received room list: {room_count} rooms")

                offset = 2
                for i in range(room_count):
                    if offset + 44 > len(payload):
                        break

                    room_data = payload[offset:offset+44]
                    room_id = struct.unpack('!I', room_data[0:4])[0]
                    room_name = room_data[4:36].decode('utf-8').rstrip('\x00')
                    game_mode, difficulty = struct.unpack('!BB', room_data[36:38])
                    current_players, max_players = struct.unpack('!BB', room_data[38:40])
                    map_id = struct.unpack('!H', room_data[40:42])[0]
                    status, is_private = struct.unpack('!BB', room_data[42:44])

                    game_modes = {1: "DUO", 2: "TRIO", 3: "SQUAD"}
                    difficulties = {1: "EASY", 2: "NORMAL", 3: "HARD"}
                    statuses = {1: "WAITING", 2: "IN_PROGRESS", 3: "FINISHED"}

                    print(f"\n  Room #{room_id}: '{room_name}'")
                    print(f"    Mode: {game_modes.get(game_mode, '?')}")
                    print(f"    Difficulty: {difficulties.get(difficulty, '?')}")
                    print(f"    Players: {current_players}/{max_players}")
                    print(f"    Map: {map_id}")
                    print(f"    Status: {statuses.get(status, '?')}")
                    print(f"    Private: {'Yes' if is_private else 'No'}")

                    offset += 44

    def join_room(self, room_id: int, password: str = ""):
        """Send CLIENT_JOIN_ROOM packet"""
        if not self.connected:
            print("[!] Not connected to server")
            return False

        password_hash = self.hash_password(password)
        password_hash_bytes = password_hash.encode('utf-8')[:64].ljust(64, b'\x00')

        payload = struct.pack('!II', self.player_id, room_id)
        payload += password_hash_bytes

        print(f"\n[*] Joining room {room_id}...")
        self.send_packet(self.CLIENT_JOIN_ROOM, payload)
        response = self.receive_packet()

        if response:
            version, pkt_type, payload_len, seq_num, payload = response
            if pkt_type == self.SERVER_ROOM_JOINED:
                joined_room_id = struct.unpack('!I', payload[:4])[0]
                self.current_room_id = joined_room_id
                print(f"[✓] Joined room {joined_room_id} successfully!")
                return True
            elif pkt_type == self.SERVER_ROOM_ERROR:
                error_code = payload[0]
                error_msg = payload[1:65].decode('utf-8').rstrip('\x00')
                print(f"[✗] Failed to join room!")
                print(f"    Error code: {error_code}")
                print(f"    Message: {error_msg}")
                return False
        return False

    def leave_room(self):
        """Send CLIENT_LEAVE_ROOM packet"""
        if not self.connected:
            print("[!] Not connected to server")
            return

        if self.current_room_id is None:
            print("[!] Not in any room")
            return

        payload = struct.pack('!II', self.player_id, self.current_room_id)

        print(f"\n[*] Leaving room {self.current_room_id}...")
        self.send_packet(self.CLIENT_LEAVE_ROOM, payload)
        self.current_room_id = None
        print("[✓] Left room")

    def close(self):
        """Close the socket"""
        self.sock.close()


def print_menu():
    """Print interactive menu"""
    print("\n" + "="*60)
    print("R-Type Room Management Test Client")
    print("="*60)
    print("1. Connect to server")
    print("2. Create room")
    print("3. List all rooms")
    print("4. Join room")
    print("5. Leave room")
    print("6. Show status")
    print("0. Quit")
    print("="*60)


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

    print(f"Server: {host}:{port}")
    client = RTypeRoomTestClient(host, port)
    client.connect_socket()

    try:
        while True:
            print_menu()
            if client.connected:
                status = f"Connected as '{client.player_name}' (ID: {client.player_id})"
                if client.current_room_id:
                    status += f" - In room #{client.current_room_id}"
                print(f"Status: {status}")
            else:
                print("Status: Disconnected")

            choice = input("\nChoice: ").strip()

            if choice == '0':
                print("Goodbye!")
                break
            elif choice == '1':
                name = input("Player name: ").strip() or "TestPlayer"
                client.connect_to_server(name)
            elif choice == '2':
                room_name = input("Room name (leave empty for auto): ").strip()
                password = input("Password (leave empty for public): ").strip()
                print("Game modes: 1=DUO, 2=TRIO, 3=SQUAD")
                mode = int(input("Game mode [3]: ").strip() or "3")
                print("Difficulties: 1=EASY, 2=NORMAL, 3=HARD")
                diff = int(input("Difficulty [2]: ").strip() or "2")
                map_id = int(input("Map ID [0]: ").strip() or "0")
                client.create_room(room_name, password, mode, diff, map_id)
            elif choice == '3':
                client.request_room_list()
            elif choice == '4':
                room_id = int(input("Room ID: ").strip())
                password = input("Password (if private): ").strip()
                client.join_room(room_id, password)
            elif choice == '5':
                client.leave_room()
            elif choice == '6':
                print(f"\n--- Status ---")
                print(f"Connected: {client.connected}")
                print(f"Player name: {client.player_name}")
                print(f"Player ID: {client.player_id}")
                print(f"Current room: {client.current_room_id or 'None'}")
            else:
                print("[!] Invalid choice")

    except KeyboardInterrupt:
        print("\n\n[!] Interrupted by user")

    finally:
        client.close()


if __name__ == '__main__':
    main()
