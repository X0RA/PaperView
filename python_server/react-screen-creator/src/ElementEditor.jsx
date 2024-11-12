// ElementEditor.jsx
import React from 'react';
import { Card, Select, Space, InputNumber, Row, Col } from 'antd';
import { WHITE_DISPLAY } from './constants';
import { calculateAnchorPosition, calculateDisplayPosition } from './utils';

const { Option } = Select;

function ElementEditor({ element, updateElement, disabled = false }) {
	// Ensure we have defaults if not set
	const currentAnchor = element.anchor || "tl";
	const currentLevel = element.level || 1;

	const anchorPosition = calculateAnchorPosition(element);

	const anchorOptions = [
		{ value: "tl", label: "Top Left" },
		{ value: "tm", label: "Top Middle" },
		{ value: "tr", label: "Top Right" },
		{ value: "bl", label: "Bottom Left" },
		{ value: "bm", label: "Bottom Middle" },
		{ value: "br", label: "Bottom Right" },
		{ value: "m", label: "Middle" },
	];

	const handleAnchorChange = (value) => {
		updateElement(element.id, { anchor: value });
	};

	const handleLevelChange = (value) => {
		updateElement(element.id, { level: value });
	};

	const handlePositionChange = (key, value) => {
		updateElement(element.id, { [key]: value });
	};

	return (
		<Card
			title={
				disabled
					? "No Element Selected"
					: `Edit ${element.type.charAt(0).toUpperCase() + element.type.slice(1)}`
			}
			style={{
				marginBottom: "16px",
				opacity: disabled ? 0.5 : 1,
				pointerEvents: disabled ? "none" : "auto",
			}}
			size="small"
		>
			<Space direction="vertical" style={{ width: "100%" }} size="middle">
				<Row gutter={12}>
					<Col>
						<InputNumber
							prefix="X:"
							value={Math.round(anchorPosition.x)}
							onChange={(value) => {
								const newPos = calculateDisplayPosition({ ...element, x: value });
								handlePositionChange('x', newPos.x);
							}}
							style={{ width: '80px' }}
						/>
					</Col>
					<Col>
						<InputNumber
							prefix="Y:"
							value={Math.round(anchorPosition.y)}
							onChange={(value) => {
								const newPos = calculateDisplayPosition({ ...element, y: value });
								handlePositionChange('y', newPos.y);
							}}
							style={{ width: '80px' }}
						/>
					</Col>
					<Col>
						<Select
							style={{ width: "120px" }}
							value={currentAnchor}
							onChange={handleAnchorChange}
						>
							{anchorOptions.map((option) => (
								<Option key={option.value} value={option.value}>
									{option.label}
								</Option>
							))}
						</Select>
					</Col>
					<Col>
						<Select
							style={{ width: "120px" }}
							value={currentLevel}
							onChange={handleLevelChange}
							disabled={element.type !== "text" && element.type !== "button"}
						>
							{[1, 2, 3, 4].map((level) => (
								<Option key={level} value={level}>
									<div
										style={{
											color: WHITE_DISPLAY.textColors[level],
											fontWeight: level === 1 ? "bold" : "normal",
										}}
									>
										Level {level}
									</div>
								</Option>
							))}
						</Select>
					</Col>
					<Col>
						<Select
							style={{ width: "120px" }}
							value={element.filled !== false}
							onChange={(value) => updateElement(element.id, { filled: value })}
							disabled={element.type !== "button"}
						>
							<Option value={true}>Filled</Option>
							<Option value={false}>Outlined</Option>
						</Select>
					</Col>
				</Row>

				<Row gutter={12}>
					<Col>
						<InputNumber
							prefix="R:"
							value={element.radius || 20}
							min={0}
							max={35}
							onChange={(value) => updateElement(element.id, { radius: value })}
							style={{ width: '80px' }}
							disabled={element.type !== "button"}
						/>
					</Col>
					<Col>
						<InputNumber
							prefix="PX:"
							value={element.padding_x || 10}
							min={0}
							max={100}
							onChange={(value) => updateElement(element.id, { padding_x: value })}
							style={{ width: '80px' }}
							disabled={element.type !== "button"}
						/>
					</Col>
					<Col>
						<InputNumber
							prefix="PY:"
							value={element.padding_y || 5}
							min={0}
							max={50}
							onChange={(value) => updateElement(element.id, { padding_y: value })}
							style={{ width: '80px' }}
							disabled={element.type !== "button"}
						/>
					</Col>
				</Row>
			</Space>
		</Card>
	);
}

export default ElementEditor;