import serial
import threading
import time
from flask import Flask, render_template
from flask_socketio import SocketIO

SERIAL_PORT = '/dev/tty.usbmodem14201'  
BAUD_RATE = 9600

app = Flask(__name__)
app.config['SECRET_KEY'] = 'mysecretkey!'
socketio = SocketIO(app, async_mode='threading')

# --- Global Değişkenler ---
thread = None
thread_lock = threading.Lock()
# Bağlantı durumunu tutacak olan paylaşılan değişken
connection_status = {'status': 'initializing'} 

def background_thread():
    """Seri portu dinleyen ve veri geldikçe arayüze gönderen arkaplan iş parçacığı."""
    global connection_status

    while True:
        try:
            connection_status['status'] = 'initializing'
            print(f"{SERIAL_PORT} portuna bağlanılmaya çalışılıyor...")
            with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
                print(f"{SERIAL_PORT} portuna bağlanıldı. Veri bekleniyor...")
                last_data_time = time.time()
                
                # Porta bağlanıldı ama henüz veri gelmedi durumunu arayüze bildir
                connection_status['status'] = 'waiting_for_data'
                socketio.emit('connection_status', connection_status)

                while True:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    
                    if line:
                        last_data_time = time.time()
                        # Eğer durum 'veri bekleniyor' veya 'kopuk' ise 'bağlandı' olarak güncelle
                        if connection_status['status'] != 'connected':
                            print("Veri akışı başladı, bağlantı aktif.")
                            connection_status['status'] = 'connected'
                            socketio.emit('connection_status', connection_status)
                        
                        # Gelen veriyi logla ve ayrıştır
                        with open('telemetry_data.txt', 'a', encoding='utf-8') as f:
                            f.write(line + '\n')
                        try:
                            parts = line.split(';')
                            if len(parts) == 5:
                                data = {
                                    'time_ms': int(float(parts[0])),
                                    'speed_kmh': float(parts[1]),
                                    'temp_c': float(parts[2]),
                                    'voltage_v': float(parts[3]),
                                    'energy_wh': float(parts[4]),
                                }
                                socketio.emit('update_data', data)
                            else:
                                print(f"Hatalı parça sayısı: {len(parts)}")
                        except (ValueError, IndexError) as e:
                            print(f"Veri ayrıştırılamadı: {e} -> Gelen veri: '{line}'")
                    
                    # Zaman aşımı kontrolü
                    if connection_status['status'] == 'connected' and (time.time() - last_data_time > 15):
                        print("Zaman aşımı: 15 saniyedir veri gelmiyor, bağlantı kesildi.")
                        connection_status['status'] = 'disconnected'
                        socketio.emit('connection_status', connection_status)
                    
                    socketio.sleep(0.01)

        except serial.SerialException:
            if connection_status['status'] != 'disconnected':
                print(f"Seri port bulunamadı veya bağlantı kesildi.")
                connection_status['status'] = 'disconnected'
                socketio.emit('connection_status', connection_status)
            socketio.sleep(5)
        except Exception as e:
            print(f"Beklenmedik bir hata oluştu: {e}")
            if connection_status['status'] != 'disconnected':
                connection_status['status'] = 'disconnected'
                socketio.emit('connection_status', connection_status)
            socketio.sleep(5)

@app.route('/')
def index():
    return render_template('index.html')

@socketio.on('connect')
def connect():
    """Yeni bir web istemcisi bağlandığında çalışır."""
    global thread
    with thread_lock:
        if thread is None:
            thread = socketio.start_background_task(target=background_thread)
    # Yeni istemciye mevcut bağlantı durumunu hemen gönder
    socketio.emit('connection_status', connection_status)
    print(f'İstemci bağlandı, durum gönderildi: {connection_status['status']}')

if __name__ == '__main__':
    print("Web sunucusu http://127.0.0.1:5000 adresinde başlatılıyor...")
    socketio.run(app, debug=True, use_reloader=False)
