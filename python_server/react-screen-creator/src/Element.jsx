// Element.jsx
import React, { useRef, useEffect } from 'react';
import { useDrag } from 'react-dnd';
import TextElement from "./elements/TextElement";
import ButtonElement from "./elements/ButtonElement";
import ImageElement from "./elements/ImageElement";

function Element({ 
  element, 
  updateElement, 
  deleteElement,
  isSelected,
  onSelect 
}) {
  const elementRef = useRef(null);
  const [{ isDragging }, dragRef] = useDrag({
    type: "element",
    item: element,
    collect: (monitor) => ({
      isDragging: monitor.isDragging(),
    }),
  });

  // Measure and update element dimensions
  useEffect(() => {
    if (elementRef.current) {
      const { offsetWidth, offsetHeight } = elementRef.current;
      if (offsetWidth !== element.width || offsetHeight !== element.height) {
        updateElement(element.id, {
          width: offsetWidth,
          height: offsetHeight
        });
      }
    }
  }, [element.text, element.type]);

  // Combine refs
  const combineRefs = (...refs) => {
    return (node) => {
      refs.forEach(ref => {
        if (typeof ref === 'function') {
          ref(node);
        } else if (ref) {
          ref.current = node;
        }
      });
    };
  };

  const { x, y, text, type, level = 1 } = element;

  const renderContent = () => {
    switch (type) {
      case "text":
        return <TextElement text={text} level={level} />;

      case "button":
        return (
          <ButtonElement
            text={text}
            radius={element.radius}
            filled={element.filled !== false}
            level={level}
          />
        );

      case "image":
        return <ImageElement width={element.width || 200} height={element.height || 200} />;

      default:
        return null;
    }
  };

  return (
    <div
      ref={combineRefs(dragRef, elementRef)}
      style={{
        position: "absolute",
        left: x,
        top: y,
        cursor: "move",
        opacity: isDragging ? 0.5 : 1,
        outline: isSelected ? "1px solid #1890ff" : "none",
        padding: "0px",
      }}
      onClick={(e) => {
        e.stopPropagation();
        onSelect(element);
      }}
      onDoubleClick={(e) => {
        e.stopPropagation();
        const newText = prompt("Enter new text:", text);
        if (newText !== null) {
          updateElement(element.id, { text: newText });
        }
      }}
      onContextMenu={(e) => {
        e.preventDefault();
        if (window.confirm("Delete this element?")) {
          deleteElement(element.id);
        }
      }}
    >
      {renderContent()}
    </div>
  );
}

export default Element;