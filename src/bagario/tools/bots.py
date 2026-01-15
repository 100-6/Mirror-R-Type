#!/usr/bin/env python3
"""
Bagario Bot Script
Creates multiple bots with basic AI that connect to a Bagario server.

Usage:
    python3 bots.py <num_bots> <server_ip>

Example:
    python3 bots.py 5 127.0.0.1

Requirements:
    pip install pyenet

Note: pyenet requires the ENet library to be installed on the system.
    - Ubuntu/Debian: sudo apt install libenet-dev
    - macOS: brew install enet
    - Arch: pacman -S enet
"""

import argparse
import math
import random
import struct
import threading
import time
from dataclasses import dataclass, field
from enum import IntEnum
from typing import Dict, List, Optional, Tuple

try:
    import enet
except ImportError:
    print("Error: pyenet library not found.")
    print("Please install it with: pip install pyenet")
    print("\nYou may also need to install the ENet C library:")
    print("  Ubuntu/Debian: sudo apt install libenet-dev")
    print("  macOS: brew install enet")
    print("  Arch: pacman -S enet")
    exit(1)


# =============================================================================
# PROTOCOL CONSTANTS
# =============================================================================

ENET_PORT = 5002
PROTOCOL_VERSION = 0x01

# ENet channels
CHANNEL_RELIABLE = 0
CHANNEL_UNRELIABLE = 1
NUM_CHANNELS = 2

# Map size (from server config)
MAP_WIDTH = 5000.0
MAP_HEIGHT = 5000.0

# Entity mass constants
STARTING_MASS = 15.0
MIN_SPLIT_MASS = 35.0
MIN_EJECT_MASS = 35.0


class PacketType(IntEnum):
    """Packet type identifiers for the Bagario protocol."""
    # Client -> Server
    CLIENT_CONNECT = 0x01
    CLIENT_DISCONNECT = 0x02
    CLIENT_PING = 0x04
    CLIENT_INPUT = 0x10
    CLIENT_SPLIT = 0x11
    CLIENT_EJECT_MASS = 0x12
    CLIENT_SET_SKIN = 0x13

    # Server -> Client
    SERVER_ACCEPT = 0x81
    SERVER_REJECT = 0x82
    SERVER_PONG = 0x85
    SERVER_SNAPSHOT = 0xA0
    SERVER_ENTITY_SPAWN = 0xB0
    SERVER_ENTITY_DESTROY = 0xB1
    SERVER_PLAYER_EATEN = 0xC0
    SERVER_LEADERBOARD = 0xC1
    SERVER_PLAYER_SKIN = 0xC2


class EntityType(IntEnum):
    """Types of entities in the game."""
    PLAYER_CELL = 1
    FOOD = 2
    VIRUS = 3
    EJECTED_MASS = 4


class DisconnectReason(IntEnum):
    """Reasons for disconnection."""
    USER_QUIT = 0x01
    TIMEOUT = 0x02
    KICKED = 0x03
    SERVER_SHUTDOWN = 0x04


# =============================================================================
# DATA STRUCTURES
# =============================================================================

@dataclass
class Entity:
    """Represents a game entity."""
    entity_id: int
    entity_type: EntityType
    x: float
    y: float
    mass: float
    color: int
    owner_id: int

    @property
    def radius(self) -> float:
        """Calculate radius from mass."""
        return 10.0 * math.sqrt(self.mass / math.pi)


@dataclass
class GameState:
    """Current game state for a bot."""
    player_id: int = 0
    map_width: float = MAP_WIDTH
    map_height: float = MAP_HEIGHT
    entities: Dict[int, Entity] = field(default_factory=dict)
    my_cells: List[Entity] = field(default_factory=list)
    total_mass: float = STARTING_MASS
    is_alive: bool = True
    server_tick: int = 0


# =============================================================================
# BOT AI
# =============================================================================

