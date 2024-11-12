// App.jsx
import React, { useState, useRef, useEffect } from "react";
import Canvas from "./Canvas";
import Toolbar from "./Toolbar";
import ElementEditor from "./ElementEditor"; // New component
import ExportModal from "./ExportModal";
import { Button, Layout, Typography } from "antd";

import { calculateAnchorPosition, calculateDisplayPosition } from "./utils";

const { Header, Content, Footer } = Layout;
const { Title } = Typography;

const API_BASE_URL = 'http://localhost:5000';

function App() {
	const [elements, setElements] = useState([]);
	const [showExportModal, setShowExportModal] = useState(false);
	const [selectedElement, setSelectedElement] = useState(null);
	const elementIdCounter = useRef(1);

	const clearLayout = () => {
		return new Promise((resolve) => {
			elementIdCounter.current = 1;
			setElements([]);
				resolve();
		});
	};

	const fetchLayout = async () => {
		try {
			const response = await fetch(`${API_BASE_URL}/layout/get-layout`);
			if (!response.ok) {
				throw new Error('Failed to fetch layout');
			}
			const data = await response.json();

				await clearLayout();

			// Add IDs and convert positions to display coordinates
			const elementsWithIds = data.layout.elements.map((element, index) => {
				// Create a copy of the element with id
				const elementWithId = {
					...element,
					id: index + 1,
					// Ensure these properties exist with defaults
					width: element.width || 100,
					height: element.height || 50,
					anchor: element.anchor || 'tl',
				};

				// Calculate display position
				const displayPos = calculateDisplayPosition(elementWithId);

				return {
					...elementWithId,
					x: displayPos.x,
					y: displayPos.y
				};
			});

			setElements(elementsWithIds);
			elementIdCounter.current = Math.max(...elementsWithIds.map(el => el.id)) + 1;
		} catch (error) {
			console.error('Error fetching layout:', error);
		}
	};

	const addElement = (element) => {
		const newElement = {
			...element,
			id: elementIdCounter.current,
		};
		elementIdCounter.current += 1;
		setElements([...elements, newElement]);
	};

	const updateElement = (id, newProps) => {
		setElements(
			elements.map((el) => {
				if (el.id === id) {
					const updatedElement = { ...el, ...newProps };
					if (selectedElement?.id === id) {
						setSelectedElement(updatedElement);
					}
					return updatedElement;
				}
				return el;
			}),
		);
	};

	const deleteElement = (id) => {
		setElements(elements.filter((el) => el.id !== id));
		if (selectedElement?.id === id) {
			setSelectedElement(null);
		}
		elementIdCounter.current -= 1;
	};

	const handleElementSelect = (element) => {
		setSelectedElement(element);
	};

	const handleCanvasClick = (e) => {
		if (e.target === e.currentTarget) {
			setSelectedElement(null);
		}
	};

	return (
		<Layout style={{ minHeight: "100vh", background: "#f5f5f5" }}>
			<Header
				style={{
					background: "#1a1a1a",
					padding: "0 24px",
					display: "flex",
					alignItems: "center",
					boxShadow: "0 2px 8px rgba(0,0,0,0.15)",
				}}
			>
				<Title level={3} style={{ color: "white", margin: 0 }}>
					E-Ink Display Layout Editor
				</Title>
			</Header>
			<Content
				style={{
					padding: "24px",
					maxWidth: "1200px",
					margin: "0 auto",
					width: "100%",
				}}
			>
				<div
					style={{
						background: "white",
						padding: "24px",
						borderRadius: "8px",
						boxShadow: "0 2px 8px rgba(0,0,0,0.06)",
					}}
				>
					<Toolbar addElement={addElement} />
					<ElementEditor
						element={
							selectedElement || {
								type: "none",
								x: 0,
								y: 0,
								anchor: "tl",
								level: 1,
							}
						}
						updateElement={updateElement}
						disabled={!selectedElement}
					/>
					<Canvas
						elements={elements}
						updateElement={updateElement}
						deleteElement={deleteElement}
						selectedElement={selectedElement}
						onElementSelect={handleElementSelect}
						onClick={handleCanvasClick}
					/>
					<Button
						type="primary"
						onClick={() => setShowExportModal(true)}
						style={{
							marginTop: "16px",
							height: "40px",
							borderRadius: "6px",
							fontWeight: "500",
						}}
					>
						Export JSON
					</Button>
					<Button type="primary"
						style={{
							marginLeft: "10px",
							marginTop: "16px",
							height: "40px",
							borderRadius: "6px",
							fontWeight: "500",
						}}
						onClick={() => {
							fetchLayout();
						}}>Fetch Layout</Button>
				</div>
				{showExportModal && (
					<ExportModal
						elements={elements}
						onClose={() => setShowExportModal(false)}
					/>
				)}
			</Content>
			<Footer
				style={{
					textAlign: "center",
					background: "#1a1a1a",
					color: "rgba(255,255,255,0.85)",
					padding: "16px",
				}}
			>
				E-Ink Display Editor ©2024
			</Footer>
		</Layout>
	);
}

export default App;