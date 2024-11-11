// elements/ImageElement.jsx
import React from "react";

function ImageElement({ width, height }) {
	return (
		<div
			style={{
				width,
				height,
				border: "1px dashed black",
			}}
		/>
	);
}

export default ImageElement;