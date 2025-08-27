import serial
import threading
from flask import Flask, render_template
from flask_socketio import SocketIO

SERIAL_PORT = '/dev/tty.usbmodem14201'  
BAUD_RATE = 9600

app = Flask(_name_)

app.config['SECRET_KEY'] = 'mysecretkey!'

socketio = SocketIO(app, async_mode='threading')


thread = None
thread_lock = threading.Lock()

def background_thread():
    while True:
        try:
            print(f"{SERIAL_PORT} portuna bağlanılmaya çalışılıyor...")
            with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
                print(f"{SERIAL_PORT} portuna bağlanıldı. Veri bekleniyor...")
                while True:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        with open('telemetry_data.txt', 'a', encoding='utf-8') as f:
                            f.write(line + '\n')
                        print(f"Ham veri alındı: '{line}'")
                        try:
                            parts = line.split(';')
                            print(f"Parçalar: {parts}")

                            if len(parts) == 5:
                                print("Parça sayısı 5, ayrıştırma denemesi...")
                                data = {
                                    'time_ms': int(float(parts[0])),
                                    'speed_kmh': float(parts[1]),
                                    'temp_c': float(parts[2]),
                                    'voltage_v': float(parts[3]),
                                    'energy_wh': float(parts[4]),
                                }
                                print(f"Ayrıştırma başarılı: {data}")
                                socketio.emit('update_data', data)
                            else:
                                print(f"Hatalı parça sayısı: {len(parts)}")
                        except (ValueError, IndexError) as e:
                            print(f"Veri ayrıştırılamadı: {e} -> Gelen veri: '{line}'")
                    # else:
                    #     print("Boş satır alındı.") 
        except serial.SerialException:
            print(f"Seri port bulunamadı veya bağlantı kesildi. 5 saniye sonra tekrar denenecek.")
            socketio.sleep(5)
        except Exception as e:
            print(f"Beklenmedik bir hata oluştu: {e}")
            socketio.sleep(5)


@app.route('/')
def index():
    return render_template('index.html')

@socketio.on('connect')
def connect():
    global thread
    with thread_lock:
        if thread is None:
            thread = socketio.start_background_task(target=background_thread)
    print('İstemci bağlandı')

if _name_ == '_main_':
    print("Web sunucusu http://127.0.0.1:5000 adresinde başlatılıyor...")
    socketio.run(app, debug=True, use_reloader=False)