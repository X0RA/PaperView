from selenium import webdriver
from selenium.webdriver.chrome.options import Options
import base64
import io
import time
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By

def get_screenshot_data(url, window_width=540, window_height=1099):
    """
    Takes a screenshot of a webpage and returns the image data as a base64 string
    
    Args:
        url (str): The URL to capture
        window_width (int): Width of the browser window
        window_height (int): Height of the browser window
    
    Returns:
        str: Base64 encoded PNG image data
    """
    chrome_options = Options()
    chrome_options.add_argument("--headless")
    chrome_options.add_argument(f"--window-size={window_width},{window_height}")
    chrome_options.add_argument("--hide-scrollbars")
    
    driver = webdriver.Chrome(options=chrome_options)
    
    # might break
    driver.execute_cdp_cmd("Network.setBlockedURLs", {"urls": ["*plausible.io*"]})
    driver.execute_cdp_cmd("Network.enable", {})
    
    try:
        driver.get(url)
        
        driver.execute_script(f"""
            document.body.style.overflow = 'hidden';
            document.body.style.width = '{window_width}px';
            document.body.style.height = '{window_height}px';
            window.resizeTo({window_width}, {window_height});
        """)
        
        driver.implicitly_wait(1)
        
        image_data = driver.get_screenshot_as_base64()
        return image_data
        
    finally:
        driver.quit()

def get_screenshot_bytes(url, window_width=540, window_height=1099):
    """
    Takes a screenshot of a webpage and returns the raw image bytes
    
    Args:
        url (str): The URL to capture
        window_width (int): Width of the browser window
        window_height (int): Height of the browser window
    
    Returns:
        bytes: Raw PNG image data
    """
    chrome_options = Options()
    chrome_options.add_argument("--headless")
    chrome_options.add_argument(f"--window-size={window_width},{window_height}")
    chrome_options.add_argument("--hide-scrollbars")
    
        # Add these arguments to disable web security and CORS
    chrome_options.add_argument("--disable-web-security")
    chrome_options.add_argument("--disable-site-isolation-trials")
    chrome_options.add_argument("--allow-running-insecure-content")
    chrome_options.add_argument("--no-sandbox")
    chrome_options.add_argument("--disable-gpu")
    
     # If needed, you can also add this to bypass same-origin policy
    chrome_options.add_argument("--disable-features=IsolateOrigins,site-per-process")
    
    driver = webdriver.Chrome(options=chrome_options)
    
    # might break
    driver.execute_cdp_cmd("Network.setBlockedURLs", {"urls": ["*plausible.io*"]})
    driver.execute_cdp_cmd("Network.enable", {})
    
    try:
        driver.get(url)
        # time.sleep(2)
        WebDriverWait(driver, 10).until(
            lambda d: d.find_element(By.CSS_SELECTOR, '[data-loaded="true"]')
        )
        
        driver.execute_script(f"""
            document.body.style.overflow = 'hidden';
            document.body.style.width = '{window_width}px';
            document.body.style.height = '{window_height}px';
            window.resizeTo({window_width}, {window_height});
        """)
        
        driver.implicitly_wait(1)
        
        image_bytes = driver.get_screenshot_as_png()
        return image_bytes
        
    finally:
        driver.quit()

def save_screenshot(url, output_file="screenshot.png", window_width=540, window_height=1099):
    """
    Takes a screenshot of a webpage and saves it to a file
    
    Args:
        url (str): The URL to capture
        output_file (str): Path where the screenshot should be saved
        window_width (int): Width of the browser window
        window_height (int): Height of the browser window
    
    Returns:
        bool: True if successful, False otherwise
    """
    try:
        image_bytes = get_screenshot_bytes(url, window_width, window_height)
        with open(output_file, "wb") as f:
            f.write(image_bytes)
        return True
    except Exception as e:
        print(f"Error saving screenshot: {e}")
        return False

if __name__ == "__main__":
    # Example URL - replace with your target URL
    test_url = "http://localhost:5173/"
    
    # Example 1: Get base64 encoded image data
    base64_data = get_screenshot_data(test_url)
    print("Base64 data length:", len(base64_data))
    
    # Example 2: Get raw bytes
    bytes_data = get_screenshot_bytes(test_url)
    print("Bytes data length:", len(bytes_data))
    
    # Example 3: Save to file
    success = save_screenshot(test_url, "example_screenshot.png")
    if success:
        print("Screenshot saved successfully to example_screenshot.png")
    else:
        print("Failed to save screenshot")