class BotAI:
    """Simple AI for bot decision making."""

    # AI behavior weights
    FOOD_WEIGHT = 1.0
    SMALLER_PLAYER_WEIGHT = 2.5
    LARGER_PLAYER_THREAT = 3.0
    VIRUS_THREAT = 2.0
    EDGE_AVOIDANCE = 1.5

    # Distance thresholds
    CHASE_DISTANCE = 800.0
    FLEE_DISTANCE = 500.0
    EDGE_MARGIN = 200.0

    def __init__(self, game_state: GameState):
        self.state = game_state
        self.target_x = MAP_WIDTH / 2
        self.target_y = MAP_HEIGHT / 2
        self.last_split_time = 0
        self.last_eject_time = 0

    def get_my_position(self) -> Tuple[float, float]:
        """Get the center of mass of all my cells."""
        if not self.state.my_cells:
            return MAP_WIDTH / 2, MAP_HEIGHT / 2

        total_mass = sum(c.mass for c in self.state.my_cells)
        if total_mass == 0:
            return MAP_WIDTH / 2, MAP_HEIGHT / 2

        cx = sum(c.x * c.mass for c in self.state.my_cells) / total_mass
        cy = sum(c.y * c.mass for c in self.state.my_cells) / total_mass
        return cx, cy

    def get_my_largest_cell(self) -> Optional[Entity]:
        """Get my largest cell."""
        if not self.state.my_cells:
            return None
        return max(self.state.my_cells, key=lambda c: c.mass)

    def distance(self, x1: float, y1: float, x2: float, y2: float) -> float:
        """Calculate Euclidean distance."""
        return math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2)

    def normalize(self, x: float, y: float) -> Tuple[float, float]:
        """Normalize a vector."""
        length = math.sqrt(x * x + y * y)
        if length < 0.001:
            return 0.0, 0.0
        return x / length, y / length

    def can_eat(self, my_mass: float, target_mass: float) -> bool:
        """Check if I can eat the target (need ~25% more mass)."""
        return my_mass > target_mass * 1.25

    def is_threat(self, my_mass: float, target_mass: float) -> bool:
        """Check if the target can eat me."""
        return target_mass > my_mass * 1.25

    def update(self) -> Tuple[float, float, bool, bool]:
        """
        Update AI and return (target_x, target_y, should_split, should_eject).
        """
        my_x, my_y = self.get_my_position()
        largest_cell = self.get_my_largest_cell()

        if not largest_cell:
            # Dead or no cells, move to center
            return MAP_WIDTH / 2, MAP_HEIGHT / 2, False, False

        my_mass = largest_cell.mass
        my_radius = largest_cell.radius

        # Collect all forces
        force_x = 0.0
        force_y = 0.0

        best_food_score = -1
        best_food_target = None

        best_prey_score = -1
        best_prey_target = None

        threats: List[Tuple[float, float, float]] = []  # (x, y, threat_level)

        # Analyze all entities
        for entity in self.state.entities.values():
            if entity.owner_id == self.state.player_id:
                continue  # Skip my own cells

            dist = self.distance(my_x, my_y, entity.x, entity.y)

            if entity.entity_type == EntityType.FOOD:
                # Food is always attractive
                if dist < self.CHASE_DISTANCE:
                    score = (1.0 / max(dist, 1.0)) * self.FOOD_WEIGHT
                    if score > best_food_score:
                        best_food_score = score
                        best_food_target = entity

            elif entity.entity_type == EntityType.PLAYER_CELL:
                if self.can_eat(my_mass, entity.mass):
                    # Prey - chase it!
                    if dist < self.CHASE_DISTANCE:
                        # Bigger prey = more attractive
                        score = (entity.mass / max(dist, 1.0)) * self.SMALLER_PLAYER_WEIGHT
                        if score > best_prey_score:
                            best_prey_score = score
                            best_prey_target = entity

                elif self.is_threat(my_mass, entity.mass):
                    # Threat - run away!
                    if dist < self.FLEE_DISTANCE:
                        threat_level = (entity.mass / my_mass) * self.LARGER_PLAYER_THREAT
                        threats.append((entity.x, entity.y, threat_level / max(dist, 1.0)))

            elif entity.entity_type == EntityType.VIRUS:
                # Viruses are dangerous if I'm big
                if my_mass > 100 and dist < self.FLEE_DISTANCE:
                    threats.append((entity.x, entity.y, self.VIRUS_THREAT / max(dist, 1.0)))

        # Decision: prioritize survival over hunting
        should_split = False
        should_eject = False
        current_time = time.time()

        if threats:
            # Flee from threats - combine all threat vectors
            for tx, ty, level in threats:
                dx = my_x - tx
                dy = my_y - ty
                nx, ny = self.normalize(dx, dy)
                force_x += nx * level * 100
                force_y += ny * level * 100

        elif best_prey_target:
            # Chase prey
            dx = best_prey_target.x - my_x
            dy = best_prey_target.y - my_y
            dist = self.distance(my_x, my_y, best_prey_target.x, best_prey_target.y)
            force_x = dx
            force_y = dy

            # Consider splitting to catch prey
            if (my_mass >= MIN_SPLIT_MASS * 2 and
                dist < my_radius * 4 and
                dist > my_radius * 1.5 and
                self.can_eat(my_mass / 2, best_prey_target.mass) and
                current_time - self.last_split_time > 1.0):
                should_split = True
                self.last_split_time = current_time

        elif best_food_target:
            # Chase food
            dx = best_food_target.x - my_x
            dy = best_food_target.y - my_y
            force_x = dx
            force_y = dy

        else:
            # Wander randomly
            if random.random() < 0.02:  # Change direction occasionally
                angle = random.uniform(0, 2 * math.pi)
                self.target_x = my_x + math.cos(angle) * 300
                self.target_y = my_y + math.sin(angle) * 300

            force_x = self.target_x - my_x
            force_y = self.target_y - my_y

        # Edge avoidance
        if my_x < self.EDGE_MARGIN:
            force_x += (self.EDGE_MARGIN - my_x) * self.EDGE_AVOIDANCE
        elif my_x > MAP_WIDTH - self.EDGE_MARGIN:
            force_x -= (my_x - (MAP_WIDTH - self.EDGE_MARGIN)) * self.EDGE_AVOIDANCE

        if my_y < self.EDGE_MARGIN:
            force_y += (self.EDGE_MARGIN - my_y) * self.EDGE_AVOIDANCE
        elif my_y > MAP_HEIGHT - self.EDGE_MARGIN:
            force_y -= (my_y - (MAP_HEIGHT - self.EDGE_MARGIN)) * self.EDGE_AVOIDANCE

        # Calculate final target position
        target_x = my_x + force_x
        target_y = my_y + force_y

        # Clamp to map bounds
        target_x = max(0, min(MAP_WIDTH, target_x))
        target_y = max(0, min(MAP_HEIGHT, target_y))

        return target_x, target_y, should_split, should_eject


