import serial
import time

if __name__ == '__main__':
    while True:
        try:
            # configure the serial connections (the parameters differ on the device you are connecting to)
            with serial.Serial(port='/dev/ttyACM0', baudrate=115200, timeout=1,
                               xonxoff=False, rtscts=False, dsrdtr=True) as s:
                while True:
                    line = s.readline()  # Read a line from the serial port
                    if line:  # Check if the line is not empty
                        print(line.decode('utf-8').strip())  # Decode and print the line
            break  # Exit the loop if the connection is successful

        except serial.SerialException:
            time.sleep(0.5)  # Wait 500ms before retrying
        except Exception as e:
            print(f'An error occurred: {e}')
            break  # Exit the loop on other exceptions

    if 's' in locals() and s.is_open:
        s.close()  # Ensure the serial port is closed if it was opened
