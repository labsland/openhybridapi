function processSerialText(serialText) {
    var config = window.CONFIG.SIMULATION_CONFIG;
    var outputs = config.serial.dut2sim.outputs;      // e.g., [2, 3]
    var latch = config.serial.dut2sim.latch;          // e.g., 0
    var pulse = config.serial.dut2sim.pulse;          // e.g., 1
    var numPulses = config.serial.dut2sim.num_pulses; // e.g., 256

    var blockSize = outputs.length;                // e.g., 2
    var expectedLength = numPulses * blockSize;
    var timeBetweenPulses = 100;

    // a) Validate the length of serialText.
    if (serialText.length !== expectedLength) {
        alert("serialText should be " + expectedLength + " characters long, but got " + serialText.length);
        return;
    }

    // Helper function: delay execution for a specified time.
    function delay(ms, callback) {
        setTimeout(callback, ms);
    }

    // Function to process each block (chunk) of the serialText.
    function processBlock(chunkIndex) {
        if (chunkIndex >= serialText.length) {
            // All blocks processed.
            return;
        }

        // Extract a chunk of length equal to the number of outputs.
        var chunk = serialText.substr(chunkIndex, blockSize);

        // Function to process each output for the current chunk.
        function processOutput(outputIndex) {
            if (outputIndex >= outputs.length) {
                // After setting all outputs, send the pulse.
                $.ajax({
                    url: window.BASE_URL + "/gpios/dut2sim/" + pulse,
                    method: "POST",
                    dataType: "json",
                    data: JSON.stringify({ value: "1" }),
                    success: function () {
                        delay(timeBetweenPulses, function () {
                            $.ajax({
                                url: window.BASE_URL + "/gpios/dut2sim/" + pulse,
                                method: "POST",
                                dataType: "json",
                                data: JSON.stringify({ value: "0" }),
                                success: function () {
                                    delay(timeBetweenPulses, function () {
                                        processBlock(chunkIndex + blockSize);
                                    });
                                }
                            });
                        });
                    }
                });
                return;
            }

            // Send the output value for the current output.
            var output = outputs[outputIndex];
            var value = (chunk.charAt(outputIndex) === "1");
            $.ajax({
                url: window.BASE_URL + "/gpios/dut2sim/" + output,
                method: "POST",
                dataType: "json",
                data: JSON.stringify({ value: value }),
                success: function () {
                    // Process the next output in the chunk.
                    processOutput(outputIndex + 1);
                }
            });
        }

        // Start processing outputs for this chunk.
        processOutput(0);
    }

    // First, toggle the latch signal before processing the serialText.
    $.ajax({
        url: window.BASE_URL + "/gpios/dut2sim/" + latch,
        method: "POST",
        dataType: "json",
        data: JSON.stringify({ value: "1" }),
        success: function () {
            delay(timeBetweenPulses, function () {
                $.ajax({
                    url: window.BASE_URL + "/gpios/dut2sim/" + latch,
                    method: "POST",
                    dataType: "json",
                    data: JSON.stringify({ value: "0" }),
                    success: function () {
                        delay(timeBetweenPulses, function () {
                            // Now start processing the serialText blocks.
                            processBlock(0);
                        });
                    }
                });
            });
        }
    });
}


function dut2SimSendSerial(serialText) {
    var $latch;
    var $pulse;
    var $outputs = {}
    for (var i = 0; i < window.CONFIG.SIMULATION_CONFIG.gpios.dut2sim.labels.length; i++) {
        var gpio = window.CONFIG.SIMULATION_CONFIG.gpios.dut2sim.labels[i].trim();
        if (gpio === "latch") {
            $latch = $("#gpios-dut2sim-" + i);
            console.log("$latch is: " + "#gpios-dut2sim-" + i + "; and there are: " + $latch.length);
        } else if (gpio === "pulse") {
            $pulse = $("#gpios-dut2sim-" + i);
            console.log("$pulse is: " + "#gpios-dut2sim-" + i + "; and there are: " + $pulse.length);
        } else {
            $outputs[gpio] = $("#gpios-dut2sim-" + i);
            console.log("" + gpio + " is: " + "#gpios-dut2sim-" + i + "; and there are: " + $outputs[gpio].length);
        }
    }

    $pulse.prop("checked", false).trigger("change"); 
    console.log("pulse=0");
    setTimeout(function () { 
        $latch.prop("checked", true).trigger("change"); 
        console.log("latch=1");
    }, 50); // 50 ms
    setTimeout(function () { 
        $latch.prop("checked", false).trigger("change"); 
        console.log("latch=0");
    }, 500); // 100 ms

    var timeAfterLatch = 300;
    var numberOfOutputs = Object.keys($outputs).length;

    for (var i = 0; i < window.CONFIG.SIMULATION_CONFIG.serial.dut2sim.num_pulses; i++) {
        var timeBetween = 150; // ms
        let outputCount = 0;
        for (const outputName in $outputs) {
            let $output = $outputs[outputName];
            let currentChar = serialText.charAt(i * numberOfOutputs + outputCount);
            let checked = currentChar === '1' ? true : false;
            console.log(i, numberOfOutputs, outputCount, currentChar);
            setTimeout(function () {
                $output.prop("checked", checked).trigger("change"); 
                console.log("Change " + $output.attr("id") + " to " + checked);
            }, timeAfterLatch + i * timeBetween + 15);
            console.log(i, ": ", outputName, ": ", checked, "; output: ", $output);

            outputCount += 1;
        }
        setTimeout(function () { $pulse.prop("checked", true).trigger("change"); }, timeAfterLatch + i * timeBetween + 30); // after putting the value, communicate that there is a new value
        setTimeout(function () { $pulse.prop("checked", false).trigger("change"); }, timeAfterLatch + i * timeBetween + 60); // some time later, finish and in the next you will show the next value
    }
}

