from PIL import Image
import math
import io

def convert_image_to_epd_data(filename, screen_width=1200, screen_height=826):
    """
    Convert an image file to EPD display format raw data.
    
    Args:
        filename: Path to the input image file
        screen_width: Target display width (default: 1200)
        screen_height: Target display height (default: 826)
    
    Returns:
        tuple: (width, height, bytes_data)
        where bytes_data is a bytearray of the processed image data
    """
    if screen_width % 2:
        raise ValueError("Screen width must be even!")

    # Open and process image
    im = Image.open(filename)
    im = im.convert(mode='L')
    im = im.rotate(90, expand=True)
    im.thumbnail((screen_width, screen_height), Image.Resampling.LANCZOS)
    
    # Calculate output size
    width = im.size[0]
    height = im.size[1]
    output_size = (math.ceil(width / 2) * 2 * height) // 2
    bytes_data = bytearray(output_size)
    
    # Process image data
    byte_index = 0
    for y in range(height):
        byte = 0
        for x in range(width):
            l = im.getpixel((x, y))
            if x % 2 == 0:
                byte = l >> 4
            else:
                byte |= l & 0xF0
                bytes_data[byte_index] = byte
                byte_index += 1
        if width % 2:  # Handle odd width
            bytes_data[byte_index] = byte
            byte_index += 1
    
    return (width, height, bytes_data)

def convert_image_bytes_to_epd_data(image_bytes, screen_width=1200, screen_height=826):
    """
    Convert image bytes data to EPD display format raw data.
    Args:
        image_bytes: Raw image bytes data
        screen_width: Target display width (default: 1200)
        screen_height: Target display height (default: 826)
    Returns:
        tuple: (width, height, bytes_data)
        where bytes_data is a bytearray of the processed image data
    """
    if screen_width % 2:
        raise ValueError("Screen width must be even!")
        
    # Create image from bytes
    im = Image.open(io.BytesIO(image_bytes))
    im = im.convert(mode='L')
    im = im.rotate(90, expand=True)
    im.thumbnail((screen_width, screen_height), Image.Resampling.LANCZOS)
    
    # Calculate output size
    width = im.size[0]
    height = im.size[1]
    output_size = (math.ceil(width / 2) * 2 * height) // 2
    bytes_data = bytearray(output_size)
    
    # Process image data
    byte_index = 0
    for y in range(height):
        byte = 0
        for x in range(width):
            l = im.getpixel((x, y))
            if x % 2 == 0:
                byte = l >> 4
            else:
                byte |= l & 0xF0
                bytes_data[byte_index] = byte
                byte_index += 1
        
        if width % 2:  # Handle odd width
            bytes_data[byte_index] = byte
            byte_index += 1
            
    return (width, height, bytes_data)


if __name__ == "__main__":
    try:
        width, height, data = convert_image_to_epd_data("test.png")
        print(f"Image converted: {width}x{height}, {len(data)} bytes")
        print("First 10 bytes:", " ".join(f"0x{b:02X}" for b in data[:10]))
    except Exception as e:
        print(f"Error: {e}")