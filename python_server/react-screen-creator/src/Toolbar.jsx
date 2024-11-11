// Toolbar.jsx
import React, { useState } from 'react';
import { Button, Select, Space, Tooltip } from 'antd';
import { FileTextOutlined , AppstoreOutlined, PictureOutlined } from '@ant-design/icons';

function Toolbar({ addElement }) {
  const [selectedAnchor, setSelectedAnchor] = useState('tl');
  const [selectedLevel, setSelectedLevel] = useState(1);

  const anchorOptions = [
    { value: 'tl', label: 'Top Left' },
    { value: 'tm', label: 'Top Middle' },
    { value: 'tr', label: 'Top Right' },
    { value: 'bl', label: 'Bottom Left' },
    { value: 'bm', label: 'Bottom Middle' },
    { value: 'br', label: 'Bottom Right' },
    { value: 'm', label: 'Middle' },
  ];

  const createElement = (type) => {
    const newElement = {
      type,
      x: 50,
      y: 50,
      text: type === 'text' ? 'Sample Text' : type === 'button' ? 'Button' : '/image/path',
      width: type === 'image' ? 200 : 100,
      height: type === 'image' ? 200 : 50,
      anchor: selectedAnchor,
      level: selectedLevel,
      // Add button-specific properties
      ...(type === 'button' && {
        radius: 20,
        padding_x: 10,
        padding_y: 5,
        filled: true
      })
    };
    addElement(newElement);
  };

  
  return (
    <div style={{ 
      marginBottom: '24px',
      padding: '16px',
      background: '#f8f9fa',
      borderRadius: '6px',
      border: '1px solid #e8e8e8'
    }}>
      <Space size="middle" wrap>
        <Select
          style={{ width: 140 }}
          value={selectedAnchor}
          onChange={(value) => setSelectedAnchor(value)}
          options={anchorOptions}
          placeholder="Select Anchor"
        />
        <Select
          style={{ width: 140 }}
          value={selectedLevel}
          onChange={(value) => setSelectedLevel(value)}
          options={[1, 2, 3, 4].map((level) => ({
            value: level,
            label: `Level ${level}`,
          }))}
          placeholder="Select Level"
        />
        <Space size="small">
          <Tooltip title="Add Text">
            <Button 
              icon={<FileTextOutlined />} 
              onClick={() => createElement('text')}
              type="primary"
              ghost
            >
              Text
            </Button>
          </Tooltip>
          <Tooltip title="Add Button">
            <Button 
              icon={<AppstoreOutlined />} 
              onClick={() => createElement('button')}
              type="primary"
              ghost
            >
              Button
            </Button>
          </Tooltip>
          <Tooltip title="Add Image">
            <Button 
              icon={<PictureOutlined />} 
              onClick={() => createElement('image')}
              type="primary"
              ghost
            >
              Image
            </Button>
          </Tooltip>
        </Space>
      </Space>
    </div>
  );
}

export default Toolbar;