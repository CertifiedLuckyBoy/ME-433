import math
import random
import sys

import pgzrun

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    serial = None


WIDTH = 900
HEIGHT = 560

GYRO_DEADZONE = 250
GYRO_GAIN = 560.0
MOUSE_X_DIRECTION = -1
MOUSE_Y_DIRECTION = 1
SHOT_COOLDOWN = 10
TARGET_COUNT = 5

ax_raw = 0
ay_raw = 0
az_raw = 16384
gx_raw = 0
gy_raw = 0
gz_raw = 0
button_raw = 0
last_button = 0
serial_status = "no serial"

crosshair_x = WIDTH / 2
crosshair_y = HEIGHT / 2
score = 0
shots = 0
misses = 0
hit_flash = 0
shot_cooldown = 0
ser = None
targets = []


def find_port():
    if serial is None:
        return None

    if len(sys.argv) > 1:
        return sys.argv[1]

    ports = list(serial.tools.list_ports.comports())
    for port in ports:
        name = port.device.lower()
        description = port.description or ""
        manufacturer = port.manufacturer or ""
        text = (description + " " + manufacturer).lower()
        if "usbmodem" in name or "pico" in text or "rp2" in text:
            return port.device

    if ports:
        return ports[0].device

    return None


def open_serial():
    global ser, serial_status

    port = find_port()
    if port is None or serial is None:
        serial_status = "demo mode"
        return

    try:
        ser = serial.Serial(port, 115200, timeout=0)
        serial_status = port
    except serial.SerialException:
        serial_status = "demo mode"


def parse_line(line):
    line = line.strip()
    if line.startswith("(") and line.endswith(")"):
        line = line[1:-1]

    parts = line.split(",")
    if len(parts) != 7:
        return None

    try:
        values = [int(part) for part in parts]
    except ValueError:
        return None

    return values


def read_pico():
    global ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw, button_raw

    if ser is None:
        gx_raw = int(math.sin(score * 0.5 + shots * 0.08) * 1200)
        gy_raw = int(math.cos(score * 0.4 + shots * 0.06) * 1600)
        gz_raw = 0
        ax_raw = int(math.sin(shots * 0.03) * 4000)
        ay_raw = int(math.cos(shots * 0.02) * 4000)
        az_raw = 16384
        button_raw = 1 if keyboard.space else 0
        return

    while ser.in_waiting:
        raw = ser.readline().decode("utf-8", errors="ignore")
        data = parse_line(raw)
        if data is not None:
            ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw, button_raw = data


def clamp(value, low, high):
    return max(low, min(high, value))


def gyro_to_move(value):
    if abs(value) < GYRO_DEADZONE:
        return 0.0
    return value / GYRO_GAIN


def make_target():
    return {
        "x": random.randint(90, WIDTH - 90),
        "y": random.randint(140, HEIGHT - 95),
        "r": random.randint(22, 34),
        "speed": random.uniform(0.6, 1.5),
        "dir": random.choice([-1, 1]),
        "color": random.choice([
            (224, 71, 71),
            (238, 151, 58),
            (210, 196, 74),
        ]),
    }


def reset_targets():
    targets.clear()
    for i in range(TARGET_COUNT):
        targets.append(make_target())


def update_targets():
    for target in targets:
        target["x"] = target["x"] + target["dir"] * target["speed"]
        if target["x"] < 70 or target["x"] > WIDTH - 70:
            target["dir"] = -target["dir"]


def shoot():
    global score, shots, misses, hit_flash

    shots = shots + 1
    best_index = -1
    best_distance = 9999

    for i in range(len(targets)):
        target = targets[i]
        dx = crosshair_x - target["x"]
        dy = crosshair_y - target["y"]
        distance = math.sqrt(dx * dx + dy * dy)
        if distance < best_distance:
            best_distance = distance
            best_index = i

    if best_index >= 0 and best_distance <= targets[best_index]["r"] + 10:
        score = score + 1
        hit_flash = 12
        targets[best_index] = make_target()
    else:
        misses = misses + 1
        hit_flash = 4


