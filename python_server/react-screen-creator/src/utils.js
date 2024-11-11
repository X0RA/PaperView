
// utils.js
const ACTUAL_WIDTH = 960;
const ACTUAL_HEIGHT = 540;
const DISPLAY_WIDTH = ACTUAL_WIDTH - 330; // 630
const DISPLAY_HEIGHT = ACTUAL_HEIGHT - 180; // 360

// Scale factor between display and actual coordinates
export const SCALE_X = ACTUAL_WIDTH / DISPLAY_WIDTH;
export const SCALE_Y = ACTUAL_HEIGHT / DISPLAY_HEIGHT;


export const calculateAnchorPosition = (element) => {
    const elementHeight = element.height || 50;
    const elementWidth = element.width || 100;

    // Convert display coordinates to actual coordinates
    const convertX = (x) => (x * ACTUAL_WIDTH) / DISPLAY_WIDTH;
    const convertY = (y) => (y * ACTUAL_HEIGHT) / DISPLAY_HEIGHT;

    switch (element.anchor) {
        case "tl": // Top Left
            return {
                x: convertX(element.x),
                y: convertY(element.y)
            };
        case "tm": // Top Middle
            return {
                x: convertX(element.x + elementWidth / 2),
                y: convertY(element.y)
            };
        case "tr": // Top Right
            return {
                x: convertX(element.x + elementWidth),
                y: convertY(element.y)
            };
        case "bl": // Bottom Left
            return {
                x: convertX(element.x),
                y: convertY(element.y + elementHeight)
            };
        case "bm": // Bottom Middle
            return {
                x: convertX(element.x + elementWidth / 2),
                y: convertY(element.y + elementHeight)
            };
        case "br": // Bottom Right
            return {
                x: convertX(element.x + elementWidth),
                y: convertY(element.y + elementHeight)
            };
        case "m": // Middle
            return {
                x: convertX(element.x + elementWidth / 2),
                y: convertY(element.y + elementHeight / 2)
            };
        default:
            return {
                x: convertX(element.x),
                y: convertY(element.y)
            };
    }
};

export const calculateDisplayPosition = (element) => {
    const elementHeight = element.height || 50;
    const elementWidth = element.width || 100;

    console.log("calculatedisplaypos" + elementWidth, elementHeight)

    // Convert actual coordinates to display coordinates
    const convertX = (x) => (x * DISPLAY_WIDTH) / ACTUAL_WIDTH;
    const convertY = (y) => (y * DISPLAY_HEIGHT) / ACTUAL_HEIGHT;

    switch (element.anchor) {
        case "tl": // Top Left
            return {
                x: convertX(element.x),
                y: convertY(element.y)
            };
        case "tm": // Top Middle
            return {
                x: convertX(element.x) - elementWidth / 2,
                y: convertY(element.y)
            };
        case "tr": // Top Right
            return {
                x: convertX(element.x) - elementWidth,
                y: convertY(element.y)
            };
        case "bl": // Bottom Left
            return {
                x: convertX(element.x),
                y: convertY(element.y) - elementHeight
            };
        case "bm": // Bottom Middle
            return {
                x: convertX(element.x) - elementWidth / 2,
                y: convertY(element.y) - elementHeight
            };
        case "br": // Bottom Right
            return {
                x: convertX(element.x) - elementWidth,
                y: convertY(element.y) - elementHeight
            };
        case "m": // Middle
            return {
                x: convertX(element.x) - elementWidth / 2,
                y: convertY(element.y) - elementHeight / 2
            };
        default:
            return {
                x: convertX(element.x),
                y: convertY(element.y)
            };
    }
};