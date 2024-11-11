// elements/ButtonElement.jsx
import React from "react";

function ButtonElement({
	text,
	width,
	height,
	radius = 20,
	filled = true,
	level = 1,
}) {
	const getButtonStyle = () => {
		const backgroundColor = filled
			? level === 1
				? "#000000"
				: "#cccccc"
			: "transparent";

		const borderColor = level === 1 ? "#000000" : "#cccccc";

		return {
			width,
			height,
			backgroundColor,
			border: filled ? "none" : `2px solid ${borderColor}`,
			borderRadius: `${radius}px`,
			display: "flex",
			alignItems: "center",
			justifyContent: "center",
			color: filled
				? level === 1
					? "#ffffff"
					: "#000000"
				: level === 1
					? "#000000"
					: "#cccccc",
			fontWeight: level === 1 ? "bold" : "normal",
			padding: "5px 10px",
			boxSizing: "border-box",
			fontSize: "16px",
			position: "relative",
		};
	};

	return <div style={getButtonStyle()}>{text}</div>;
}

export default ButtonElement;