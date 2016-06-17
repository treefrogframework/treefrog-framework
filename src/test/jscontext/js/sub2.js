var sub = require ("./sub");

module.exports = function(message, lang) {
    return (lang == 'ja') ? ('こんにちは ' + message) : sub(message);
};
