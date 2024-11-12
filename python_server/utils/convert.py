from PIL import Image
import math
import io
import numpy as np

def adjust_brightness(image, brightness_factor):
    """
    Adjust brightness of an image by a given factor.
    
    Args:
        image: PIL Image object
        brightness_factor: Factor by which to adjust brightness (1.0 for no change, 0.0 for black, 2.0 for twice the brightness)
    
    Returns:
        PIL Image object with adjusted brightness
    """
    # Convert to numpy array for easier manipulation
    img_array = np.array(image)
    
    # Adjust brightness
    img_array = np.clip(img_array * brightness_factor, 0, 255).astype(np.uint8)
    
    # Convert back to PIL Image
    im = Image.fromarray(img_array)
    return im

def dither_floyd(img, num_levels=16):
    """
    Apply Floyd-Steinberg dithering to a grayscale image.
    
    Args:
        img: PIL Image object (will be converted to grayscale if it isn't already)
        num_levels: Number of gray levels (default=16)
    
    Returns:
        PIL Image object with the dithered result
    """
    # Convert to grayscale if it isn't already
    img = img.convert('L')
    
    # Convert to numpy array and normalize to [0,1]
    arr = np.array(img, dtype=float) / 255.0
    height, width = arr.shape
    
    # Function to find nearest available gray level
    def get_nearest_level(val):
        return np.round(val * (num_levels - 1)) / (num_levels - 1)
    
    # Apply Floyd-Steinberg dithering
    for y in range(height):
        for x in range(width):
            old_val = arr[y, x]
            new_val = get_nearest_level(old_val)
            arr[y, x] = new_val
            
            error = old_val - new_val
            
            # Distribute error to neighboring pixels
            if x < width - 1:
                arr[y, x + 1] += error * 7/16
            if y < height - 1:
                if x > 0:
                    arr[y + 1, x - 1] += error * 3/16
                arr[y + 1, x] += error * 5/16
                if x < width - 1:
                    arr[y + 1, x + 1] += error * 1/16
    
    # Convert back to 8-bit image
    result = Image.fromarray((arr * 255).astype(np.uint8))
    return result

def dither_grayscale(img, num_levels=16):
    """
    Apply Sierra dithering to a grayscale image.
    Args:
        img: PIL Image object (will be converted to grayscale if it isn't already)
        num_levels: Number of gray levels (default=16)
    Returns:
        PIL Image object with the dithered result
    """
    # Convert to grayscale if it isn't already
    img = img.convert('L')
    
    # Convert to numpy array and normalize to [0,1]
    arr = np.array(img, dtype=float) / 255.0
    height, width = arr.shape
    
    # Function to find nearest available gray level
    def get_nearest_level(val):
        return np.round(val * (num_levels - 1)) / (num_levels - 1)
    
    # Apply Sierra dithering
    for y in range(height):
        for x in range(width):
            old_val = arr[y, x]
            new_val = get_nearest_level(old_val)
            arr[y, x] = new_val
            error = old_val - new_val
            
            # Distribute error using Sierra pattern
            if x < width - 1:
                arr[y, x + 1] += error * 5/32
            if x < width - 2:
                arr[y, x + 2] += error * 3/32
                
            if y < height - 1:
                if x > 1:
                    arr[y + 1, x - 2] += error * 2/32
                if x > 0:
                    arr[y + 1, x - 1] += error * 4/32
                arr[y + 1, x] += error * 5/32
                if x < width - 1:
                    arr[y + 1, x + 1] += error * 4/32
                if x < width - 2:
                    arr[y + 1, x + 2] += error * 2/32
                    
            if y < height - 2:
                if x > 0:
                    arr[y + 2, x - 1] += error * 2/32
                arr[y + 2, x] += error * 3/32
                if x < width - 1:
                    arr[y + 2, x + 1] += error * 2/32
    
    # Convert back to 8-bit image
    result = Image.fromarray((arr * 255).astype(np.uint8))
    return result

