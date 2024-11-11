// elements/TextElement.jsx
import React from "react";
import { WHITE_DISPLAY } from "../constants";

function TextElement({ text, level = 1 }) {
  const getTextColor = (level) => {
    return WHITE_DISPLAY.textColors[level];
  };

  return (
    <div
      style={{
        fontSize: "24px",
        color: getTextColor(level),
        fontWeight: level === 1 ? "bold" : "normal",
        whiteSpace: 'nowrap',
        padding: "0px",
      }}
    >
      {text}
    </div>
  );
}

export default TextElement;