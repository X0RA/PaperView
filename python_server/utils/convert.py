from PIL import Image
import math
import io

def convert_image_to_epd_data(image, target_width=1200, target_height=826):
    """
    Convert a PIL Image to EPD display format raw data with specific dimensions.
    
    Args:
        image: PIL Image object
        target_width: Desired width of the output image (default: 1200)
        target_height: Desired height of the output image (default: 826)
    
    Returns:
        tuple: (width, height, bytes_data)
        where bytes_data is a bytearray of the processed image data
    """
    if target_width % 2:
        raise ValueError("Target width must be even!")

    # Convert to grayscale and resize
    im = image.convert(mode='L')
    im = im.resize((target_width, target_height), Image.Resampling.LANCZOS)

    # Get dimensions
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

def convert_image_bytes_to_epd_data(image_bytes, target_width=1200, target_height=826):
    """
    Convert image bytes data to EPD display format raw data.
    
    Args:
        image_bytes: Raw image bytes data
        target_width: Desired width of the output image (default: 1200)
        target_height: Desired height of the output image (default: 826)
    
    Returns:
        tuple: (width, height, bytes_data)
        where bytes_data is a bytearray of the processed image data
    """
    image = Image.open(io.BytesIO(image_bytes))
    return convert_image_to_epd_data(image, target_width, target_height)

if __name__ == "__main__":
    try:
        # Example usage with a file
        image = Image.open("test.png")
        width, height, data = convert_image_to_epd_data(image)
        print(f"Image converted: {width}x{height}, {len(data)} bytes")
        print("First 10 bytes:", " ".join(f"0x{b:02X}" for b in data[:10]))
    except Exception as e:
        print(f"Error: {e}")