import kaboom from "https://cdn.jsdelivr.net/npm/kaboom@3000.1.15/dist/kaboom.mjs";

kaboom({
    width: 360, // Adjust to include border
    height: 380,
    scale: 2,
    background: [0, 0, 0], // Black background
});

// Constants
const GRID_PIXEL_DIMENSION = 320
const GRID_SIZE = 16; // Number of pixels in each row and column
const PIXEL_SIZE = Math.floor(GRID_PIXEL_DIMENSION / GRID_SIZE); // Size of each pixel
const BORDER_WIDTH = 4; // Thickness of the border
const DISPLAY_OFFSET = BORDER_WIDTH; // Offset the grid inside the border

// Function to draw the grid
function drawGrid(pixelString) {
    // Clear all previous content
    destroyAll("pixel");

    // Split string into rows
    const rows = pixelString.split(":");

    rows.forEach((row, rowIndex) => {
        // Iterate through characters in each row
        [...row].forEach((char, colIndex) => {
            // Determine color based on character
            let pixel_color = [0, 0, 0];
            switch (char) {
                case "G": pixel_color = [0, 255, 0]; break; // Green
                case "R": pixel_color = [255, 0, 0]; break; // Red
                case "Y": pixel_color = [255, 255, 0]; break; // Yellow
                default: pixel_color = [0, 0, 0]; break; // Black
            }

            // Add a rectangle for this pixel
            add([
                rect(PIXEL_SIZE, PIXEL_SIZE),
                pos(
                    colIndex * PIXEL_SIZE + DISPLAY_OFFSET + BORDER_WIDTH,
                    rowIndex * PIXEL_SIZE + DISPLAY_OFFSET + BORDER_WIDTH
                ),
                color(pixel_color),
                "pixel"
            ]);
        });
    });
}

// Add a white border
add([
    rect(GRID_SIZE * PIXEL_SIZE + 2 * BORDER_WIDTH, GRID_SIZE * PIXEL_SIZE + 2 * BORDER_WIDTH),
    pos(DISPLAY_OFFSET, DISPLAY_OFFSET),
    color([255, 255, 255]), // White color
]);

// Initial 16x16 black grid
let defaultGrid = "B".repeat(GRID_SIZE).concat(":").repeat(GRID_SIZE - 1) + "B".repeat(GRID_SIZE);
drawGrid(defaultGrid);

// Add instructions
add([
    text("Press ENTER to input a new grid", { size: 16 }),
    pos(10, 340),
    color([255, 255, 255]),
]);

// Listen for "enter" key press
onKeyPress("enter", () => {
    const userInput = prompt(`Enter your ${GRID_SIZE}x${GRID_SIZE} pixel string:`);
    const rows = userInput ? userInput.split(":") : [];
    if (rows.length === GRID_SIZE && rows.every(row => row.length === GRID_SIZE)) {
        drawGrid(userInput);
    } else {
        alert(`Invalid input! Ensure it is in the ${GRID_SIZE}x${GRID_SIZE} format.`);
    }
});

window.addEventListener("message", (event) => {

    console.log(event.data.value);
    if (event.source != parent) {
        console.log("Message from somewhere other than the parent window", event);
        return;
    }

    if(event.data.messageType == "sim2web"){
        var message = event.data.value; // GGGGGGGGGGGYYYYR:RRRRR...
        console.log("sim2web message received: ", message);
        const rows = message ? message.split(":") : [];
        if (rows.length === GRID_SIZE && rows.every(row => row.length === GRID_SIZE)) {
            drawGrid(message);
        } else {
            alert(`Invalid input! Ensure it is in the ${GRID_SIZE}x${GRID_SIZE} format.`);
        }
    }
}, false);