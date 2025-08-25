# Telemetri Web Aray 31 z 31f

Arduino'dan gelen `zaman_ms;hiz_kmh;T_bat_C;V_bat_C;kalan_enerji_Wh` formatl 31 veriyi Flask + Socket.IO ile ger 317ek zamanl 31 g 31f6rselle 35ftirir. 5 saniyeden uzun s 31fcre veri gelmezse "Ba 31flant 31 yok" g 31sterir.

## Ba 31fl 31l 31klar

- Python 3.10+ (pip mevcut)
- Paketler: `flask`, `flask-socketio`, `eventlet`, `pyserial`

> Not: Bu ortamda sanal ortam (venv) kurulam 31yorsa `pip3 install --break-system-packages -r requirements.txt` kullan 31l 31r.

## Kurulum

```bash
pip3 install --break-system-packages -r requirements.txt
```

## 37al 31 35ft 31rma (Mock Mod)

Mock jenerat 36r 36fc ile sahte veri ak 31 35f 31r:

```bash
python3 app.py
```

Taray 31c 31dan a 35fa 31fdaki adrese gidin:

- http://localhost:5000

## Ger 317ek Seri Port ile 37al 31 35ft 31rma

A 35fa 31fdaki ortam de 31fi 35fkenleri ile 37al 31 35ft 31r 31n. `MOCK=0` seri okuma a 37ar.

```bash
export SERIAL_PORT=/dev/ttyUSB0
export SERIAL_BAUD=115200
export MOCK=0
python3 app.py
```

## Yap 31

- `app.py`: Flask + Socket.IO sunucusu, seri okuyucu ve watchdog
- `templates/index.html`: Aray 35fcz
- `static/app.js`: Socket.IO istemcisi, Chart.js grafikleri
- `static/styles.css`: Modern sade tema

## Durum Mant 31 1f 31r

- Sunucu son veri zaman 31n 31 izler, 5 sn'den uzun kesinti olursa `status: connected=false` yay 31nlar.
- 37d6ny 35fcz, son gelen veriye g 31re ayr 31ca 1 saniyede bir yerel watchdog uygular.