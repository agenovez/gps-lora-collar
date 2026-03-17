Levantar Docker

cd docker

docker compose up -d --build

Ver mensajes MQTT recibidos

docker exec -it mosquitto mosquitto\_sub -t gps/lora/# -v

Ver logs del bridge

docker logs -f mqtt-bridge

Abrir dashboard

http://IP\_DEL\_SERVIDOR:8080

InfluxDB

http://IP\_DEL\_SERVIDOR:8086



usuario: admin



clave: adminadmin



PostgreSQL



host: IP\_DEL\_SERVIDOR



puerto: 15432



db: gpsdb



usuario: gpsuser



clave: gpspass

