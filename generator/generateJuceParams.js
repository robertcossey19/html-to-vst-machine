// generator/generateJuceParams.js
//
// Reads generator/currentSpec.json and generates
// builder/juce-plugin/Source/GeneratedParams.h
//
// This does NOT change DSP yet; it just emits a C++ param spec
// that Stage 2 (the JUCE build) can use.

const fs = require("fs");
const path = require("path");

function main() {
  const specPath = path.join(__dirname, "currentSpec.json");
  const outHeaderPath = path.join(
    __dirname,
    "..",
    "builder",
    "juce-plugin",
    "Source",
    "GeneratedParams.h"
  );

  if (!fs.existsSync(specPath)) {
    throw new Error(
      `Spec file not found: ${specPath}. Make sure generator/currentSpec.json exists.`
    );
  }

  const raw = fs.readFileSync(specPath, "utf8").trim();
  if (!raw) {
    throw new Error("Spec file is empty.");
  }

  let spec;
  try {
    spec = JSON.parse(raw);
  } catch (err) {
    console.error("Failed to parse currentSpec.json as JSON.");
    throw err;
  }

  const name = spec.name || "Unnamed Plugin";
  const engine = spec.engine || "auto";
  const params = Array.isArray(spec.params) ? spec.params : [];

  console.log(`Generating JUCE params for "${name}" (engine: ${engine})`);
  console.log(`Found ${params.length} parameter(s).`);

  // Build C++ header
  let header = "";
  header += "// ===================================================================\n";
  header += "//  AUTO-GENERATED FILE — DO NOT EDIT BY HAND\n";
  header += "//  Generated from generator/currentSpec.json\n";
  header += "//  Plugin: " + name + "\n";
  header += "//  Engine: " + engine + "\n";
  header += "// ===================================================================\n";
  header += "\n";
  header += "#pragma once\n\n";
  header += "#include <cstddef>\n\n";
  header += "struct HtmlToVstParamSpec {\n";
  header += "    const char* id;\n";
  header += "    const char* label;\n";
  header += "    const char* type;   // \"knob\", \"enum\", \"bool\", ...\n";
  header += "    double minValue;\n";
  header += "    double maxValue;\n";
  header += "    double defaultValue;\n";
  header += "};\n\n";

  header += "static constexpr HtmlToVstParamSpec kHtmlToVstParams[] = {\n";

  params.forEach((p, idx) => {
    const id = String(p.id || "").replace(/"/g, '\\"');
    const label = String(p.label || id).replace(/"/g, '\\"');
    const type = String(p.type || "knob").replace(/"/g, '\\"');

    let min = 0.0;
    let max = 1.0;
    let d = 0.0;

    if (type === "knob" || type === "slider") {
      if (typeof p.min === "number") min = p.min;
      if (typeof p.max === "number") max = p.max;
      if (typeof p.default === "number") d = p.default;
    } else if (type === "bool") {
      min = 0.0;
      max = 1.0;
      d = p.default ? 1.0 : 0.0;
    } else if (type === "enum") {
      // Map enums to 0..N-1 index space for now
      const opts = Array.isArray(p.options) ? p.options : [];
      min = 0.0;
      max = opts.length > 0 ? opts.length - 1 : 0.0;
      const defIdx = opts.length > 0 ? opts.indexOf(p.default) : -1;
      d = defIdx >= 0 ? defIdx : 0.0;
    } else {
      // Fallback
      if (typeof p.min === "number") min = p.min;
      if (typeof p.max === "number") max = p.max;
      if (typeof p.default === "number") d = p.default;
    }

    header += `    { "${id}", "${label}", "${type}", ${min}, ${max}, ${d} }`;
    header += idx === params.length - 1 ? "\n" : ",\n";
  });

  header += "};\n\n";
  header += "static constexpr std::size_t kNumHtmlToVstParams = sizeof(kHtmlToVstParams) / sizeof(HtmlToVstParamSpec);\n";

  fs.mkdirSync(path.dirname(outHeaderPath), { recursive: true });
  fs.writeFileSync(outHeaderPath, header, "utf8");

  console.log(`Wrote ${outHeaderPath}`);
}

try {
  main();
} catch (err) {
  console.error(err);
  process.exit(1);
}
