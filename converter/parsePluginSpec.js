// converter/parsePluginSpec.js
//
// This is the FIRST piece of the machine.
// It takes raw HTML/JS (like from a CodePen export)
// and tries to extract the @plugin JSON block.
//
// Later, the API + VST builder will call this function.

function parsePluginSpecFromText(text) {
  if (!text || typeof text !== "string") {
    throw new Error("parsePluginSpecFromText: input must be a non-empty string.");
  }

  // 1) Look for a /* @plugin ... @endplugin */ block anywhere in the text.
  const pluginBlockRegex = /\/\*\s*@plugin\s*([\s\S]*?)\s*@endplugin\s*\*\//m;
  const match = text.match(pluginBlockRegex);

  if (!match) {
    throw new Error("No @plugin block found in the provided text.");
  }

  const jsonText = match[1].trim();

  let spec;
  try {
    spec = JSON.parse(jsonText);
  } catch (err) {
    throw new Error("Failed to parse @plugin JSON: " + err.message);
  }

  // Basic sanity checks so we don't pass garbage downstream
  if (!spec.name || typeof spec.name !== "string") {
    throw new Error("Invalid plugin spec: missing 'name' (string).");
  }

  if (!spec.engine || typeof spec.engine !== "string") {
    // default engine to "auto" if not provided
    spec.engine = "auto";
  }

  if (!Array.isArray(spec.params)) {
    throw new Error("Invalid plugin spec: 'params' must be an array.");
  }

  // Normalize each param a bit
  spec.params = spec.params.map((p, idx) => {
    if (!p || typeof p !== "object") {
      throw new Error(`Invalid param at index ${idx}: not an object.`);
    }

    if (!p.id || typeof p.id !== "string") {
      throw new Error(`Invalid param at index ${idx}: missing 'id' (string).`);
    }

    // Fallbacks / defaults
    const normalized = {
      id: p.id,
      label: typeof p.label === "string" ? p.label : p.id,
      type: typeof p.type === "string" ? p.type : "knob",
    };

    if (normalized.type === "enum") {
      if (!Array.isArray(p.options) || p.options.length === 0) {
        throw new Error(`Param '${p.id}' is enum but has no 'options' array.`);
      }
      normalized.options = p.options;
      normalized.default =
        p.default !== undefined ? p.default : p.options[0];
    } else if (normalized.type === "bool") {
      normalized.default = !!p.default;
    } else {
      // Numeric knob/slider
      const min = Number(p.min);
      const max = Number(p.max);
      const def = Number(p.default);

      if (!Number.isFinite(min) || !Number.isFinite(max)) {
        throw new Error(
          `Param '${p.id}' must have numeric 'min' and 'max' for type '${normalized.type}'.`
        );
      }
      normalized.min = min;
      normalized.max = max;
      normalized.default = Number.isFinite(def)
        ? def
        : min + (max - min) * 0.5;
    }

    return normalized;
  });

  return spec;
}

// For Node/CommonJS style require(...)
module.exports = {
  parsePluginSpecFromText,
};
