.pragma library

function htmlEscaped(str) {
    str = str.replace(/&/g, "&amp;");
    str = str.replace(/</g, "&lt;");
    str = str.replace(/>/g, "&gt;");
    str = str.replace(/"/g, "&quot;");
    str = str.replace(/'/g, "&apos;");
    return str
}

// QML doesn't provide a way to replace in strings :(
function replace(str, from, to) {
    str = str.replace(from, to);
}