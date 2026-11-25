const fs = require("node:fs");
const path = require("node:path");

const shell = fs.readFileSync(path.join(__dirname, "../../cmake/emscripten/shell.html"), "utf8");
function script(name) {
    const match = shell.match(new RegExp(`<script id="opende-${name}">([\\s\\S]*?)</script>`));
    if (!match) throw new Error(`Missing shell script: ${name}`);
    return match[1];
}
module.exports = { shell, script };
