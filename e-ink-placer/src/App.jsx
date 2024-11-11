// App.jsx
import React, { useState, useRef } from 'react';
import Canvas from './Canvas';
import Toolbar from './Toolbar';
import ElementEditor from './ElementEditor'; // New component
import ExportModal from './ExportModal';
import { Button, Layout, Typography } from 'antd';

const { Header, Content, Footer } = Layout;
const { Title } = Typography;

function App() {
  const [elements, setElements] = useState([]);
  const [showExportModal, setShowExportModal] = useState(false);
  const [selectedElement, setSelectedElement] = useState(null);
  const elementIdCounter = useRef(1);  

  const addElement = (element) => {
    const newElement = {
      ...element,
      id: elementIdCounter.current
    };
    elementIdCounter.current += 1; 
    setElements([...elements, newElement]);
  };

  const updateElement = (id, newProps) => {
    setElements(
      elements.map((el) => {
        if (el.id === id) {
          const updatedElement = { ...el, ...newProps };
          // Update selectedElement if this is the currently selected element
          if (selectedElement?.id === id) {
            setSelectedElement(updatedElement);
          }
          return updatedElement;
        }
        return el;
      })
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
    <Layout style={{ minHeight: '100vh', background: '#f5f5f5' }}>
      <Header style={{ 
        background: '#1a1a1a', 
        padding: '0 24px',
        display: 'flex',
        alignItems: 'center',
        boxShadow: '0 2px 8px rgba(0,0,0,0.15)'
      }}>
        <Title level={3} style={{ color: 'white', margin: 0 }}>E-Ink Display Layout Editor</Title>
      </Header>
      <Content style={{ padding: '24px', maxWidth: '1200px', margin: '0 auto', width: '100%' }}>
        <div style={{ 
          background: 'white', 
          padding: '24px',
          borderRadius: '8px',
          boxShadow: '0 2px 8px rgba(0,0,0,0.06)'
        }}>
          <Toolbar addElement={addElement} />
          {selectedElement && (
            <ElementEditor 
              element={selectedElement}
              updateElement={updateElement}
            />
          )}
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
              marginTop: '16px',
              height: '40px',
              borderRadius: '6px',
              fontWeight: '500'
            }}
          >
            Export JSON
          </Button>
        </div>
        {showExportModal && (
          <ExportModal elements={elements} onClose={() => setShowExportModal(false)} />
        )}
      </Content>
      <Footer style={{ 
        textAlign: 'center', 
        background: '#1a1a1a', 
        color: 'rgba(255,255,255,0.85)',
        padding: '16px'
      }}>
        E-Ink Display Editor ©2024
      </Footer>
    </Layout>
  );
}

export default App;