function requestSim2DutGpios() {
    $.get(window.BASE_URL + "/gpios/sim2dut/?previous_response=" + window.SIM2DUT_STATUS).done(function (data) {
        window.SIM2DUT_STATUS = data.value;

        for (var i = 0; i < data.value.length; i++) {
            if (data.value[i] == "1") {
                $("#gpios-sim2dut-" + i).prop('checked', true);
                $("#gpios-sim2dut-" + i + "-label").text("1");
            } else if (data.value[i] == "0") {
                $("#gpios-sim2dut-" + i).prop('checked', false);
                $("#gpios-sim2dut-" + i + "-label").text("0");

            }
        }
        requestSim2DutGpios();
    });
}

function requestDut2SimGpios() {
    $.get(window.BASE_URL + "/gpios/dut2sim/?previous_response=" + window.DUT2SIM_STATUS).done(function (data) {
        window.DUT2SIM_STATUS = data.value;

        for (var i = 0; i < data.value.length; i++) {
            if (data.value[i] == "1") {
                $("#gpios-dut2sim-" + i).prop('checked', true);
            } else if (data.value[i] == "0") {
                $("#gpios-dut2sim-" + i).prop('checked', false);

            }
            $("#gpios-dut2sim-" + i).trigger("change");
        }

        requestDut2SimGpios();
    });
}

function requestWeb2SimMessages() {
    $.get(window.BASE_URL + "/messages/web2sim/?previous_response=" + encodeURIComponent(window.WEB2SIM_STATUS)).done(function (data) {
        window.WEB2SIM_STATUS = data.value;
        $("#web2sim-messages").val(data.value);
        $("#web2sim-messages").trigger('input');
        requestWeb2SimMessages();
    });
}

function requestSim2WebMessages() {
    $.get(window.BASE_URL + "/messages/sim2web/?previous_response=" + encodeURIComponent(window.SIM2WEB_STATUS)).done(function (data) {
        if (window.SIM2WEB_STATUS !== data.value) {
            console.log("Sending data with postMessage");
            $("#simulation-iframe")[0].contentWindow.postMessage({
                messageType: "sim2web",
                version: "1.0",
                value: data.value
            }, "*");
        }
        window.SIM2WEB_STATUS = data.value;
        $("#sim2web-messages").text(data.value);
        requestSim2WebMessages();
    });
}


function initializeGpios() {
    $(".gpio-dut2sim").each(function () {
        var $checkbox = $(this).find("input");
        var $label = $(this).find("label");
        var number = $checkbox.data("number");

        $checkbox.on("change", function() {
            var currentValue;
            if (this.checked) {
                $label.text("1");
                currentValue = "1";
            } else {
                $label.text("0");
                currentValue = "0";
            }
            if (currentValue != window.DUT2SIM_STATUS[number]) {
                $.ajax({
                    'url': window.BASE_URL + "/gpios/dut2sim/" + number,
                    'method': 'POST',
                    'dataType': 'json',
                    'data': JSON.stringify({value: currentValue == "1"})
                });
            }
        });
    });

    requestDut2SimGpios();
    requestSim2DutGpios();
}

function initializeMessages() {
    $("#web2sim-messages-button").click(function() {
        $.ajax({
            'url': window.BASE_URL + "/messages/web2sim/",
            'method': 'POST',
            'dataType': 'json',
            'data': JSON.stringify({
                value: $("#web2sim-messages").val() 
            })
        });
    });


    console.debug("Added event listener for messages from iframe");

    /**
     * Listen for incoming messages from the visualization iframe.
     */
    window.addEventListener("message", (event) => {

        console.debug("Received message from iframe: ", event.data);

        var simulationIframe = $("#simulation-iframe")[0];
        if (event.data.messageType !== "web2sim") {
            return;
        }

        const expectedOrigin = new URL(simulationIframe.src).origin;

        if (event.origin !== expectedOrigin) {
            console.log("Message from somewhere other than the iframe", event);
            return;
        }

        $.ajax({
            'url': window.BASE_URL + "/messages/web2sim/",
            'method': 'POST',
            'dataType': 'json',
            'data': JSON.stringify({
                value: event.data.value
            })
        });
    }, false);
    

    requestWeb2SimMessages();
    requestSim2WebMessages();
}

function loadSimulation() {
    window.sim2dut = "";
    initializeGpios();
    initializeMessages();
}
