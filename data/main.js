(function () {
    'use strict';

    var ip = location.hostname;
    var ipButton = document.getElementById("ipButton");
    var ipText = document.getElementById("ipText");
    try {
        var connection = new WebSocket('ws://' + ip + ':81/', ['arduino']);
    } catch (e) {
        errorLog();
    }
    ipText.value = ip;

    ipButton.addEventListener("click", function () {
        if (connection) {
            connection.close();
            errorLog();
        }
        var ip = (ipText.value === "") ? location.hostname : ipText.value;
        removeError();
        connection = new WebSocket('ws://' + ip + ':81/', ['arduino']);
    });

    connection.onopen = function () {
        connection.send('Prisijungta ' + new Date());
        console.log('Prisijungta ' + new Date())
        removeError();
    };
    connection.onerror = function (error) {
        console.log('WebSocket Error ', error);
        errorLog();
    };
    connection.onmessage = function (e) {
        try {
            var doc = JSON.parse(e.data);
            if (doc.servo || doc.servo == 0) {
                //TODO
                var elements = document.querySelectorAll("#servo" + doc.servo + " input");
                for (var i = 0; i < elements.length; i++) {
                    elements[i].value = doc.position;
                }
            }
        } catch {
            console.log('Serverio duomenys neisparsina');
        }
        console.log('Server: ', e.data);
    };

    function errorLog() {
        ipText.style.border = "1px solid #f00";
    }

    function removeError() {
        ipText.style.border = "1px solid #000";
    }

    function sendValue(value) {
        console.log(value);
        if (connection.readyState === connection.CLOSED) {
            errorLog();
        } else {
            connection.send(JSON.stringify(value));
        }
    }


    var servoSliders = document.querySelectorAll('input[type=range]');
    var syntheticEvent = new WheelEvent("syntheticWheel", {
        "deltaX": 4,
        "deltaMode": 0
    });

    function getValue(that) {
        if (that.max < +that.value) {
            that.value = that.max;
        }
        if (that.min > +that.value) {
            that.value = that.min;
        }
        return that.value;
    }

    for (var i = 0; i < servoSliders.length; ++i) {
        servoSliders[i].addEventListener("input", function () {
            var parentId = this.parentElement.id;
            var servoNr = parentId[parentId.length - 1];
            this.nextElementSibling.value = this.value;
            sendValue({
                servo: servoNr,
                position: this.value
            });
        });

        servoSliders[i].nextElementSibling.addEventListener("input", function () {
            var parentId = this.parentElement.id;
            this.value = getValue(this);
            this.previousElementSibling.value = this.value;
            var servoNr = parentId[parentId.length - 1];
            sendValue({
                servo: servoNr,
                position: this.value
            });
        });


        servoSliders[i].nextElementSibling.addEventListener("mousewheel", function (e) {
            var counter = e.wheelDelta > 0 ? 1 : -1;
            this.value = +this.value + counter;
            this.value = getValue(this);
            this.previousElementSibling.value = this.value;
            var parentId = this.parentElement.id;
            var servoNr = parentId[parentId.length - 1];
            sendValue({
                servo: servoNr,
                position: this.value
            });
        });

        servoSliders[i].addEventListener("mousewheel", function (e) {
            var counter = e.wheelDelta > 0 ? 1 : -1;
            this.value = +this.value + counter;
            var parentId = this.parentElement.id;
            var servoNr = parentId[parentId.length - 1];
            this.nextElementSibling.value = this.value;
            sendValue({
                servo: servoNr,
                position: this.value
            });
            e.preventDefault();
        }, false);
    }
})()