const express = require("express");
const http = require("http");
const { Server } = require("socket.io");
const mqtt = require("mqtt");
const { Client } = require("pg");
const { InfluxDB, Point } = require("@influxdata/influxdb-client");

const MQTT_HOST = process.env.MQTT_HOST || "mosquitto";
const MQTT_PORT = process.env.MQTT_PORT || "1883";
const MQTT_TOPIC = process.env.MQTT_TOPIC || "gps/lora/#";

const POSTGRES_HOST = process.env.POSTGRES_HOST || "postgres";
const POSTGRES_PORT = process.env.POSTGRES_PORT || "5432";
const POSTGRES_DB = process.env.POSTGRES_DB || "gpsdb";
const POSTGRES_USER = process.env.POSTGRES_USER || "gpsuser";
const POSTGRES_PASSWORD = process.env.POSTGRES_PASSWORD || "gpspass";

const INFLUX_URL = process.env.INFLUX_URL || "http://influxdb:8086";
const INFLUX_TOKEN = process.env.INFLUX_TOKEN || "supersecrettoken";
const INFLUX_ORG = process.env.INFLUX_ORG || "gpsorg";
const INFLUX_BUCKET = process.env.INFLUX_BUCKET || "gpsbucket";

const app = express();
const server = http.createServer(app);
const io = new Server(server, { cors: { origin: "*" } });

app.get("/health", (req, res) => {
  res.json({ ok: true, topic: MQTT_TOPIC });
});

const pgClient = new Client({
  host: POSTGRES_HOST,
  port: Number(POSTGRES_PORT),
  database: POSTGRES_DB,
  user: POSTGRES_USER,
  password: POSTGRES_PASSWORD
});

async function initPostgres() {
  await pgClient.connect();
  await pgClient.query(`
    CREATE TABLE IF NOT EXISTS gps_data (
      id SERIAL PRIMARY KEY,
      node_id TEXT,
      lat DOUBLE PRECISION,
      lng DOUBLE PRECISION,
      sat INTEGER,
      status TEXT,
      raw JSONB,
      created_at TIMESTAMP DEFAULT NOW()
    )
  `);
  console.log("PostgreSQL listo");
}

const influx = new InfluxDB({ url: INFLUX_URL, token: INFLUX_TOKEN });
const writeApi = influx.getWriteApi(INFLUX_ORG, INFLUX_BUCKET, "ns");

const mqttUrl = `mqtt://${MQTT_HOST}:${MQTT_PORT}`;
const mc = mqtt.connect(mqttUrl, {
  clientId: "mqtt-bridge-" + Math.random().toString(16).slice(2),
  reconnectPeriod: 2000
});

mc.on("connect", () => {
  console.log("MQTT conectado:", mqttUrl);
  mc.subscribe(MQTT_TOPIC, { qos: 0 }, (err) => {
    if (err) console.error("MQTT subscribe error:", err);
    else console.log("Suscrito a:", MQTT_TOPIC);
  });
});

mc.on("message", async (topic, payload) => {
  const raw = payload.toString("utf8").trim();

  io.emit("gps", { topic, raw, ts: Date.now() });

  try {
    const obj = JSON.parse(raw);

    const nodeId = obj.id || "SIN_ID";
    const lat = obj.lat !== undefined ? Number(obj.lat) : null;
    const lng = obj.lng !== undefined ? Number(obj.lng) : null;
    const sat = obj.sat !== undefined ? Number(obj.sat) : null;
    const status = obj.status || null;

    await pgClient.query(
      `INSERT INTO gps_data (node_id, lat, lng, sat, status, raw)
       VALUES ($1, $2, $3, $4, $5, $6)`,
      [nodeId, lat, lng, sat, status, obj]
    );

    const point = new Point("gps")
      .tag("node_id", nodeId);

    if (lat !== null) point.floatField("lat", lat);
    if (lng !== null) point.floatField("lng", lng);
    if (sat !== null) point.intField("sat", sat);
    if (status !== null) point.stringField("status", status);

    writeApi.writePoint(point);
    writeApi.flush().catch(console.error);

    console.log("Mensaje procesado:", raw);
  } catch (e) {
    console.error("Mensaje no JSON o error al guardar:", e.message);
  }
});

mc.on("error", (e) => console.error("MQTT error:", e.message));

io.on("connection", (socket) => {
  console.log("WebSocket cliente:", socket.id);
  socket.emit("info", { connected: true, topic: MQTT_TOPIC });
});

initPostgres()
  .then(() => {
    server.listen(3000, () => console.log("Bridge listo en :3000"));
  })
  .catch((err) => {
    console.error("Error iniciando PostgreSQL:", err);
    process.exit(1);
  });