# =============================================================================
# PACKET BUILDERS
# =============================================================================

def pack_connect(player_name: str) -> bytes:
    """Create a CLIENT_CONNECT packet."""
    packet = struct.pack('<BB', PacketType.CLIENT_CONNECT, PROTOCOL_VERSION)
    name_bytes = player_name.encode('utf-8')[:31]
    packet += name_bytes.ljust(32, b'\x00')
    return packet


def pack_disconnect(player_id: int) -> bytes:
    """Create a CLIENT_DISCONNECT packet."""
    return struct.pack('<BIB', PacketType.CLIENT_DISCONNECT, player_id, DisconnectReason.USER_QUIT)


def pack_input(player_id: int, target_x: float, target_y: float, sequence: int) -> bytes:
    """Create a CLIENT_INPUT packet."""
    return struct.pack('<BIffI', PacketType.CLIENT_INPUT, player_id, target_x, target_y, sequence)


def pack_split(player_id: int) -> bytes:
    """Create a CLIENT_SPLIT packet."""
    return struct.pack('<BI', PacketType.CLIENT_SPLIT, player_id)


def pack_eject(player_id: int, direction_x: float, direction_y: float) -> bytes:
    """Create a CLIENT_EJECT_MASS packet."""
    return struct.pack('<BIff', PacketType.CLIENT_EJECT_MASS, player_id, direction_x, direction_y)


# =============================================================================
# PACKET PARSERS
# =============================================================================

def unpack_accept(data: bytes) -> Optional[dict]:
    """Parse a SERVER_ACCEPT packet."""
    if len(data) < 18:
        return None

    player_id, map_width, map_height, start_mass, tick_rate, max_players = struct.unpack(
        '<IfffBB', data[1:19]
    )
    return {
        'player_id': player_id,
        'map_width': map_width,
        'map_height': map_height,
        'starting_mass': start_mass,
        'tick_rate': tick_rate,
        'max_players': max_players
    }


