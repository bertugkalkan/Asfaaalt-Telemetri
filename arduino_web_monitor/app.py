import serial
import threading
from flask import Flask, render_template
from flask_socketio import SocketIO

# --- Yapılandırma ---
# Arduino'nuzun bağlı olduğu seri portu ve baud rate'i buraya girin.
# Windows'ta "COM3", "COM4" gibi olabilir.
# Linux/macOS'ta "/dev/ttyUSB0", "/dev/tty.usbmodem1411" gibi olabilir.
SERIAL_PORT = '/dev/tty.usbmodem14201'  # KENDİ PORTUNUZA GÖRE DEĞİŞTİRİN
BAUD_RATE = 9600
# --------------------

app = Flask(__name__)
# Gizli anahtar, socketio için gereklidir.
app.config['SECRET_KEY'] = 'mysecretkey!'
# Hata düzeltmesi: eventlet uyumsuzluğunu önlemek için async_mode'u manuel olarak 'threading' yapın.
socketio = SocketIO(app, async_mode='threading')

# Arka planda seri portu dinleyecek thread için global değişken
thread = None
thread_lock = threading.Lock()

def background_thread():
    """
    Arka planda seri porttan veri okur ve istemcilere gönderir.
    Bağlantı koptuğunda yeniden bağlanmayı dener.
    """
    while True:
        try:
            print(f"{SERIAL_PORT} portuna bağlanılmaya çalışılıyor...")
            with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
                print(f"{SERIAL_PORT} portuna bağlanıldı. Veri bekleniyor...")
                while True:
                    line = ser.readline().decode('utf-8').strip()
                    if line:
                        try:
                            parts = line.split(';')
                            if len(parts) == 5:
                                data = {
                                    'time_ms': int(parts[0]),
                                    'speed_kmh': float(parts[1]),
                                    'temp_c': float(parts[2]),
                                    'voltage_v': float(parts[3]),
                                    'energy_wh': float(parts[4]),
                                }
                                # 'update_data' olayı ile veriyi tüm istemcilere gönder
                                socketio.emit('update_data', data)
                        except (UnicodeDecodeError, ValueError, IndexError) as e:
                            print(f"Veri ayrıştırılamadı: {e} -> Gelen veri: '{line}'")
        except serial.SerialException:
            print(f"Seri port bulunamadı veya bağlantı kesildi. 5 saniye sonra tekrar denenecek.")
            socketio.sleep(5)
        except Exception as e:
            print(f"Beklenmedik bir hata oluştu: {e}")
            socketio.sleep(5)


@app.route('/')
def index():
    """Ana sayfayı render eder."""
    return render_template('index.html')

@socketio.on('connect')
def connect():
    """Yeni bir istemci bağlandığında çalışır."""
    global thread
    with thread_lock:
        # Eğer arka plan thread'i çalışmıyorsa başlat
        if thread is None:
            thread = socketio.start_background_task(target=background_thread)
    print('İstemci bağlandı')

if __name__ == '__main__':
    print("Web sunucusu http://127.0.0.1:5000 adresinde başlatılıyor...")
    # Sunucuyu başlat
    socketio.run(app, debug=True, use_reloader=False)
