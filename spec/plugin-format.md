# Plugin Description Format (`@plugin` block)

Any CodePen-style audio plugin that wants to be converted to a VST **must** include
a `@plugin` block in a JS comment somewhere in its code:

```js
/* @plugin
{
  "name": "My Plugin Name",
  "engine": "auto",
  "params": [
    {
      "id": "drive",
      "label": "Drive",
      "type": "knob",
      "min": 0,
      "max": 1,
      "default": 0.5
    },
    {
      "id": "mode",
      "label": "Mode",
      "type": "enum",
      "options": ["A", "B", "C"],
      "default": "A"
    },
    {
      "id": "bypass",
      "label": "Bypass",
      "type": "bool",
      "default": false
    }
  ]
}
@endplugin */