def unpack_reject(data: bytes) -> Optional[dict]:
    """Parse a SERVER_REJECT packet."""
    if len(data) < 66:
        return None

    reason_code = data[1]
    reason_message = data[2:66].decode('utf-8', errors='ignore').rstrip('\x00')
    return {
        'reason_code': reason_code,
        'reason_message': reason_message
    }


def unpack_snapshot(data: bytes) -> Optional[dict]:
    """Parse a SERVER_SNAPSHOT packet."""
    if len(data) < 7:
        return None

    server_tick, entity_count = struct.unpack('<IH', data[1:7])

    entities = []
    for i in range(entity_count):
        offset = 7 + i * 25
        if offset + 25 > len(data):
            break

        entity_id, entity_type = struct.unpack('<IB', data[offset:offset+5])
        pos_x, pos_y, mass, color, owner_id = struct.unpack('<fffII', data[offset+5:offset+25])

        entities.append(Entity(
            entity_id=entity_id,
            entity_type=EntityType(entity_type) if entity_type in [1, 2, 3, 4] else EntityType.FOOD,
            x=pos_x,
            y=pos_y,
            mass=mass,
            color=color,
            owner_id=owner_id
        ))

    return {'tick': server_tick, 'entities': entities}


def unpack_player_eaten(data: bytes) -> Optional[dict]:
    """Parse a SERVER_PLAYER_EATEN packet."""
    if len(data) < 13:
        return None

    player_id, killer_id, final_mass = struct.unpack('<IIf', data[1:13])
    return {
        'player_id': player_id,
        'killer_id': killer_id,
        'final_mass': final_mass
    }


# =============================================================================
# BOT CLASS
# =============================================================================

