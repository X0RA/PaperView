// Canvas.jsx
import React from 'react';
import { useDrop } from 'react-dnd';
import Element from "./Element";

function Canvas({ 
  elements, 
  updateElement, 
  deleteElement,
  selectedElement,
  onElementSelect,
  onClick
}) {
	const CANVAS_WIDTH = 960 - 330;
	const CANVAS_HEIGHT = 540 - 180;

  const [{ isOver }, dropRef] = useDrop({
    accept: 'element',
    drop: (item, monitor) => {
      const delta = monitor.getDifferenceFromInitialOffset();
      if (delta) {
		  const newX = Math.round(item.x + delta.x);
		  const newY = Math.round(item.y + delta.y);
		  updateElement(item.id, {
			  x: newX,
			  y: newY,
		  });
      }
    },
    collect: (monitor) => ({
      isOver: monitor.isOver()
    })
  });

  return (
	  <div
		  ref={dropRef}
		  onClick={onClick}
		  style={{
			  width: CANVAS_WIDTH,
			  height: CANVAS_HEIGHT,
			  border: "1px solid #ccc",
			  backgroundColor: "#fff",
			  position: "relative",
		  }}
	  >
		  {elements.map((element) => (
			  <Element
				  key={element.id}
				  element={element} // Pass the element directly without anchor calculations
				  updateElement={updateElement}
				  deleteElement={deleteElement}
				  isSelected={selectedElement?.id === element.id}
				  onSelect={onElementSelect}
			  />
		  ))}
	  </div>
  );
}

export default Canvas;