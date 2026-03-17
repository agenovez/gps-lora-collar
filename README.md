# 🛰️ GPS LoRa Collar System with MQTT, PostgreSQL & InfluxDB

Sistema IoT para monitoreo de ubicación en tiempo real utilizando **ESP32 + GPS + LoRa**, con respaldo vía **WiFi/MQTT**, almacenamiento histórico en **PostgreSQL e InfluxDB**, y visualización en un **dashboard web con mapa interactivo**.

---

## 📌 Descripción del Proyecto

Este proyecto implementa una solución integral para rastreo de activos (ej. ganado) mediante:

* 📡 **Transmisión primaria:** LoRa 433 MHz (larga distancia, bajo consumo)
* 🌐 **Respaldo secundario:** WiFi + MQTT
* 🗄️ **Persistencia de datos:**

  * PostgreSQL (histórico estructurado)
  * InfluxDB (series de tiempo)
* 🖥️ **Visualización:** Dashboard web en tiempo real con Leaflet + WebSockets

---

## 🏗️ Arquitectura del Sistema

```
[ ESP32 TX (GPS + LoRa) ]
          │
          ▼
     (LoRa 433 MHz)
          │
          ▼
[ ESP32 RX Gateway ]
          │
          ▼
        MQTT
          │
          ▼
 [ Docker MQTT Bridge ]
      │           │
      ▼           ▼
 PostgreSQL    InfluxDB
      │
      ▼
 Dashboard Web (Leaflet + Socket.IO)
```

---

## 📂 Estructura del Repositorio

```
gps-lora-collar/
├── tx/                 # Código del collar (transmisor)
├── rx/                 # Código del gateway receptor
└── docker/             # Infraestructura backend
    ├── docker-compose.yml
    ├── mosquitto/
    ├── bridge/
    └── web/
```

---

## ⚙️ Requisitos

### Hardware

* ESP32-WROOM-32
* Módulo LoRa SX1278 (433 MHz)
* GPS NEO-6M
* Pantalla OLED SSD1306 (opcional en RX)
* Antenas LoRa adecuadas (433 MHz)

### Software

* Arduino IDE / PlatformIO
* Docker + Docker Compose
* Node.js (para desarrollo opcional)

---

## 🚀 Despliegue del Backend

```bash
cd docker
docker compose up -d --build
```

---

## 🌐 Accesos

| Servicio      | URL / Puerto           |
| ------------- | ---------------------- |
| Dashboard Web | http://IP:8080         |
| MQTT Broker   | puerto 1883            |
| Bridge API    | http://IP:13000/health |
| InfluxDB      | http://IP:8086         |
| PostgreSQL    | puerto 15432           |

---

## 🔌 Configuración MQTT

### Ver mensajes en tiempo real

```bash
docker exec -it mosquitto mosquitto_sub -t gps/lora/# -v
```

Ejemplo:

```
gps/lora/VACA_01 {"id":"VACA_01","lat":-2.90,"lng":-79.00,"sat":8}
```

---

## 🗄️ Base de Datos

### PostgreSQL

```sql
SELECT * FROM gps_data ORDER BY created_at DESC LIMIT 10;
```

### InfluxDB (Flux)

```flux
from(bucket: "gpsbucket")
 |> range(start: -1h)
```

---

## 📡 Flujo de Datos

1. El **TX (collar)** obtiene coordenadas GPS
2. Envía datos vía **LoRa**
3. El **RX Gateway** recibe y publica en MQTT
4. El **Bridge Docker**:

   * Guarda en PostgreSQL
   * Guarda en InfluxDB
   * Envía a WebSocket
5. El **Dashboard** muestra en tiempo real

---

## 📊 Formato del Payload

```json
{
  "id": "VACA_01",
  "lat": -2.897123,
  "lng": -79.004567,
  "sat": 8
}
```

Sin señal GPS:

```json
{
  "id": "VACA_01",
  "status": "NO_FIX"
}
```

---

## 🔒 Características Técnicas

* Comunicación LoRa optimizada:

  * SF7
  * BW 125 kHz
  * CR 4/5
* CRC habilitado
* MQTT con reconexión automática
* Persistencia híbrida (SQL + TimeSeries)
* Dashboard en tiempo real

---

## ⚠️ Problemas Comunes

### ❌ Error: port already allocated

```bash
docker ps
```

Cambiar puertos en `docker-compose.yml`

---

### ❌ No llegan datos MQTT

* Verificar IP del broker en RX
* Revisar logs:

```bash
docker logs -f mqtt-bridge
```

---

### ❌ Sin FIX GPS

* Verificar antena GPS
* Ubicación con cielo abierto

---

## 🔧 Mejoras Futuras

* Geocercas (Geofencing)
* Alertas por salida de zona
* Integración con Grafana
* Panel multi-nodo avanzado
* Optimización energética (deep sleep)

---

## 📚 Referencias

* MQTT Essentials – HiveMQ
* ESP32 IoT Projects (Packt)
* Hands-On IoT with MQTT (Packt)
* NMEA GPS Protocol

---

## 👨‍💻 Autores

**Carlos Andrés Genovez Tobar**
**Mauricio Xavier Loor Garcia**
**Erick Sebastian Parra Ulloa**

---

## 📜 Licencia

MIT License

---

## ⭐ Notas Finales

Este proyecto está diseñado para:

* Implementaciones reales en campo (ISP / IoT rural)
* Proyectos académicos (tesis, prototipos)
* Sistemas de monitoreo de activos en zonas remotas

---
