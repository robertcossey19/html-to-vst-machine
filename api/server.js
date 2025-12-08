// api/server.js
//
// Tiny HTTP API for the machine.
// Later, Render will run this. You do NOT have to run it locally.
//
// POST /analyze
// Body: raw HTML/JS text (e.g. CodePen export)
// Response: JSON plugin spec (parsed @plugin block)

const express = require("express");
const { parsePluginSpecFromText } = require("../converter/parsePluginSpec");

const app = express();

// Accept raw text in the body (CodePen HTML/JS)
app.use(express.text({ type: "*/*", limit: "1mb" }));

app.post("/analyze", (req, res) => {
  const text = req.body;

  if (!text || typeof text !== "string" || !text.trim()) {
    return res.status(400).json({
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
  // This log matters only when running somewhere (Render, local, etc.)
  console.log(`API listening on port ${PORT}`);
});
