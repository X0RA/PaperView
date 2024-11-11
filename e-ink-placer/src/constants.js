// constants.js
export const WHITE_DISPLAY = {
  backgroundColor: "#ffffff",
  textColors: {
    1: "#000000", // Primary (Black)
    2: "#666666", // Secondary (Dark grey)
    3: "#999999", // Tertiary (Medium grey)
    4: "#cccccc", // Quaternary (Light grey)
  },
};

export const BLACK_DISPLAY = {
  backgroundColor: "#000000",
  textColors: {
    1: "#ffffff", // Primary (White)
    2: "#cccccc", // Secondary (Light grey)
    3: "#999999", // Tertiary (Medium grey)
    4: "#666666", // Quaternary (Dark grey)
  },
};

export const calculateAnchoredPosition = (element, containerWidth, containerHeight) => {
  const { x, y, width, height, anchor } = element;
  let newX = x;
  let newY = y;

  switch (anchor) {
    case "tr":
      newX = x - width;
      break;
    case "tm":
      newX = x - width / 2;
      break;
    case "tl":
      // Default position
      break;
    case "bl":
      newY = y - height;
      break;
    case "bm":
      newX = x - width / 2;
      newY = y - height;
      break;
    case "br":
      newX = x - width;
      newY = y - height;
      break;
    case "m":
      newX = x - width / 2;
      newY = y - height / 2;
      break;
  }

  return { x: newX, y: newY };
};
