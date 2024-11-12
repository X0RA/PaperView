// Toolbar.jsx
import React, { useState } from 'react';
import { Button, Select, Space, Tooltip } from 'antd';
import { FileTextOutlined, AppstoreOutlined, PictureOutlined } from '@ant-design/icons';

// Constants
const ANCHOR_OPTIONS = [
  { value: 'tl', label: 'Top Left' },
  { value: 'tm', label: 'Top Middle' },
  { value: 'tr', label: 'Top Right' },
  { value: 'bl', label: 'Bottom Left' },
  { value: 'bm', label: 'Bottom Middle' },
  { value: 'br', label: 'Bottom Right' },
  { value: 'm', label: 'Middle' },
];

const LEVEL_OPTIONS = [1, 2, 3, 4].map((level) => ({
  value: level,
  label: `Level ${level}`,
}));

const TOOLBAR_BUTTONS = [
  {
    type: 'text',
    icon: <FileTextOutlined />,
    tooltip: 'Add Text',
    label: 'Text',
    defaultProps: {
      text: 'Sample Text',
      width: 100,
      height: 50,
    },
  },
  {
    type: 'button',
    icon: <AppstoreOutlined />,
    tooltip: 'Add Button',
    label: 'Button',
    defaultProps: {
      text: 'Button',
      width: 100,
      height: 50,
      radius: 20,
      padding_x: 10,
      padding_y: 5,
      filled: true,
    },
  },
  {
    type: 'image',
    icon: <PictureOutlined />,
    tooltip: 'Add Image',
    label: 'Image',
    defaultProps: {
      text: '/image/path',
      width: 200,
      height: 200,
    },
  },
];

const TOOLBAR_STYLES = {
  container: {
    marginBottom: '24px',
    padding: '16px',
    background: '#f8f9fa',
    borderRadius: '6px',
    border: '1px solid #e8e8e8',
  },
  select: {
    width: 140,
  },
};

// Reusable ToolbarButton component
const ToolbarButton = ({ icon, tooltip, label, onClick }) => (
  <Tooltip title={tooltip}>
    <Button 
      icon={icon}
      onClick={onClick}
      type="primary"
      ghost
    >
      {label}
    </Button>
  </Tooltip>
);

function Toolbar({ addElement }) {
  const [selectedAnchor, setSelectedAnchor] = useState('tl');
  const [selectedLevel, setSelectedLevel] = useState(1);

  const createElement = (type) => {
    const buttonConfig = TOOLBAR_BUTTONS.find(btn => btn.type === type);
    const newElement = {
      type,
      x: 50,
      y: 50,
      anchor: selectedAnchor,
      level: selectedLevel,
      ...buttonConfig.defaultProps,
    };
    addElement(newElement);
  };

  return (
    <div style={TOOLBAR_STYLES.container}>
      <Space size="middle" wrap>
        <Select
          style={TOOLBAR_STYLES.select}
          value={selectedAnchor}
          onChange={setSelectedAnchor}
          options={ANCHOR_OPTIONS}
          placeholder="Select Anchor"
        />
        <Select
          style={TOOLBAR_STYLES.select}
          value={selectedLevel}
          onChange={setSelectedLevel}
          options={LEVEL_OPTIONS}
          placeholder="Select Level"
        />
        <Space size="small">
          {TOOLBAR_BUTTONS.map((button) => (
            <ToolbarButton
              key={button.type}
              icon={button.icon}
              tooltip={button.tooltip}
              label={button.label}
              onClick={() => createElement(button.type)}
            />
          ))}
        </Space>
      </Space>
    </div>
  );
}

export default Toolbar;