class BagarioBot:
    """A bot that connects to and plays Bagario using ENet."""

    def __init__(self, name: str, server_ip: str, bot_id: int, port: int = 5002):
        self.name = name
        self.server_ip = server_ip
        self.bot_id = bot_id
        self.port = port

        self.host: Optional[enet.Host] = None
        self.peer: Optional[enet.Peer] = None

        self.game_state = GameState()
        self.ai = BotAI(self.game_state)

        self.running = False
        self.connected = False
        self.sequence = 0

        self.network_thread: Optional[threading.Thread] = None
        self.ai_thread: Optional[threading.Thread] = None

        self._lock = threading.Lock()

    def log(self, message: str):
        """Log a message with bot identifier."""
        print(f"[Bot {self.bot_id} - {self.name}] {message}")

    def connect(self) -> bool:
        """Connect to the server using ENet."""
        try:
            # Create ENet host (client)
            self.host = enet.Host(None, 1, NUM_CHANNELS, 0, 0)

            # Connect to server
            address = enet.Address(self.server_ip.encode('utf-8'), self.port)
            self.peer = self.host.connect(address, NUM_CHANNELS)

            if self.peer is None:
                self.log("Failed to create peer")
                return False

            # Wait for connection (with timeout)
            timeout_ms = 5000
            start_time = time.time()

            while (time.time() - start_time) * 1000 < timeout_ms:
                event = self.host.service(100)

                if event.type == enet.EVENT_TYPE_CONNECT:
                    self.log("ENet connected, sending handshake...")

                    # Send connect packet
                    connect_packet = pack_connect(self.name)
                    packet = enet.Packet(connect_packet, enet.PACKET_FLAG_RELIABLE)
                    self.peer.send(CHANNEL_RELIABLE, packet)
                    self.host.flush()

                elif event.type == enet.EVENT_TYPE_RECEIVE:
                    data = event.packet.data
                    if len(data) > 0:
                        packet_type = data[0]

                        if packet_type == PacketType.SERVER_ACCEPT:
                            accept_data = unpack_accept(data)
                            if accept_data:
                                self.game_state.player_id = accept_data['player_id']
                                self.game_state.map_width = accept_data['map_width']
                                self.game_state.map_height = accept_data['map_height']
                                self.connected = True
                                self.log(f"Connected! Player ID: {self.game_state.player_id}")
                                return True

                        elif packet_type == PacketType.SERVER_REJECT:
                            reject_data = unpack_reject(data)
                            if reject_data:
                                self.log(f"Rejected: {reject_data['reason_message']}")
                            return False

                elif event.type == enet.EVENT_TYPE_DISCONNECT:
                    self.log("Disconnected during handshake")
                    return False

            self.log("Connection timeout")
            return False

        except Exception as e:
            self.log(f"Connection error: {e}")
            return False

    def disconnect(self):
        """Disconnect from the server."""
        if self.connected and self.peer:
            try:
                disconnect_packet = pack_disconnect(self.game_state.player_id)
                packet = enet.Packet(disconnect_packet, enet.PACKET_FLAG_RELIABLE)
                self.peer.send(CHANNEL_RELIABLE, packet)
                self.host.flush()
                self.peer.disconnect()
            except:
                pass

        self.running = False
        self.connected = False

        # Give ENet time to send disconnect
        if self.host:
            try:
                for _ in range(10):
                    self.host.service(50)
            except:
                pass

    def network_loop(self):
        """Thread to handle ENet events."""
        while self.running:
            try:
                with self._lock:
                    if not self.host:
                        break
                    event = self.host.service(10)

                if event.type == enet.EVENT_TYPE_RECEIVE:
                    self.handle_packet(event.packet.data)

                elif event.type == enet.EVENT_TYPE_DISCONNECT:
                    self.log("Disconnected from server")
                    self.running = False
                    break

            except Exception as e:
                if self.running:
                    self.log(f"Network error: {e}")
                break

    def handle_packet(self, data: bytes):
        """Handle a received packet."""
        if not data or len(data) == 0:
            return

        packet_type = data[0]

        if packet_type == PacketType.SERVER_SNAPSHOT:
            snapshot = unpack_snapshot(data)
            if snapshot:
                self.update_game_state(snapshot)

        elif packet_type == PacketType.SERVER_PLAYER_EATEN:
            eaten_data = unpack_player_eaten(data)
            if eaten_data and eaten_data['player_id'] == self.game_state.player_id:
                self.log(f"I was eaten! Final mass: {eaten_data['final_mass']:.1f}")
                self.game_state.is_alive = False

        elif packet_type == PacketType.SERVER_PONG:
            pass  # Could track latency here

    def update_game_state(self, snapshot: dict):
        """Update game state from a snapshot."""
        self.game_state.server_tick = snapshot['tick']
        self.game_state.entities.clear()
        self.game_state.my_cells.clear()

        for entity in snapshot['entities']:
            self.game_state.entities[entity.entity_id] = entity

            if entity.owner_id == self.game_state.player_id:
                if entity.entity_type == EntityType.PLAYER_CELL:
                    self.game_state.my_cells.append(entity)

        # Calculate total mass
        self.game_state.total_mass = sum(c.mass for c in self.game_state.my_cells)

        # Check if we're still alive
        if self.game_state.my_cells:
            self.game_state.is_alive = True

    def send_packet(self, data: bytes, reliable: bool = False):
        """Send a packet to the server."""
        with self._lock:
            if not self.peer or not self.host:
                return

            flags = enet.PACKET_FLAG_RELIABLE if reliable else 0
            channel = CHANNEL_RELIABLE if reliable else CHANNEL_UNRELIABLE
            packet = enet.Packet(data, flags)
            self.peer.send(channel, packet)

    def ai_loop(self):
        """Thread to run AI and send inputs."""
        input_rate = 1.0 / 30.0  # 30 inputs per second

        while self.running:
            try:
                if not self.game_state.is_alive:
                    time.sleep(0.5)
                    continue

                # Get AI decision
                target_x, target_y, should_split, should_eject = self.ai.update()

                # Send movement input (unreliable)
                self.sequence += 1
                input_packet = pack_input(
                    self.game_state.player_id,
                    target_x,
                    target_y,
                    self.sequence
                )
                self.send_packet(input_packet, reliable=False)

                # Send split command (reliable)
                if should_split:
                    split_packet = pack_split(self.game_state.player_id)
                    self.send_packet(split_packet, reliable=True)

                # Send eject command (reliable)
                if should_eject:
                    my_x, my_y = self.ai.get_my_position()
                    dx, dy = self.ai.normalize(target_x - my_x, target_y - my_y)
                    eject_packet = pack_eject(self.game_state.player_id, dx, dy)
                    self.send_packet(eject_packet, reliable=True)

                time.sleep(input_rate)

            except Exception as e:
                if self.running:
                    self.log(f"AI error: {e}")
                time.sleep(0.1)

    def run(self):
        """Start the bot."""
        if not self.connect():
            return

        self.running = True

        # Start threads
        self.network_thread = threading.Thread(target=self.network_loop, daemon=True)
        self.ai_thread = threading.Thread(target=self.ai_loop, daemon=True)

        self.network_thread.start()
        self.ai_thread.start()

        self.log("Bot started!")

    def stop(self):
        """Stop the bot."""
        self.log("Stopping...")
        self.disconnect()


