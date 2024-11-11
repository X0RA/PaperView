// ElementEditor.jsx
import React from 'react';
import { Card, Select, Space, InputNumber } from 'antd';
import { WHITE_DISPLAY } from './constants';

const { Option } = Select;

function ElementEditor({ element, updateElement }) {
  // Ensure we have defaults if not set
  const currentAnchor = element.anchor || 'tl';
  const currentLevel = element.level || 1;

  const anchorOptions = [
    { value: 'tl', label: 'Top Left' },
    { value: 'tm', label: 'Top Middle' },
    { value: 'tr', label: 'Top Right' },
    { value: 'bl', label: 'Bottom Left' },
    { value: 'bm', label: 'Bottom Middle' },
    { value: 'br', label: 'Bottom Right' },
    { value: 'm', label: 'Middle' },
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
      title={`Edit ${element.type.charAt(0).toUpperCase() + element.type.slice(1)}`}
      style={{ marginBottom: '16px' }}
      size="small"
    >
      <Space direction="vertical" style={{ width: '100%' }}>
        <div>
          <label style={{ display: 'block', marginBottom: '4px' }}>Position:</label>
          <Space>
            <InputNumber
              prefix="X:"
              value={Math.round(element.x)}
              onChange={(value) => handlePositionChange('x', value)}
            />
            <InputNumber
              prefix="Y:"
              value={Math.round(element.y)}
              onChange={(value) => handlePositionChange('y', value)}
            />
          </Space>
        </div>
        
        <div>
          <label style={{ display: 'block', marginBottom: '4px' }}>Anchor:</label>
          <Select
            style={{ width: '100%' }}
            value={currentAnchor}
            onChange={handleAnchorChange}
          >
            {anchorOptions.map(option => (
              <Option key={option.value} value={option.value}>
                {option.label}
              </Option>
            ))}
          </Select>
        </div>

        {(element.type === 'text' || element.type === 'button') && (
          <div>
            <label style={{ display: 'block', marginBottom: '4px' }}>Text Level:</label>
            <Select
              style={{ width: '100%' }}
              value={currentLevel}
              onChange={handleLevelChange}
            >
              {[1, 2, 3, 4].map((level) => (
                <Option key={level} value={level}>
                  <div style={{ 
                    color: WHITE_DISPLAY.textColors[level],
                    fontWeight: level === 1 ? 'bold' : 'normal'
                  }}>
                    Level {level}
                  </div>
                </Option>
              ))}
            </Select>
          </div>
        )}
        {element.type === 'button' && (
        <>
            <div>
            <label style={{ display: 'block', marginBottom: '4px' }}>Button Style:</label>
            <Space>
                <InputNumber
                prefix="Radius:"
                value={element.radius || 20}
                min={0}
                max={35}
                onChange={(value) => updateElement(element.id, { radius: value })}
                />
                <InputNumber
                prefix="Padding X:"
                value={element.padding_x || 10}
                min={0}
                max={100}
                onChange={(value) => updateElement(element.id, { padding_x: value })}
                />
                <InputNumber
                prefix="Padding Y:"
                value={element.padding_y || 5}
                min={0}
                max={50}
                onChange={(value) => updateElement(element.id, { padding_y: value })}
                />
            </Space>
            </div>
            <div>
            <label style={{ display: 'block', marginBottom: '4px' }}>Fill Style:</label>
            <Select
                style={{ width: '100%' }}
                value={element.filled !== false}
                onChange={(value) => updateElement(element.id, { filled: value })}
            >
                <Option value={true}>Filled</Option>
                <Option value={false}>Outlined</Option>
            </Select>
            </div>
        </>
        )}
      </Space>
    </Card>
  );
}

export default ElementEditor;