def adjust_contrast(image, contrast_factor):
    """
    Adjust contrast of an image by a given factor.
    Args:
        image: PIL Image object
        contrast_factor: Factor by which to adjust contrast (1.0 for no change, 0.0 for gray, 2.0 for twice the contrast)
    Returns:
        PIL Image object with adjusted contrast
    """
    # Convert to numpy array and normalize to [0,1]
    img_array = np.array(image, dtype=float) / 255.0
    
    # Calculate mean
    mean = np.mean(img_array)
    
    # Adjust contrast around the mean
    img_array = (img_array - mean) * contrast_factor + mean
    
    # Clip values to [0,1] range
    img_array = np.clip(img_array, 0, 1)
    
    # Convert back to 8-bit image
    result = Image.fromarray((img_array * 255).astype(np.uint8))
    return result

def adjust_gamma(image, gamma):
    """
    Adjust gamma of an image.
    Args:
        image: PIL Image object
        gamma: Gamma value (1.0 for no change)
            - gamma < 1 will brighten the image (e.g., 0.5)
            - gamma > 1 will darken the image (e.g., 2.0)
    Returns:
        PIL Image object with adjusted gamma
    """
    # Convert to numpy array and normalize to [0,1]
    img_array = np.array(image, dtype=float) / 255.0
    
    # Apply gamma correction
    img_array = np.power(img_array, gamma)
    
    # Convert back to 8-bit image
    result = Image.fromarray((img_array * 255).astype(np.uint8))
    return result

def analyze_and_adjust_image(im):
    """
    Analyzes image and returns appropriate adjustment values based on histogram analysis
    Args:
        im: PIL Image object
    Returns:
        PIL Image object with dynamic adjustments applied
    """
    # Convert to grayscale for analysis
    gray_im = im.convert(mode="L")
    histogram = gray_im.histogram()
    
    # Calculate histogram statistics
    total_pixels = sum(histogram)
    weighted_sum = sum(i * count for i, count in enumerate(histogram))
    mean_brightness = weighted_sum / total_pixels
    
    # Calculate standard deviation
    variance = sum((i - mean_brightness) ** 2 * count for i, count in enumerate(histogram)) / total_pixels
    std_dev = variance ** 0.5
    
    # Calculate percentiles for shadow and highlight analysis
    cumsum = 0
    for i, count in enumerate(histogram):
        cumsum += count
        if cumsum >= total_pixels * 0.05:  # 5th percentile
            shadows = i
            break
            
    cumsum = 0
    for i, count in reversed(list(enumerate(histogram))):
        cumsum += count
        if cumsum >= total_pixels * 0.05:  # 95th percentile
            highlights = i
            break
    
    # Dynamic parameter calculation
    gamma = 1.0
    contrast = 1.0
    brightness = 1.0
    
    
    print(mean_brightness, std_dev, shadows, highlights)
    
    # Adjust gamma based on mean brightness
    if mean_brightness < 128:
        # Image is dark, use gamma < 1 to brighten shadows
        gamma = 0.6 + (mean_brightness / 128) * 0.2
    else:
        # Image is bright, use gamma > 1 to prevent washout
        gamma = 0.9 + ((mean_brightness - 128) / 128) * 0.5
    
    # Adjust contrast based on standard deviation
    if std_dev < 50:
        # Low contrast image, increase contrast
        contrast = 1.2 + (50 - std_dev) / 50 * 0.3
    else:
        # High contrast image, reduce contrast
        contrast = 1.0 - (std_dev - 50) / 100 * 0.2 + 0.2
    
    # Adjust brightness based on shadows and highlights
    if shadows < 20:
        # Dark shadows, increase brightness
        brightness = 1.1 + (20 - shadows) / 20 * 0.3
    elif highlights > 235:
        # Bright highlights, decrease brightness
        brightness = 0.9 - (highlights - 235) / 20 * 0.2
    
    brightness += 0.1
    gamma -= 0.1
    
    print(f"Applied parameters - Gamma: {gamma:.2f}, Contrast: {contrast:.2f}, Brightness: {brightness:.2f}")
    
    # Apply adjustments in the correct order
    im = adjust_gamma(im, gamma)
    im = adjust_contrast(im, contrast)
    im = im.convert(mode="L")
    im = adjust_brightness(im, brightness)
    im = dither_grayscale(im, 16)
    
    return im

def convert_image_to_epd_data(image, target_width=1200, target_height=826, brightness_factor = 1.3):
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

    im = image.resize((target_width, target_height), Image.Resampling.LANCZOS)
    
    im = analyze_and_adjust_image(im)

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