# =============================================================================
# MAIN
# =============================================================================

def generate_bot_name(index: int) -> str:
    """Generate a random bot name."""
    prefixes = ["Bot", "AI", "Robo", "Auto", "Nom", "Munch", "Blob", "Cell"]
    suffixes = ["inator", "3000", "Pro", "Max", "X", "Prime", "Elite", ""]

    prefix = random.choice(prefixes)
    suffix = random.choice(suffixes)

    return f"{prefix}{index}{suffix}"


def main():
    parser = argparse.ArgumentParser(
        description="Bagario Bot Script - Creates multiple bots that connect to a server",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    python3 bots.py 5 127.0.0.1     # Create 5 bots connecting to localhost
    python3 bots.py 10 192.168.1.50 # Create 10 bots connecting to a LAN server

The bots have basic AI that will:
    - Chase and eat food
    - Chase smaller players
    - Flee from larger players
    - Avoid viruses when big
    - Stay within map bounds
    - Split to catch prey when advantageous

Requirements:
    pip install pyenet

System dependencies:
    Ubuntu/Debian: sudo apt install libenet-dev
    macOS: brew install enet
    Arch: pacman -S enet
        """
    )

    parser.add_argument(
        "num_bots",
        type=int,
        help="Number of bots to spawn"
    )

    parser.add_argument(
        "server_ip",
        type=str,
        help="Server IP address to connect to"
    )

    parser.add_argument(
        "--delay",
        type=float,
        default=0.5,
        help="Delay between spawning each bot (default: 0.5 seconds)"
    )

    parser.add_argument(
        "--port",
        type=int,
        default=5002,
        help="Server port (default: 5002)"
    )

    args = parser.parse_args()

    if args.num_bots < 1:
        print("Error: Number of bots must be at least 1")
        return 1

    if args.num_bots > 50:
        print("Warning: Server max players is 50, some bots may be rejected")

    port = args.port

    print(f"=== Bagario Bot Script ===")
    print(f"Spawning {args.num_bots} bots connecting to {args.server_ip}:{port}")
    print(f"Press Ctrl+C to stop all bots\n")

    bots: List[BagarioBot] = []

    try:
        # Spawn bots with delay
        for i in range(args.num_bots):
            name = generate_bot_name(i + 1)
            bot = BagarioBot(name, args.server_ip, i + 1, port)
            bots.append(bot)
            bot.run()

            if i < args.num_bots - 1:
                time.sleep(args.delay)

        print(f"\nAll {len(bots)} bots spawned! Press Ctrl+C to stop.\n")

        # Keep main thread alive and periodically report status
        while True:
            time.sleep(5.0)

            alive_count = sum(1 for b in bots if b.game_state.is_alive and b.running)
            connected_count = sum(1 for b in bots if b.connected)
            total_mass = sum(b.game_state.total_mass for b in bots if b.running)

            print(f"[Status] Connected: {connected_count}/{len(bots)}, "
                  f"Alive: {alive_count}/{len(bots)}, "
                  f"Total Mass: {total_mass:.0f}")

    except KeyboardInterrupt:
        print("\n\nStopping all bots...")

    finally:
        for bot in bots:
            bot.stop()

        print("All bots stopped. Goodbye!")

    return 0


if __name__ == "__main__":
    exit(main())
