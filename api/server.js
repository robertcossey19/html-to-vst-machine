// api/server.js
//
// Tiny HTTP API for the machine.
// Render runs this. You do NOT have to run it locally.
//
// POST /analyze
// Body: raw HTML/JS text (e.g. CodePen export)
// Response: JSON plugin spec (parsed @plugin block)

const express = require("express");
const { parsePluginSpecFromText } = require("../converter/parsePluginSpec");

const app = express();

// --- CORS so browser-based tester can talk to this API ---
app.use((req, res, next) => {
  res.setHeader("Access-Control-Allow-Origin", "*");
  res.setHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  res.setHeader("Access-Control-Allow-Headers", "Content-Type");
  if (req.method === "OPTIONS") {
    return res.sendStatus(204);
  }
  next();
});

// Accept raw text in the body (CodePen HTML/JS)
app.use(express.text({ type: "*/*", limit: "1mb" }));

app.post("/analyze", (req, res) => {
  const text = req.body;

  if (!text || typeof text !== "string" || !text.trim()) {
    return res.status(400).json({
      ok: false,
      error: "Empty body. Send HTML/JS text for analysis."
    });
  }

  try {
    const spec = parsePluginSpecFromText(text);
    return res.json({
      ok: true,
      spec
    });
  } catch (err) {
    return res.status(400).json({
      ok: false,
      error: err.message
    });
  }
});

// Simple health check
app.get("/", (_req, res) => {
  res.send("html-to-vst-machine API is alive. POST /analyze with plugin HTML.");
});

// Start server (Render will hit `npm start` -> this runs)
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`API listening on port ${PORT}`);
});