def update():
    global crosshair_x, crosshair_y, last_button, hit_flash, shot_cooldown

    read_pico()

    crosshair_x = crosshair_x + MOUSE_X_DIRECTION * gyro_to_move(gy_raw)
    crosshair_y = crosshair_y + MOUSE_Y_DIRECTION * gyro_to_move(gx_raw)
    crosshair_x = clamp(crosshair_x, 22, WIDTH - 22)
    crosshair_y = clamp(crosshair_y, 118, HEIGHT - 22)

    update_targets()

    if shot_cooldown > 0:
        shot_cooldown = shot_cooldown - 1

    if button_raw and not last_button and shot_cooldown == 0:
        shoot()
        shot_cooldown = SHOT_COOLDOWN

    last_button = button_raw

    if hit_flash > 0:
        hit_flash = hit_flash - 1


def draw_background():
    screen.fill((15, 18, 24))
    screen.draw.filled_rect(Rect((0, 0), (WIDTH, 106)), (28, 35, 46))
    screen.draw.filled_rect(Rect((0, HEIGHT - 45), (WIDTH, 45)), (34, 39, 47))

    for x in range(0, WIDTH, 45):
        color = (22, 27, 36)
        screen.draw.line((x, 106), (x, HEIGHT - 45), color)
    for y in range(130, HEIGHT - 45, 45):
        screen.draw.line((0, y), (WIDTH, y), (22, 27, 36))

    screen.draw.text(
        "HW10 MPU6050 Aim Trainer",
        (24, 18),
        fontsize=34,
        color=(244, 247, 252),
    )
    screen.draw.text(
        "gyro controls aim   GP15 button fires",
        (24, 57),
        fontsize=20,
        color=(170, 185, 204),
    )
    screen.draw.text(
        "serial: " + serial_status,
        (24, 82),
        fontsize=17,
        color=(170, 185, 204),
    )

    screen.draw.text(
        "score " + str(score),
        (690, 18),
        fontsize=31,
        color=(255, 214, 92),
    )
    screen.draw.text(
        "shots " + str(shots) + "   miss " + str(misses),
        (690, 56),
        fontsize=20,
        color=(205, 215, 230),
    )


def draw_target(target):
    x = target["x"]
    y = target["y"]
    r = target["r"]

    screen.draw.filled_circle((x, y), r, target["color"])
    screen.draw.circle((x, y), r + 3, (244, 247, 252))
    screen.draw.filled_circle((x, y), 6, (15, 18, 24))
    screen.draw.line((x - r, y), (x + r, y), (15, 18, 24))
    screen.draw.line((x, y - r), (x, y + r), (15, 18, 24))


def draw_crosshair():
    color = (255, 255, 255)
    if hit_flash > 8:
        color = (80, 255, 155)
    elif hit_flash > 0:
        color = (255, 95, 95)

    x = crosshair_x
    y = crosshair_y
    screen.draw.circle((x, y), 13, color)
    screen.draw.line((x - 30, y), (x - 10, y), color)
    screen.draw.line((x + 10, y), (x + 30, y), color)
    screen.draw.line((x, y - 30), (x, y - 10), color)
    screen.draw.line((x, y + 10), (x, y + 30), color)


def draw_sensor_text():
    line1 = "ax " + str(ax_raw) + " ay " + str(ay_raw) + " az " + str(az_raw)
    line2 = "gx " + str(gx_raw) + " gy " + str(gy_raw) + " gz " + str(gz_raw)
    line3 = "button " + str(button_raw)

    screen.draw.text(line1, (24, HEIGHT - 38), fontsize=18, color=(170, 185, 204))
    screen.draw.text(line2, (330, HEIGHT - 38), fontsize=18, color=(170, 185, 204))
    screen.draw.text(line3, (705, HEIGHT - 38), fontsize=18, color=(170, 185, 204))


def draw():
    draw_background()

    for target in targets:
        draw_target(target)

    draw_crosshair()
    draw_sensor_text()


reset_targets()
open_serial()
pgzrun.go()
