// Element.jsx
import React from 'react';
import { useDrag } from 'react-dnd';
import { WHITE_DISPLAY } from './constants';

const getRoundedButtonStyle = (width, height, radius, filled, level) => {
    // Calculate actual button dimensions including padding
    const buttonWidth = width;
    const buttonHeight = height;
    
    const backgroundColor = filled ? 
      (level === 1 ? '#000000' : '#cccccc') : 
      'transparent';
    
    const borderColor = level === 1 ? '#000000' : '#cccccc';
    
    return {
      width: buttonWidth,
      height: buttonHeight,
      backgroundColor,
      border: filled ? 'none' : `2px solid ${borderColor}`,
      borderRadius: `${radius}px`,
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'center',
      color: filled ? 
        (level === 1 ? '#ffffff' : '#000000') : 
        (level === 1 ? '#000000' : '#cccccc'),
      fontWeight: level === 1 ? 'bold' : 'normal',
      padding: '5px 10px', // Default padding from ButtonElement.h
      boxSizing: 'border-box',
      fontSize: '16px',
      position: 'relative',
    };
  };


function Element({ 
  element, 
  updateElement, 
  deleteElement,
  isSelected,
  onSelect 
}) {
  const [{ isDragging }, dragRef] = useDrag({
    type: 'element',
    item: element,
    collect: (monitor) => ({
      isDragging: monitor.isDragging(),
    }),
  });

  const { x, y, width, height, text, type, level = 1 } = element;
  
  const getTextColor = (level) => {
    return WHITE_DISPLAY.textColors[level];
  };

  const renderContent = () => {
    switch (type) {
      case 'text':
        return (
          <div style={{ 
            fontSize: '16px',
            color: getTextColor(level),
            fontWeight: level === 1 ? 'bold' : 'normal',
          }}>
            {text}
          </div>
        );
      
        case 'button':
        return (
          <div style={getRoundedButtonStyle(
            width, 
            height, 
            element.radius || 20, // Default radius from ExportModal
            element.filled !== false, // Default to filled
            level
          )}>
            {text}
          </div>
        );
      
      case 'image':
        return (
          <div style={{
            width,
            height,
            border: '1px dashed black'
          }} />
        );
    }
  };

  return (
    <div
      ref={dragRef}
      style={{
        position: 'absolute',
        left: x,
        top: y,
        cursor: 'move',
        opacity: isDragging ? 0.5 : 1,
        outline: isSelected ? '2px solid #1890ff' : 'none',
        padding: '4px',
      }}
      onClick={(e) => {
        e.stopPropagation();
        onSelect(element);
      }}
      onDoubleClick={(e) => {
        e.stopPropagation();
        const newText = prompt('Enter new text:', text);
        if (newText !== null) {
          updateElement(element.id, { text: newText });
        }
      }}
      onContextMenu={(e) => {
        e.preventDefault();
        if (window.confirm('Delete this element?')) {
          deleteElement(element.id);
        }
      }}
    >
      {renderContent()}
    </div>
  );
}

export default Element;