// Canvas.jsx
import React from 'react';
import { useDrop } from 'react-dnd';
import Element from './Element';
import { calculateAnchoredPosition } from './constants';

function Canvas({ 
  elements, 
  updateElement, 
  deleteElement,
  selectedElement,
  onElementSelect,
  onClick
}) {
  const CANVAS_WIDTH = 960;
  const CANVAS_HEIGHT = 544;

  const [{ isOver }, dropRef] = useDrop({
    accept: 'element',
    drop: (item, monitor) => {
      const delta = monitor.getDifferenceFromInitialOffset();
      if (delta) {
        const rawX = Math.round(item.x + delta.x);
        const rawY = Math.round(item.y + delta.y);
        
        const anchoredPosition = calculateAnchoredPosition(
          { ...item, x: rawX, y: rawY },
          CANVAS_WIDTH,
          CANVAS_HEIGHT
        );
        
        updateElement(item.id, anchoredPosition);
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
        border: '1px solid #ccc',
        backgroundColor: '#fff',
        position: 'relative'
      }}
    >
      {elements.map((element) => {
        // Preserve all element properties while adding anchored position
        const anchoredPosition = calculateAnchoredPosition(
          element,
          CANVAS_WIDTH,
          CANVAS_HEIGHT
        );
        
        const elementWithPosition = {
          ...element,
          ...anchoredPosition,
        };
        
        return (
          <Element
            key={element.id}
            element={elementWithPosition}
            updateElement={updateElement}
            deleteElement={deleteElement}
            isSelected={selectedElement?.id === element.id}
            onSelect={onElementSelect}
          />
        );
      })}
    </div>
  );
}

export default Canvas;