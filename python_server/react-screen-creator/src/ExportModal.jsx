import React from "react";
import { Modal, Button } from "antd";
import { calculateAnchorPosition } from './utils';

function ExportModal({ elements, onClose }) {
	const exportData = elements.map((el) => {
		const anchorPosition = calculateAnchorPosition(el);

		const base = {
			id: el.id,
			type: el.type,
			x: Math.round(anchorPosition.x),
			y: Math.round(anchorPosition.y),
			anchor: el.anchor,
			width: el.width,
			height: el.height
		};

		if (el.type === "text") {
			return { ...base, text: el.text, level: el.level };
		} else if (el.type === "button") {
			return {
				...base,
				text: el.text,
				callback: "/api/button/" + el.id,
				filled: true,
				radius: 20,
				padding_x: 10,
				padding_y: 5,
				level: el.level,
			};
		} else if (el.type === "image") {
			return {
				...base,
				text: "/api/images/" + el.id,
				width: el.width,
				height: el.height,
				inverted: false,
			};
		}
		return base;
	});

	return (
		<Modal
			visible
			title="E-ink Display JSON Configuration"
			onCancel={onClose}
			footer={[
				<Button key="close" onClick={onClose}>
					Close
				</Button>,
				<Button
					key="copy"
					type="primary"
					onClick={() => {
						navigator.clipboard.writeText(
							JSON.stringify({ elements: exportData }, null, 2),
						);
					}}
				>
					Copy to Clipboard
				</Button>,
				<Button
					key="save"
					type="primary"
					onClick={() => {
						fetch("http://localhost:5000/layout/save-layout", {
							method: "POST",
							headers: {
								"Content-Type": "application/json",
							},
							body: JSON.stringify(exportData),
						});
					}}
				>
					Save Layout to Server
				</Button>,
			]}
		>
			<pre style={{ maxHeight: 400, overflow: "auto" }}>
				{JSON.stringify({ elements: exportData }, null, 2)}
			</pre>
		</Modal>
	);
}

export default ExportModal;
