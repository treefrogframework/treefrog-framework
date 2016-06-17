var Hello = (function () {
    function Hello(n) {
        this.name = n;
    }
    Hello.prototype.hello = function () {
        return 'My name is ' + this.name;
    };
    return Hello;
}());
