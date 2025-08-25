from __future__ import annotations

import os
import time
import threading
from collections import deque
from dataclasses import dataclass
from typing import Deque, Optional, Dict, Any

from flask import Flask, render_template, request
from flask_socketio import SocketIO

try:
	import serial  # pyserial
except Exception:
	serial = None  # type: ignore


@dataclass
class Telemetry:
	milliseconds: int
	speed_kmh: float
	temp_batt_c: float
	voltage_batt_v: float
	energy_remaining_wh: float

	@staticmethod
	def parse(line: str) -> Optional["Telemetry"]:
		# Expected format: zaman_ms;hiz_kmh;T_bat_C;V_bat_C;kalan_enerji_Wh
		parts = [p.strip() for p in line.strip().split(";")]
		if len(parts) != 5:
			return None
		try:
			ms = int(float(parts[0]))
			speed = float(parts[1])
			t_bat = float(parts[2])
			v_bat = float(parts[3])
			energy_wh = float(parts[4])
			return Telemetry(ms, speed, t_bat, v_bat, energy_wh)
		except ValueError:
			return None


def create_app() -> Flask:
	app = Flask(__name__)
	app.config["SECRET_KEY"] = os.environ.get("SECRET_KEY", "dev-secret")
	return app


app = create_app()
socketio = SocketIO(app, cors_allowed_origins="*")


telemetry_buffer: Deque[Telemetry] = deque(maxlen=300)  # ~5 minutes at 1 Hz
last_data_time = 0.0
status_lock = threading.Lock()


def emit_status(is_connected: bool) -> None:
	status: Dict[str, Any] = {"connected": is_connected, "ts": time.time()}
	socketio.emit("status", status)


def serial_reader(port: str, baudrate: int) -> None:
	global last_data_time
	if serial is None:
		print("pyserial not installed; serial reader disabled")
		return
	try:
		with serial.Serial(port, baudrate=baudrate, timeout=1) as ser:  # type: ignore[attr-defined]
			print(f"[serial] Opened {port} @ {baudrate}")
			while True:
				line_bytes = ser.readline()
				if not line_bytes:
					time.sleep(0.01)
					continue
				try:
					line = line_bytes.decode("utf-8", errors="ignore")
				except Exception:
					continue
				tele = Telemetry.parse(line)
				if tele is None:
					continue
				telemetry_buffer.append(tele)
				with status_lock:
					last_data_time = time.time()
				socketio.emit("telemetry", tele.__dict__)
	except Exception as exc:
		print(f"[serial] Error: {exc}")


def mock_generator() -> None:
	global last_data_time
	print("[mock] Generating demo telemetry. Set MOCK=0 to disable.")
	start = time.time()
	ms = 0
	while True:
		elapsed = time.time() - start
		# Generate simple waveforms for demo
		speed = 30 + 15 * __import__("math").sin(elapsed / 2)
		t_bat = 35 + 3 * __import__("math").sin(elapsed / 5)
		v_bat = 48 + 2 * __import__("math").cos(elapsed / 3)
		energy_wh = max(0.0, 500.0 - 5.0 * elapsed)
		tele = Telemetry(ms, speed, t_bat, v_bat, energy_wh)
		telemetry_buffer.append(tele)
		with status_lock:
			last_data_time = time.time()
		socketio.emit("telemetry", tele.__dict__)
		time.sleep(1.0)
		ms += 1000


def watchdog_thread() -> None:
	# Emits connection status based on last_data_time
	was_connected = False
	while True:
		with status_lock:
			is_connected = (time.time() - last_data_time) < 5.0
		if is_connected != was_connected:
			emit_status(is_connected)
			was_connected = is_connected
		time.sleep(0.5)


@app.route("/")
def index():
	return render_template("index.html")


@socketio.on("connect")
def handle_connect():
	# On connect, send buffered history
	history = [t.__dict__ for t in telemetry_buffer]
	socketio.emit("history", history)
	with status_lock:
		is_connected = (time.time() - last_data_time) < 5.0
	emit_status(is_connected)


if __name__ == "__main__":
	port = os.environ.get("SERIAL_PORT")
	baud = int(os.environ.get("SERIAL_BAUD", "115200"))
	mock = os.environ.get("MOCK", "1") not in ("0", "false", "False")

	threading.Thread(target=watchdog_thread, daemon=True).start()

	if port and not mock:
		threading.Thread(target=serial_reader, args=(port, baud), daemon=True).start()
	else:
		threading.Thread(target=mock_generator, daemon=True).start()

	bind_host = os.environ.get("HOST", "0.0.0.0")
	bind_port = int(os.environ.get("PORT", "5000"))
	socketio.run(app, host=bind_host, port=bind_port)